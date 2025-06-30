#include <CoordinateSystem.hpp>

CoordinateSystem::LatLon::LatLon(
    const std::vector<float>& lats,
    const std::vector<float>& lons
): latitudes(lats), longitudes(lons) {}

bool CoordinateSystem::LatLon::operator==(const LatLon& other) const {
    return latitudes == other.latitudes &&
        longitudes == other.longitudes;
}

bool CoordinateSystem::LatLon::operator!=(const LatLon& other) const {
    return !(*this == other);
}

CoordinateSystem::CoordinateSystem(const LatLon& coords) {
    coordinates_ = coords;
}

CoordinateSystem::CoordinateSystem(const CoordinateSystem& other)
    : coordinates_(other.coordinates_) {}

CoordinateSystem::CoordinateSystem(CoordinateSystem&& other) noexcept
    : coordinates_(std::move(other.coordinates_)) {}

CoordinateSystem::CoordinateSystem(
    const std::vector<float>& latitudes,
    const std::vector<float>& longitudes
) {
    LatLon latlon(latitudes, longitudes);
    coordinates_ = std::move(latlon);
}

CoordinateSystem& CoordinateSystem::operator=(const CoordinateSystem& other) {
    if (this != &other) {
        coordinates_ = other.coordinates_;
    }
    return *this;
}

CoordinateSystem& CoordinateSystem::operator=(CoordinateSystem&& other) noexcept {
    if (this != &other) {
        coordinates_ = std::move(other.coordinates_);
    }
    return *this;
}

bool CoordinateSystem::operator==(const CoordinateSystem& other) const {
    return coordinates_ == other.coordinates_;
}

bool CoordinateSystem::operator!=(const CoordinateSystem& other) const {
    return !(*this == other);
}