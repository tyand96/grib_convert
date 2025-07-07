#ifndef COORDINATE_SYSTEM_HPP
#define COORDINATE_SYSTEM_HPP

#include <vector>
#include <string>
#include <unordered_set>

struct Point {
    float latitude;
    float longitude;

    bool operator==(const Point& other) const;
    bool operator!=(const Point& other) const;
};

namespace std {
    template<>
    struct hash<Point> {
        size_t operator()(const Point& p) const {
            size_t h1 = std::hash<float>{}(p.latitude);
            size_t h2 = std::hash<float>{}(p.longitude);

            // Combine the hash codes
            return h1 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
        }
    };
}

class CoordinateSystem{
public:
    enum class GridType {
        UNDEFINED,
        REGULAR_LATLON,     // regular_ll
        COMPOSITE,           // internal composite
    };

    struct Grid {
        std::unordered_set<Point> points;

        bool operator==(const Grid& other) const { return points == other.points; };
        bool operator!=(const Grid& other) const { return !(*this == other); };

        bool isPointValid(float latitude, float longitude) const;
        bool isPointValid(const Point& point) const;
        bool contains(const Grid& other) const;
        bool overlaps(const Grid& other) const;

        std::unique_ptr<Grid> clone() const;

        static Grid createRegularGrid(
            const std::unordered_set<float>& latitudes,
            const std::unordered_set<float>& longitudes
        );
    };

    // Constructors
    CoordinateSystem();
    CoordinateSystem(std::unique_ptr<Grid> grid, GridType gridType);

    // Regular Grid convenience function.
    static CoordinateSystem createRegularGrid(const std::unordered_set<float>& latitudes, const std::unordered_set<float>& longitudes);

    // Copy and move
    CoordinateSystem(const CoordinateSystem& other);
    CoordinateSystem(CoordinateSystem&& other) noexcept;
    CoordinateSystem& operator=(const CoordinateSystem& other);
    CoordinateSystem& operator=(CoordinateSystem&& other) noexcept;

    ~CoordinateSystem() = default;

    bool operator==(const CoordinateSystem& other) const;
    bool operator!=(const CoordinateSystem& other) const;

    bool overlaps(const CoordinateSystem& other) const;
    bool contains(const CoordinateSystem& other) const;

    CoordinateSystem combine(const CoordinateSystem& other) const;

    GridType getGridType() const { return gridType_; };
    const Grid* getGrid() const { return grid_.get(); };
    bool isPointValid(float latitude, float longitude) const;
    bool isPointValid(const Point& point) const;

    std::string getGridTypeString() const;

    static GridType stringToGridType(const std::string& gridTypeStr);

private:
    std::unique_ptr<Grid> grid_;
    GridType gridType_;
};

#endif // COORDINATE_SYSTEM_HPP