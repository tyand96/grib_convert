#include <EnsembleInfo.hpp>

bool EnsembleInfo::operator==(const EnsembleInfo& other) const {
    return memberNumber == other.memberNumber &&
        initTime == other.initTime;
}

bool EnsembleInfo::operator!=(const EnsembleInfo& other) const {
    return !(*this == other);
}