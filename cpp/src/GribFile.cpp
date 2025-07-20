#include <GribFile.hpp>
#include <GribMessage.hpp>
#include <ThreadPool.hpp>
#include <CoordinateSystem.hpp>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_set>
#include <iostream>
#include <thread>
#include <netcdf>

bool GribFile::Metadata::operator==(const Metadata& other) const {
    return centers == other.centers &&
        variables == other.variables &&
        totalMessages == other.totalMessages &&
        estimatedMemorySize == other.estimatedMemorySize &&
        coordinates == other.coordinates &&
        hasConsistentGrid == other.hasConsistentGrid;
}

bool GribFile::Metadata::operator!=(const Metadata& other) const {
    return !(*this == other);
}

void GribFile::MessageCache::add(size_t index, std::shared_ptr<GribMessage> message) {
    // If this index is already in cache, remove it from LRU list
    if (contains(index)) {
        lruList.remove(index);
    }
    // If cache is full, remove least recently used item
    else if (cache.size() >= MAX_CACHE_SIZE) {
        size_t lruIndex = lruList.back();
        lruList.pop_back();
        cache.erase(lruIndex);
    }

    // Add new message to cache
    cache[index] = message;
    // Add index to front of LRU list (most recently used)
    lruList.push_front(index);
}

std::shared_ptr<GribMessage> GribFile::MessageCache::get(size_t index) {
    if (!contains(index)) {
        return nullptr;
    }

    // Move accessed index to front of LRU list
    lruList.remove(index);
    lruList.push_front(index);

    return cache[index];
}

bool GribFile::MessageCache::contains(size_t index) const {
    return cache.find(index) != cache.end();
}

GribFile::GribFile(std::string filepath, bool validate) {
    filepath_ = filepath;
    loadMetadata(validate);
}

GribFile::GribFile(const GribFile& other)
    :
        filepath_(other.filepath_),
        metadata_(other.metadata_),
        messageCache_() {}

GribFile::GribFile(GribFile&& other) noexcept
    :
        filepath_(std::move(other.filepath_)),
        metadata_(std::move(other.metadata_)),
        messageCache_(std::move(other.messageCache_)) {}

GribFile GribFile::operator=(const GribFile& other) {
    if (this != &other) {
        filepath_ = other.filepath_;
        metadata_ = other.metadata_;
        messageCache_ = std::make_unique<MessageCache>();
        if (other.messageCache_) {
            *messageCache_ = *other.messageCache_;
        }
    }
    return *this;
}

GribFile GribFile::operator=(GribFile&& other) {
    if (this != &other) {
        filepath_ = std::move(other.filepath_);
        metadata_ = std::move(other.metadata_);
        messageCache_ = std::move(other.messageCache_);
    }
    return *this;
}

bool GribFile::operator==(const GribFile& other) const {
    return filepath_ == other.filepath_ &&
        metadata_ == other.metadata_;
}

bool GribFile::operator!=(const GribFile& other) const {
    return !(*this == other);
}

GribFile::FileGuard::FileGuard(const std::string& filepath) {
    file_ = fopen(filepath.c_str(), "rb");
    if (!file_) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
}

GribFile::FileGuard::~FileGuard() {
    if (file_) {
        fclose(file_);
        file_ = nullptr;
    }
}

GribFile::FileGuard::FileGuard(FileGuard&& other) noexcept
    : file_(other.file_) {
        other.file_ = nullptr;
    }

GribFile::FileGuard& GribFile::FileGuard::operator=(FileGuard&& other) noexcept {
    if (this != &other) {
        if (file_) {
            fclose(file_);
        }
        file_ = other.file_;
        other.file_ = nullptr;
    }
    return *this;
}

GribFile::FileGuard GribFile::openFile() const {
    return FileGuard(filepath_);
}

GribFile::CodesHandleGuard::CodesHandleGuard(FILE* file) {
    int err = 0;
    handle_ = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err);
    if (!handle_) {
        throw std::runtime_error("Failed to create GRIB handle.");
    }

    if (err != CODES_SUCCESS) {
        throw std::runtime_error("Error reading GRIB message: " + std::to_string(err));
    }
}

GribFile::CodesHandleGuard::CodesHandleGuard(const FileGuard& fileGuard) {
    *this = CodesHandleGuard(fileGuard.get());
}

GribFile::CodesHandleGuard::~CodesHandleGuard() {
    if (handle_) {
        codes_handle_delete(handle_);
        handle_ = nullptr;
    }
}

GribFile::CodesHandleGuard::CodesHandleGuard(CodesHandleGuard&& other) noexcept
    : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

GribFile::CodesHandleGuard& GribFile::CodesHandleGuard::operator=(CodesHandleGuard&& other) noexcept {
    if (this != &other) {
        if (handle_) {
            codes_handle_delete(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

GribFile::CodesHandleGuard GribFile::getHandle(const FileGuard& fileGuard) const {
    return CodesHandleGuard(fileGuard);
}

GribFile::Iterator& GribFile::Iterator::operator++() {
    currentIndex_++;
    currentMessage_.reset();
    return *this;
}

GribFile::Iterator GribFile::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

const GribMessage& GribFile::Iterator::operator*() const {
    if (!currentMessage_) {
        currentMessage_ = parent_->loadMessage(currentIndex_);
    }
    return *currentMessage_;
}

const GribMessage* GribFile::Iterator::operator->() const {
    if (!currentMessage_) {
        currentMessage_ = parent_->loadMessage(currentIndex_);
    }
    return currentMessage_.get();
}

bool GribFile::Iterator::operator!=(const Iterator& other) const {
    return currentIndex_ != other.currentIndex_ || parent_ != other.parent_;
}

bool GribFile::Iterator::operator==(const Iterator& other) const {
    return !(*this != other);
}

GribFile::Iterator GribFile::begin() {
    return Iterator(this, 0);
}

GribFile::Iterator GribFile::end() {
    return Iterator(this, metadata_.totalMessages);
}

void GribFile::loadMetadata(bool validate) {
    // Reset metadata
    metadata_ = Metadata();
    metadata_.hasConsistentGrid = true;

    // Initialize message cache if not already created.
    if (!messageCache_) {
        messageCache_ = std::make_unique<MessageCache>();
    } else {
        messageCache_->cache.clear();
        messageCache_->lruList.clear();
    }

    FileGuard fileGuard = openFile();
    CodesHandleGuard handleGuard = getHandle(fileGuard);
    FILE* file = fileGuard.get();
    codes_handle* h = handleGuard.get();

    int err = 0;
    size_t messageCount = 0;
    size_t cachedCount = 0;
    CoordinateSystem::GridType firstGridType;
    bool firstMessage = true;

    off_t currentOffset = ftell(file);

    // Process all messages in the file.
    while ((h = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err)) != nullptr) {
        auto handleGuard = [](codes_handle* handle) {
            if (handle) codes_handle_delete(handle);
        };
        std::unique_ptr<codes_handle, decltype(handleGuard)> handleCleanup(h, handleGuard);

        if (err != CODES_SUCCESS) {
            throw std::runtime_error("Error readding GRIB message: " + std::to_string(err));
        }

        messageCount++;

        // Extract center information
        long centerCode = 0;
        CODES_CHECK(codes_get_long(h, "centre", &centerCode), 0);
        Center center = center_from_code(centerCode);
        metadata_.centers.insert(center);

        // Extract variable information
        long varCode = 0;
        CODES_CHECK(codes_get_long(h, "indicatorOfParameter", &varCode), 0);
        Variable variable = variable_from_code(varCode);
        metadata_.variables.insert(variable);

        // Extract grid type
        char gridType[256] = {0};
        size_t len = sizeof(gridType);
        CODES_CHECK(codes_get_string(h, "gridType", gridType, &len), 0);
        CoordinateSystem::GridType currentGridType = CoordinateSystem::stringToGridType(gridType);

        // Extract time information
        TimeInfo timeInfo = extractTimeInfo(h);
        metadata_.times.insert(timeInfo);

        // Extract ensemble information
        EnsembleInfo ensInfo = extractEnsembleInfo(h);
        metadata_.ensembles.insert(ensInfo);

        if (firstMessage) {
            firstGridType = currentGridType;
            firstMessage = false;
        } else if (currentGridType != firstGridType) {
            metadata_.hasConsistentGrid = false;
            break;
        }

        // Estimate memory requirements
        size_t valuesLen = 0;
        CODES_CHECK(codes_get_size(h, "values", &valuesLen), 0);
        metadata_.estimatedMemorySize += valuesLen * sizeof(float);

        if (messageCache_ && cachedCount < MessageCache::MAX_CACHE_SIZE) {
            TimeInfo timeInfo = extractTimeInfo(h);
            EnsembleInfo ensInfo = extractEnsembleInfo(h);
            CoordinateSystem coords = extractCoordinateSystem(h);

            // Get data values
            std::vector<double> values(valuesLen);
            CODES_CHECK(codes_get_double_array(h, "values", values.data(), &valuesLen), 0);

            std::shared_ptr<GribMessage> message = std::make_shared<GribMessage>(
                timeInfo, ensInfo,
                std::vector<float>(values.begin(), values.end()),
                coords,
                variable, center
            );

            messageCache_->add(cachedCount, std::move(message));
            cachedCount++;
        }

        // Store in map for later validation
        DimensionKey key = std::make_tuple(
            timeInfo,
            ensInfo.memberNumber,
            variable,
            center
        );
        if (dimensionMessages_.find(key) == dimensionMessages_.end()) {
            dimensionMessages_[key] = std::vector<off_t>();
        }
        dimensionMessages_[key].push_back(currentOffset);
        messageOffsets_.push_back(currentOffset);
        currentOffset = ftell(file);

    }

    if (err != CODES_SUCCESS && err != CODES_END_OF_FILE) {
        throw std::runtime_error("Error reading GRIB file: " + std::to_string(err));
    }

    if (messageCount == 0) {
        throw std::runtime_error("No valid GRIB messages found in file: " + filepath_);
    }

    if (!metadata_.hasConsistentGrid) {
        throw std::runtime_error("Warning: Inconsistent grid types found in GRIB file: " + filepath_);
    }

    metadata_.totalMessages = messageCount;
    if (validate) {
        metadata_.hasConsistentGrid = validateGridConsistency();
    }

    // Set the coordinates.
    auto firstCachedMessage = messageCache_->get(0);
    metadata_.coordinates = firstCachedMessage->getCoordinateSystem();
}

CoordinateSystem GribFile::extractCoordinateSystem(codes_handle* h) const {
    char gridTypeStr[256] = {0};
    size_t len = sizeof(gridTypeStr);
    CODES_CHECK(codes_get_string(h, "gridType", gridTypeStr, &len), 0);
    CoordinateSystem::GridType gridType = CoordinateSystem::stringToGridType(gridTypeStr);

    switch (gridType) {
        case CoordinateSystem::GridType::REGULAR_LATLON:
            return extractRegularGrid(h);
        default:
            throw std::runtime_error("Unexpected grid type: " + std::string(gridTypeStr));
    }
}

CoordinateSystem GribFile::extractRegularGrid(codes_handle* h) const {
    long ni = 0, nj = 0;
    CODES_CHECK(codes_get_long(h, "Ni", &ni), 0);
    CODES_CHECK(codes_get_long(h, "Nj", &nj), 0);

    if (ni <= 0 || nj <= 0) {
        throw std::runtime_error("Invalid grid dimensions in GRIB message");
    }

    // Extract grid parameters
    double lat1 = 0.0, lon1 = 0.0, latIncrement = 0.0, lonIncrement = 0.0;
    CODES_CHECK(codes_get_double(h, "latitudeOfFirstGridPointInDegrees", &lat1), 0);
    CODES_CHECK(codes_get_double(h, "longitudeOfFirstGridPointInDegrees", &lon1), 0);
    CODES_CHECK(codes_get_double(h, "iDirectionIncrementInDegrees", &lonIncrement), 0);
    CODES_CHECK(codes_get_double(h, "jDirectionIncrementInDegrees", &latIncrement), 0);

    // Handle the scanning mode
    long scanningMode = 0;
    CODES_CHECK(codes_get_long(h, "scanningMode", &scanningMode), 0);
    bool iScansNegatively = (scanningMode & 128) != 0;
    bool jScansPositively = (scanningMode & 64) != 0;

    // Generate coordinates
    std::unordered_set<float> latitudes;
    std::unordered_set<float> longitudes;

    // Generate longitudes
    longitudes.reserve(ni);
    for (long i = 0; i < ni; i++) {
        double lon = iScansNegatively ?
            lon1 - i * lonIncrement :
            lon1 + i * lonIncrement;

        // Normalize longitude to [0, 360)
        while (lon < 0) lon += 360.0;
        while (lon >= 360.0) lon -= 360.0;

        longitudes.insert(static_cast<float>(lon));
    }

    // Generate latitudes
    for (long j = 0; j < nj; j++) {
        double lat = jScansPositively ?
            lat1 + j * latIncrement :
            lat1 - j * latIncrement;

        latitudes.insert(static_cast<float>(lat));
    }

    return CoordinateSystem::createRegularGrid(
        latitudes, longitudes
    );
}

TimeInfo GribFile::extractTimeInfo(codes_handle* h) const {
    TimeInfo timeInfo;
    // Extract the date components
    long dataDate;
    if (codes_get_long(h, "dataDate", &dataDate) == 0) {
        // dataDate format is YYYYMMDD
        timeInfo.year = dataDate / 10000;
        timeInfo.month = (dataDate % 10000) / 100;
        timeInfo.day = dataDate % 100;
    } else {
        // Try individual date components
        long year = 0, month = 0, day = 0;
        if (codes_get_long(h, "year", &year) == 0) {
            timeInfo.year = static_cast<unsigned int>(year);
        }
        if (codes_get_long(h, "month", &month) == 0) {
            timeInfo.month = static_cast<unsigned int>(month);
        }
        if (codes_get_long(h, "day", &day) == 0) {
            timeInfo.day = static_cast<unsigned int>(day);
        }
    }

    // Extract the time information
    long dataTime;
    if (codes_get_long(h, "dataTime", &dataTime) == 0) {
        timeInfo.hour = dataTime / 100;
    } else {
        long hour = 0;
        if (codes_get_long(h, "hour", &hour) == 0) {
            timeInfo.hour = static_cast<unsigned int>(hour);
        }
    }

    timeInfo.timezone = Timezone::UTC;

    return timeInfo;
}

EnsembleInfo GribFile::extractEnsembleInfo(codes_handle* h) const {
    EnsembleInfo ensInfo;

    // Extract ensemble member number
    long memberNumber = 0;

    CODES_CHECK(codes_get_long(h, "perturbationNumber", &memberNumber), 0);
    ensInfo.memberNumber = static_cast<unsigned int>(memberNumber);

    TimeInfo initTime;

    // Get date components
    long year = 0, month = 0, day = 0, hour = 0;

    CODES_CHECK(codes_get_long(h, "year", &year), 0);
    if (year < 100) {
        initTime.year = static_cast<unsigned int>(year + 2000);
    } else {
        initTime.year = static_cast<unsigned int>(year);
    }

    CODES_CHECK(codes_get_long(h, "month", &month), 0);
    initTime.month = static_cast<unsigned int>(month);

    CODES_CHECK(codes_get_long(h, "day", &day), 0);
    initTime.day = static_cast<unsigned int>(day);

    CODES_CHECK(codes_get_long(h, "hour", &hour), 0);
    initTime.hour = static_cast<unsigned int>(hour);

    initTime.timezone = Timezone::UTC;

    ensInfo.initTime = initTime;

    return ensInfo;
}

Variable GribFile::extractVariable(codes_handle* h) const {
    long varCode = 0;
    CODES_CHECK(codes_get_long(h, "indicatorOfParameter", &varCode), 0);
    Variable variable = variable_from_code(varCode);
    return variable;
}

Center GribFile::extractCenter(codes_handle* h) const {
    long centerCode = 0;
    CODES_CHECK(codes_get_long(h, "centre", &centerCode), 0);
    Center center = center_from_code(centerCode);
    return center;
}

void GribFile::extractData(codes_handle* h, size_t numValues, std::vector<double>& data) const {
    if (!h) {
        throw std::runtime_error("Invalid GRIB handle");
    }

    data.resize(numValues);

    // Extract the values 
    int err = codes_get_double_array(h, "values", data.data(), &numValues);
    if (err != CODES_SUCCESS) {
        throw std::runtime_error("Failed to get data from GRIB file.");
    }

    // Check for and handle missing values
    double missingValue = 0.0;
    if (codes_is_defined(h, "missingValue") == 1) {
        err = codes_get_double(h, "missingValue", &missingValue);
        if (err == CODES_SUCCESS) {
            // Replace missing values with NaN
            for (size_t i = 0; i < numValues; i++) {
                if (data[i] == missingValue) {
                    data[i] = std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
    }
}

std::shared_ptr<GribMessage> GribFile::loadMessage(size_t index) const noexcept {
    if (index >= messageOffsets_.size()) {
        std::cerr << "Message index is out of bounds." << std::endl;
        return nullptr;
    }

    FileGuard fileGuard(filepath_);
    FILE* file = fileGuard.get();

    off_t messageOff = messageOffsets_[index];

    try {
        if (fseek(file, messageOff, SEEK_SET) != 0) {
            std::cerr << "Failed to seek to message offset: " << messageOff << std::endl;
            return nullptr;
        }
        int err = 0;
        CodesHandleGuard handleGuard(fileGuard);
        codes_handle* h = handleGuard.get();

        // Extract time and ensemble information.
        TimeInfo timeInfo = extractTimeInfo(h);
        EnsembleInfo ensInfo = extractEnsembleInfo(h);
        CoordinateSystem coords = extractCoordinateSystem(h);
        Variable variable = extractVariable(h);
        Center center = extractCenter(h);

        std::vector<double> data;
        extractData(h, metadata_.coordinates.numPoints(), data);
        std::vector<float> dataFloat(data.begin(), data.end());

        return std::make_shared<GribMessage>(
            timeInfo, ensInfo,
            dataFloat, coords,
            variable, center
        );

    } catch (const std::exception& e) {
        std::cerr << "Error creating handle from GRIB file: " << e.what() << std::endl;
        return nullptr;
    }
}

CoordinateSystem GribFile::buildMergedSystem(FILE* file, const std::vector<off_t>& messageOffsets) const {
    int err = 0;

    CoordinateSystem fullCoords;
    for (const auto offset : messageOffsets) {
        if (fseek(file, offset, SEEK_SET) != 0) {
            throw std::runtime_error("Failed to seek to offset in GRIB file: " + filepath_);
        }
        codes_handle* h = codes_handle_new_from_file(nullptr, file, PRODUCT_GRIB, &err);
        if (!h) {
            throw std::runtime_error("Failed to create handle from GRIB file: " + filepath_);
        }
        auto handleGuard = [](codes_handle* handle) {
            if (handle) codes_handle_delete(handle);
        };
        std::unique_ptr<codes_handle, decltype(handleGuard)> handleCleanup(h, handleGuard);

        if (err != CODES_SUCCESS) {
            throw std::runtime_error("Error reading GRIB message: " + std::to_string(err));
        }

        // Extract coordinates
        CoordinateSystem coords = extractCoordinateSystem(h);
        if (!coords.getGrid()) {
            throw std::runtime_error("Failed to extract coordinates from GRIB message at offset: " + std::to_string(offset));
        }
        fullCoords = fullCoords.combine(coords);
    }
    
    return fullCoords;
}

void GribFile::toNetCDF(const std::string& outputPath, size_t batchSize, size_t numThreads) const {
    std::cout << "Converting" << std::endl;

    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
    }

    ThreadPool pool(numThreads);

    // Create NetCDF with proper dimensions
    netCDF::NcFile ncFile(outputPath, netCDF::NcFile::replace);

    // Create dimensions based on metadata
    const CoordinateSystem& coords = metadata_.coordinates;
    if (!coords.getGrid()) {
        throw std::runtime_error("Error: Coordinate grid is null.");
    }
    auto [longitudes, latitudes] = coords.getGrid()->getSplitPoints();
    auto latDim = ncFile.addDim("latitude", latitudes.size());
    auto lonDim = ncFile.addDim("longitude", longitudes.size());
    auto timeDim = ncFile.addDim("pred_time", metadata_.times.size());
    auto memberDim = ncFile.addDim("ensemble", metadata_.ensembles.size());
    auto centerDim = ncFile.addDim("center", metadata_.centers.size());

    // Add coordinate variables
    auto latVar = ncFile.addVar("latitude", netCDF::NcType::nc_FLOAT, {latDim});
    auto lonVar = ncFile.addVar("longitude", netCDF::NcType::nc_FLOAT, {lonDim});
    auto timeVar = ncFile.addVar("pred_time", netCDF::NcType::nc_FLOAT, {timeDim});
    auto memberVar = ncFile.addVar("ensemble", netCDF::NcType::nc_UINT, {memberDim});
    auto centerVar = ncFile.addVar("center", netCDF::NcType::nc_STRING, {centerDim});

    // Write coordinate values.
    latVar.putVar(latitudes.data());
    lonVar.putVar(longitudes.data());

    std::unordered_map<float, size_t> timeIndexMap;
    std::vector<float> timeValues;
    timeValues.reserve(metadata_.times.size());
    timeValues.reserve(metadata_.times.size());
    for (const auto& time : metadata_.times) {
        timeValues.push_back(time.toFloat());
    }
    std::sort(timeValues.begin(), timeValues.end());
    size_t timeIndex = 0;
    for (const auto& time : timeValues) {
        timeIndexMap[time] = timeIndex++;
    }
    timeVar.putVar(timeValues.data());
    timeVar.putAtt("units", "hours since 1970-01-01 00:00:00 UTC");

    std::vector<EnsembleInfo> sortedEnsembles(metadata_.ensembles.begin(), metadata_.ensembles.end());
    std::sort(sortedEnsembles.begin(), sortedEnsembles.end(),
        [](const EnsembleInfo& ens1, const EnsembleInfo& ens2){
            return ens1.initTime < ens2.initTime;}
    );
    std::vector<unsigned int> memberValues(sortedEnsembles.size());
    std::unordered_map<unsigned int, size_t> memberIndexMap;
    size_t memberIndex = 0;
    for (const auto& ens : sortedEnsembles) {
        unsigned int memberNumber = memberIndex + 1; // 1-based index
        memberIndexMap[memberNumber] = memberIndex;
        memberValues[memberIndex++] = memberNumber;
    }
    std::sort(memberValues.begin(), memberValues.end());
    memberVar.putVar(memberValues.data());

    std::vector<std::string> sortedCenters;
    std::unordered_map<std::string, size_t> centerIndexMap;
    sortedCenters.reserve(metadata_.centers.size());
    for (const auto& center : metadata_.centers) {
        sortedCenters.push_back(center_as_string(center));
    }
    std::sort(sortedCenters.begin(), sortedCenters.end());
    size_t centerIndex = 0;
    for (const auto& center : sortedCenters) {
        centerIndexMap[center] = centerIndex++;
    }
    centerVar.putVar(sortedCenters.data()); 

    // Create a variable for the initialization time of the ensemble members.
    auto initTimeVar = ncFile.addVar("init_time", netCDF::NcType::nc_FLOAT, {memberDim});
    std::vector<float> initTimes;
    initTimes.reserve(sortedEnsembles.size());
    for (const auto& ens : sortedEnsembles) {
        initTimes.push_back(ens.initTime.toFloat());
    }
    std::cout << "Writing initialization times: " << initTimes.size() << std::endl;
    initTimeVar.putVar(initTimes.data());
    initTimeVar.putAtt("units", "hours since 1970-01-01 00:00:00 UTC");

    // Now to write the data variables.
    std::unordered_map<Variable, netCDF::NcVar> dataVars;
    for (const auto& var : metadata_.variables) {
        std::vector<netCDF::NcDim> dims = {timeDim, memberDim, latDim, lonDim};
        auto ncVar = ncFile.addVar(variable_as_string(var), netCDF::NcType::nc_FLOAT, dims);
        ncVar.putAtt("units", units(var));
        dataVars[var] = ncVar;
    }

    // Use a mutex to protect access to the NetCDF file.
    std::mutex ncMutex;

    // // Process variables in batches.
    // for (const Variable& var : metadata_.variables) {
    //     std::vector<DimensionKey> keys;
    //     for (const auto& [key, fileLocs] : dimensionMessages_) {
    //         if (std::get<2>(key) == var) {
    //             keys.push_back(key);
    //         }
    //     }

    //     if (keys.empty()) {
    //         throw std::runtime_error("No messages found for variable: " + variable_as_string(var));
    //     }

    //     // Process each batch of messages for this variable.
    //     for (size_t i = 0; i < keys.size(); i+= batchSize) {
    //         size_t end = std::min(i + batchSize, keys.size());
    //         std::vector<DimensionKey> batch(keys.begin() + i, keys.begin() + end);

    //         // Submit to thread pool.
    //         pool.enqueue([this, &dataVars, &ncMutex, &timeIndexMap, &memberIndexMap, &centerIndexMap, var, batch]() {
    //             for (size_t idx : batch) {
    //                 std::shared_ptr<GribMessage> message = this->loadMessage(idx);
    //             }
    //         });
    //     }
    // }

}

bool GribFile::validateGridConsistency() const {
    switch (validationStatus_) {
        case ValidationStatus::NOT_VALIDATED: {
            size_t messageCount = getMessageCount();

            if (messageCount <= 10) {
                // Perform sequential validation
                bool result = performSequentialValidation(filepath_);
                validationStatus_ = result ? ValidationStatus::PASSED : ValidationStatus::FAILED;
                return result;
            }

            // For larger files, use parallel validation.
            bool result = performParallelValidation(filepath_);
            validationStatus_ = result ? ValidationStatus::PASSED : ValidationStatus::FAILED;
            return result;
        }
            break;
        default: {
            // Already validated, return the cached result.
            return validationStatus_ == ValidationStatus::PASSED;
        }     
    }
}

bool GribFile::isValid() const {
    return validationStatus_ == ValidationStatus::PASSED;
}

bool GribFile::performSequentialValidation(const std::string& filepath) const {
    // Open the GRIB file using eccodes.
    FILE* file = fopen(filepath_.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Failed to open GRIB file: " + filepath_);
    }

    auto fileGuard = [](FILE* file) { 
        if (file) fclose(file); 
    };
    std::unique_ptr<FILE, decltype(fileGuard)> fileCleanup(file, fileGuard);

    std::unordered_set<size_t> gridHashes;
    for (const auto [key, fileLocs] : dimensionMessages_) {
        auto [timeInfo, memberNumber, variable, center] = key;
        CoordinateSystem fullCoords = buildMergedSystem(file, fileLocs);
        gridHashes.insert(fullCoords.getGrid()->hash());
    }

    return gridHashes.size() == 1; // All hashes should be the same for a consistent grid
}

bool GribFile::performParallelValidation(const std::string& filepath) const {
    std::unordered_set<size_t> gridHashes;
    std::mutex hashMutex;
    std::mutex exceptionMutex;
    std::vector<std::exception_ptr> exceptions;
    std::vector<std::future<size_t>> futures;

    const unsigned int numThreads = std::min(
        std::thread::hardware_concurrency(),
        static_cast<unsigned int>(8)
    );
    if (numThreads == 0) {
        throw std::runtime_error("Failed to determine number of hardware threads.");
    }
    
    // Parallelize using threads
    ThreadPool threadPool(numThreads);
    std::atomic<bool> hadError{false};
    for (const auto& mapItem : dimensionMessages_) {
        futures.push_back(threadPool.enqueue([this, &filepath, &mapItem, &hadError]() -> size_t {
            try {
                const auto& fileLocs = mapItem.second;
                if (fileLocs.empty()) {
                    return 0;
                }

                FILE* file = fopen(filepath.c_str(), "rb");
                if (!file) {
                    hadError = true;
                    throw std::runtime_error("Failed to open GRIB file: " + filepath);
                }

                auto fileGuard = [](FILE* file) {
                    if (file) fclose(file);
                };
                std::unique_ptr<FILE, decltype(fileGuard)> fileCleanup(file, fileGuard);

                CoordinateSystem fullCoords = buildMergedSystem(file, fileLocs);
                return fullCoords.getGrid()->hash();
            } catch (const std::exception& e) {
                hadError = true;
                std::cerr << "Exception in thread: " << e.what() << std::endl;
                throw;
            }
        }));

        if (hadError) {
            std::cerr << "Error detected, stopping all threads..." << std::endl;
            threadPool.forceStop();
            return false;
        }
    }

    try {
        threadPool.waitForCompletion();
    } catch (const std::exception& e) {
        std::cerr << "Error waiting for thread completion: " << e.what() << std::endl;
        threadPool.forceStop();
        return false;
    }

    for (auto& future : futures) {
        if (future.valid()) {
            try {
                size_t hash = future.get();
                if (hash != 0) {
                    std::lock_guard<std::mutex> lock(hashMutex);
                    gridHashes.insert(hash);
                }
            } catch (const std::exception& e) {
                std::cerr << "Task failed: " << e.what() << std::endl;
                return false;
            }
        }
    }

    return gridHashes.size() == 1; // All hashes should be the same for a consistent grid
}