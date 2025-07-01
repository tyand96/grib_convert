#include <CoordinateSystem.hpp>
#include <stdexcept>
#include <unordered_map>

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

CoordinateSystem::CoordinateSystem(const LatLon& coords, GridType gridType) {
    coordinates_ = coords;
    gridType_ = gridType;
}

CoordinateSystem::CoordinateSystem(const CoordinateSystem& other)
    : coordinates_(other.coordinates_) {}

CoordinateSystem::CoordinateSystem(CoordinateSystem&& other) noexcept
    : coordinates_(std::move(other.coordinates_)) {}

CoordinateSystem::CoordinateSystem(
    const std::vector<float>& latitudes,
    const std::vector<float>& longitudes,
    GridType gridType
) {
    LatLon latlon(latitudes, longitudes);
    coordinates_ = std::move(latlon);
    gridType_ = gridType;
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

std::string CoordinateSystem::getGridTypeString() const {
    switch (gridType_) {
        case GridType::REGULAR_LATLON:
            return "regular_ll";
        default:
            throw std::invalid_argument("Invalid grid type.");
    }
}

CoordinateSystem::GridType CoordinateSystem::stringToGridType(const std::string& gridTypeStr) {
    std::unordered_map<std::string, GridType> gridMap = {
        {"regular_ll", GridType::REGULAR_LATLON}
    };

    auto it = gridMap.find(gridTypeStr);
    if (it != gridMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Invalid grid type string: " + gridTypeStr);
}
