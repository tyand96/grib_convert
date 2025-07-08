#ifndef GRIB_FILE_HPP
#define GRIB_FILE_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <set>
#include <functional>
#include <list>
#include <tuple>
#include <unordered_map>
#include <set>
#include <eccodes.h>

#include "./Center.hpp"
#include "./Variable.hpp"
#include "./CoordinateSystem.hpp"
#include "./TimeInfo.hpp"
#include "./EnsembleInfo.hpp"

class GribMessage;

namespace std {
    template<>
    struct hash<std::tuple<TimeInfo, unsigned int, Variable, Center>> {
        size_t operator()(const std::tuple<TimeInfo, unsigned int, Variable, Center>& key) const {
            const auto& [timeInfo, memberNumber, variable, center] = key;
            size_t h1 = std::hash<TimeInfo>{}(timeInfo);
            size_t h2 = std::hash<unsigned int>{}(memberNumber);
            size_t h3 = std::hash<Variable>{}(variable);
            size_t h4 = std::hash<Center>{}(center);

            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}

enum class ValidationMode {
    AUTO,  // Validate only if not previously validated
    FORCE,  // Always validate, regardless of previous validation
    SKIP    // Skip validation entirely (unsafe, but fast)
};

enum class ValidationStatus {
    NOT_VALIDATED,
    PASSED,
    FAILED
};

class GribFile {
public:
    struct Metadata {
        std::set<Center> centers;
        std::set<Variable> variables;
        size_t totalMessages;
        size_t estimatedMemorySize;
        CoordinateSystem coordinates;
        bool hasConsistentGrid;

        bool operator==(const Metadata& other) const;
        bool operator!=(const Metadata& other) const;
    };

    class Iterator {
    public:
        Iterator& operator++();
        const GribMessage& operator*() const;
        bool operator!=(const Iterator& other) const;
        Iterator operator++(int);
        const GribMessage* operator->() const;
        bool operator==(const Iterator& other) const;
    
    private:
        GribFile* parent_;
        size_t currentIndex_;
        mutable std::shared_ptr<GribMessage> currentMessage_;
    };

    explicit GribFile(std::string filepath);
    GribFile(const GribFile& other);
    GribFile(GribFile&& other) noexcept;
    GribFile operator=(const GribFile& other);
    GribFile operator=(GribFile&& other);
    ~GribFile() = default;

    bool operator==(const GribFile& other) const;
    bool operator!=(const GribFile& other) const;

    Iterator begin();
    Iterator end();
    void toNetCDF(const std::string& outputPath, size_t batchSize = 100) const;

    bool validateGridConsistency() const;

    const std::string& getFilePath() const { return filepath_; };
    const Metadata& getMetadata() const { return metadata_; };
    bool isValid() const;
    size_t getMessageCount() const { return metadata_.totalMessages; };
    unsigned int getEstimatedMemorySize() const { return metadata_.estimatedMemorySize; };
    std::vector<Variable> getVariables() const;
    CoordinateSystem getCoordinateSystem() const;

private:
    std::string filepath_;
    Metadata metadata_;
    mutable ValidationStatus validationStatus_ = ValidationStatus::NOT_VALIDATED;

    struct MessageCache {
        static constexpr size_t MAX_CACHE_SIZE = 10;
        std::unordered_map<size_t, std::shared_ptr<GribMessage>> cache;

        std::list<size_t> lruList;

        void add(size_t index, std::shared_ptr<GribMessage> message);
        std::shared_ptr<GribMessage> get(size_t index);
        bool contains(size_t index) const;


    };
    std::unique_ptr<MessageCache> messageCache_;

    void loadMetadata();
    CoordinateSystem extractCoordinateSystem(codes_handle* h) const;
    CoordinateSystem extractRegularGrid(codes_handle* h) const;
    TimeInfo extractTimeInfo(codes_handle* h) const;
    EnsembleInfo extractEnsembleInfo(codes_handle *h) const;
    std::vector<std::string> getCompatibilityIssues() const;
    std::shared_ptr<GribMessage> loadMessage(size_t index);
    std::vector<size_t> getMessageIndicesByVariable(const Variable& var) const;
    void processMessageBatch(const std::vector<size_t>& indices, std::function<void(const GribMessage&)> processor);

    using DimensionKey = std::tuple<TimeInfo, unsigned int, Variable, Center>;
    using DimensionMap = std::unordered_map<DimensionKey, std::vector<off_t>>;
    DimensionMap dimensionMessages_;
    bool validateMessageCompatibility(codes_handle* handle, DimensionMap& coverage) const;
    size_t computeGridHash(FILE* file, const std::vector<off_t>& messageOffsets) const;
    bool performSequentialValidation(const std::string& filepath) const;
    bool performParallelValidation(const std::string& filepath) const;    
};

#endif // GRIB_FILE_HPP