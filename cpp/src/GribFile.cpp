#include <GribFile.hpp>
#include <GribMessage.hpp>
#include <ThreadPool.hpp>
#include <stdexcept>
#include <unordered_set>
#include <iostream>
#include <thread>

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

GribFile::GribFile(std::string filepath) {
    filepath_ = filepath;
    loadMetadata();
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

bool GribFile::operator==(const GribFile& other) const {
    return filepath_ == other.filepath_ &&
        metadata_ == other.metadata_;
}

bool GribFile::operator!=(const GribFile& other) const {
    return !(*this == other);
}


void GribFile::loadMetadata() {
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

    // OPen the GRIB file using eccodes.
    int err = 0;
    FILE* file = fopen(filepath_.c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Failed to open GRIB file: " + filepath_);
    }

    auto fileGuard = [](FILE* file) { 
        if (file) fclose(file); 
    };
    std::unique_ptr<FILE, decltype(fileGuard)> fileCleanup(file, fileGuard);

    codes_handle* h = nullptr;
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

        // Extract ensemble information
        EnsembleInfo ensInfo = extractEnsembleInfo(h);

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

    for (const auto c: metadata_.centers) {
        std::cout << center_as_string(c) << std::endl;
    }

    std::cout << std::endl;

    for (const auto v: metadata_.variables) {
        std::cout << variable_as_string(v) << std::endl;
    }

    std::cout << getEstimatedMemorySize() << " bytes estimated memory size." << std::endl;
    std::cout << getMessageCount() << " messages found in file." << std::endl;
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

bool GribFile::validateMessageCompatibility(codes_handle* handle, GribFile::DimensionMap& dimensionCoverage) const {
    if (!handle) {
        return false;
    }

    bool coordsEverywhere = true;

    // Extract dimensions from the current message
    TimeInfo timeInfo = extractTimeInfo(handle);
    EnsembleInfo ensInfo = extractEnsembleInfo(handle);

    long varCode = 0;
    CODES_CHECK(codes_get_long(handle, "indicatorOfParameter", &varCode), 0);
    Variable variable = variable_from_code(varCode);

    long centerCode = 0;
    CODES_CHECK(codes_get_long(handle, "centre", &centerCode), 0);
    Center center = center_from_code(centerCode);

    DimensionKey key = std::make_tuple(timeInfo, ensInfo.memberNumber, variable, center);

    CoordinateSystem coords = extractCoordinateSystem(handle);

    // // Add the coordinates to the dimension coverage map
    // if (dimensionCoverage.find(key) == dimensionCoverage.end()) {
    //     // If this key is not in the map, create a new entry
    //     dimensionCoverage[key] = coords;
    // } else {
    //     dimensionCoverage[key] = dimensionCoverage[key].combine(coords);
    // }

    // for (const auto& keys : dimensionCoverage) {
    //     if (keys.first == key) {
    //         continue; // Skip the same key
    //     }

    //     if (!keys.second.contains(coords)) {
    //         return false;
    //     }
    // }
    return true;
}

// CoordinateSystem::LatLon GribFile::distillLatLon() const {
//     CoordinateSystem::LatLon latLon;

//     // Collect all unique latitudes and longitudes from the dimension coverage
//     std::unordered_set<float> uniqueLats;
//     std::unordered_set<float> uniqueLons;

//     for (const auto& entry : dimensionCoverage_) {
//         for (const auto& coord : entry.second) {
//             uniqueLats.insert(coord.first);
//             uniqueLons.insert(coord.second);
//         }
//     }

//     // Convert to vectors
//     latLon.latitudes.assign(uniqueLats.begin(), uniqueLats.end());
//     latLon.longitudes.assign(uniqueLons.begin(), uniqueLons.end());

//     return latLon;
// }

size_t GribFile::computeGridHash(FILE* file, const std::vector<off_t>& messageOffsets) const {
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
    // Compute a hash of the combined coordinates
    size_t hash = fullCoords.getGrid()->hash();

    return hash;
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
        gridHashes.insert(computeGridHash(file, fileLocs));
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

                return computeGridHash(file, fileLocs);
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