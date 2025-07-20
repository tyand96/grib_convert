#ifndef ENSEMBLE_INFO_HPP
#define ENSEMBLE_INFO_HPP

#include "./TimeInfo.hpp"

struct EnsembleInfo{
    unsigned int memberNumber;
    TimeInfo initTime;

    // Default Constructor
    EnsembleInfo() : memberNumber(0), initTime() {}
    EnsembleInfo(
        unsigned int m, TimeInfo initTime
    ): memberNumber(m), initTime(initTime) {}

    bool operator==(const EnsembleInfo& other) const;
    bool operator!=(const EnsembleInfo& other) const;

};

namespace std {
    template<>
    struct hash<EnsembleInfo> {
        size_t operator()(const EnsembleInfo& e) const {
            size_t h1 = std::hash<unsigned int>{}(e.memberNumber);
            size_t h2 = std::hash<TimeInfo>{}(e.initTime);

            return h1 ^ (h2 << 1);
        }
    };
}

#endif // ENSEMBLE_INFO_HPP