#include <GribMessage.hpp>
#include <sstream>
#include <iomanip>

GribMessage::GribMessage(
    const TimeInfo& forecastTime,
    const EnsembleInfo& ensembleInfo,
    const std::vector<float>& data,
    const CoordinateSystem& coordinates,
    Variable variable,
    Center center
) : 
    forecastTime_(forecastTime),
    ensembleInfo_(ensembleInfo),
    data_(data),
    coordinates_(coordinates),
    variable_(variable),
    center_(center) {}

GribMessage::GribMessage(const GribMessage& other)
    :
        forecastTime_(other.forecastTime_),
        ensembleInfo_(other.ensembleInfo_),
        data_(other.data_),
        coordinates_(other.coordinates_),
        variable_(other.variable_),
        center_(other.center_) {}

GribMessage::GribMessage(GribMessage&& other) noexcept
    :
        forecastTime_(std::move(other.forecastTime_)),
        ensembleInfo_(std::move(other.ensembleInfo_)),
        data_(std::move(other.data_)),
        coordinates_(std::move(other.coordinates_)),
        variable_(std::move(other.variable_)),
        center_(std::move(other.center_)) {}

GribMessage& GribMessage::operator=(const GribMessage& other) {
    if (this != &other) {
        forecastTime_ = other.forecastTime_;
        ensembleInfo_ = other.ensembleInfo_;
        data_ = other.data_;
        coordinates_ = other.coordinates_;
        variable_ = other.variable_;
        center_ = other.center_;
    }
    return *this;
}

GribMessage& GribMessage::operator=(GribMessage&& other) noexcept {
    if (this != &other) {
        forecastTime_ = std::move(other.forecastTime_);
        ensembleInfo_ = std::move(other.ensembleInfo_);
        data_ = std::move(other.data_);
        coordinates_ = std::move(other.coordinates_);
        variable_ = std::move(other.variable_);
        center_ = std::move(other.center_);
    }
    return *this;
}