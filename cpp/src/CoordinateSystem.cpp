#include <CoordinateSystem.hpp>
#include <stdexcept>
#include <unordered_map>
#include <set>

bool Point::operator==(const Point& other) const {
    return latitude == other.latitude &&
            longitude == other.longitude;
}

bool Point::operator!=(const Point& other) const {
    return !(*this == other);
}

bool CoordinateSystem::Grid::isPointValid(float latitude, float longitude) const {
    return points.find({latitude, longitude}) != points.end();
}

bool CoordinateSystem::Grid::isPointValid(const Point& point) const {
    return points.find(point) != points.end();
}

bool CoordinateSystem::Grid::contains(const Grid& other) const {
    for (const auto& point : other.points) {
        if (!isPointValid(point)) {
            return false;
        }
    }
    return true;
}

bool CoordinateSystem::Grid::overlaps(const Grid& other) const {
    for (const auto& point : other.points) {
        if (isPointValid(point)) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<CoordinateSystem::Grid> CoordinateSystem::Grid::clone() const {
    auto clonedGrid = std::make_unique<Grid>();
    clonedGrid->points = points; // Copy the points.
    return clonedGrid;
}

CoordinateSystem::Grid CoordinateSystem::Grid::createRegularGrid(
    const std::unordered_set<float>& latitudes,
    const std::unordered_set<float>& longitudes
) {
    Grid grid;
    for (const auto& lat : latitudes) {
        for (const auto& lon : longitudes) {
            grid.points.insert({lat, lon});
        }
    }
    return grid;
}

CoordinateSystem::CoordinateSystem()
    : grid_(nullptr),
        gridType_(GridType::UNDEFINED) {}

CoordinateSystem::CoordinateSystem(std::unique_ptr<Grid> grid, GridType gridType) {
    if (!grid) {
        throw std::invalid_argument("Grid cannot be null.");
    }

    if (gridType == GridType::UNDEFINED) {
        throw std::invalid_argument("Grid type cannot be UNDEFINED.");
    }

    grid_ = std::move(grid);
    gridType_ = gridType;
}

CoordinateSystem CoordinateSystem::createRegularGrid(
    const std::unordered_set<float>& latitudes,
    const std::unordered_set<float>& longitudes
) {
    return CoordinateSystem(
        std::make_unique<Grid>(Grid::createRegularGrid(
            latitudes, longitudes
        )),
        GridType::REGULAR_LATLON
    );
}

CoordinateSystem::CoordinateSystem(const CoordinateSystem& other)
    : grid_(other.grid_ ? other.grid_->clone() : nullptr),
        gridType_(other.gridType_)
        {}

CoordinateSystem::CoordinateSystem(CoordinateSystem&& other) noexcept
    : grid_(std::move(other.grid_)),
        gridType_(other.gridType_)
        {}

CoordinateSystem& CoordinateSystem::operator=(const CoordinateSystem& other) {
    if (this != &other) {
        grid_ = other.grid_ ? other.grid_->clone() : nullptr;
        gridType_ = other.gridType_;
    }
    return *this;
}

CoordinateSystem& CoordinateSystem::operator=(CoordinateSystem&& other) noexcept {
    if (this != &other) {
        grid_ = std::move(other.grid_);
        gridType_ = other.gridType_;
        other.gridType_ = GridType::UNDEFINED; // Reset the moved-from object
        other.grid_ = nullptr; // Reset the moved-from object   
    }
    return *this;
}

bool CoordinateSystem::operator==(const CoordinateSystem& other) const {
    return gridType_ == other.gridType_ &&
        ((grid_ && other.grid_) ? (*grid_ == *other.grid_) : (grid_ == other.grid_));
}

bool CoordinateSystem::operator!=(const CoordinateSystem& other) const {
    return !(*this == other);
}

bool CoordinateSystem::overlaps(const CoordinateSystem& other) const {
    if (!grid_ || !other.grid_) {
        return false;
    }
    return grid_->overlaps(*other.grid_);
}

bool CoordinateSystem::contains(const CoordinateSystem& other) const {
    if (!grid_ || !other.grid_) {
        return false;
    }
    return grid_->contains(*other.grid_);
}

CoordinateSystem CoordinateSystem::combine(const CoordinateSystem& other) const {
    auto combinedGrid = grid_->clone();
    if (other.grid_) {
        combinedGrid->points.insert(other.grid_->points.begin(), other.grid_->points.end());
    }
    GridType combinedGridType = (gridType_ == other.gridType_) ? gridType_ : GridType::COMPOSITE;
    return CoordinateSystem(
        std::move(combinedGrid),
        combinedGridType
    );
}

bool CoordinateSystem::isPointValid(float latitude, float longitude) const {
    if (!grid_) {
        return false;
    }
    return grid_->isPointValid(latitude, longitude);
}

bool CoordinateSystem::isPointValid(const Point& point) const {
    if (!grid_) {
        return false;
    }
    return grid_->isPointValid(point);
}

std::string CoordinateSystem::getGridTypeString() const {
    switch (gridType_) {
        case GridType::REGULAR_LATLON:
            return "regular_ll";
        case GridType::COMPOSITE:
            return "composite";
        case GridType::UNDEFINED:
        default:
            return "undefined";
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
