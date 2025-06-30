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

#endif // ENSEMBLE_INFO_HPP