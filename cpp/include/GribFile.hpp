#ifndef GRIB_FILE_HPP
#define GRIB_FILE_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <set>
#include <functional>
#include <list>

#include "./Center.hpp"
#include "./Variable.hpp"
#include "./CoordinateSystem.hpp"

class GribMessage;

class GribFile {
public:
    struct Metadata {
        std::set<Center> centers;
        std::set<Variable> variables;
        size_t totalMessages;
        size_t estimatedMemorySize;
        CoordinateSystem coordinates;
        bool hasConsistentGrid;
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

    GribFile();
    explicit GribFile(const std::string& filepath);
    GribFile(const GribFile& other);
    GribFile(GribFile&& other) noexcept;
    GribFile operator=(const GribFile& other);
    GribFile operator=(GribFile&& other);
    ~GribFile() = default;

    bool operator==(const GribFile& other) const;
    bool operator!=(const GribFile& other) const;

    Iterator begin();
    Iterator end();
    void exportToNetCDF(const std::string& outputPath, size_t batchSize = 100) const;

    const Metadata& getMetadata() const { return metadata_; };
    bool isValid() const;
    size_t getMessageCount() const { return metadata_.totalMessages; };
    std::vector<Variable> getVariables() const;
    CoordinateSystem getCoordinateSystem() const;

private:
    std::string filepath_;
    Metadata metadata_;

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
    bool validateMessageCompatibililty() const;
    std::vector<std::string> getCompatibilityIssues() const;
    std::shared_ptr<GribMessage> loadMessage(size_t index);
    std::vector<size_t> getMessageIndicesByVariable(const Variable& var) const;
    void processMessageBatch(const std::vector<size_t>& indices, std::function<void(const GribMessage&)> processor);
    
};

#endif // GRIB_FILE_HPP