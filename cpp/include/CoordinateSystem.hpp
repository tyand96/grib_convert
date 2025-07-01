#ifndef COORDINATE_SYSTEM_HPP
#define COORDINATE_SYSTEM_HPP

#include <vector>
#include <string>

class CoordinateSystem {
public:
    enum class GridType {
        UNDEFINED,
        REGULAR_LATLON,     // regular_ll
    };

    struct LatLon {
        std::vector<float> latitudes;
        std::vector<float> longitudes;

        LatLon() = default;
        LatLon(const std::vector<float>& lats, const std::vector<float>& lons);

        bool operator==(const LatLon& other) const;
        bool operator!=(const LatLon& other) const;
    };

    CoordinateSystem() : gridType_(GridType::UNDEFINED) {};
    CoordinateSystem(const std::vector<float>& latitudes, const std::vector<float>& longitudes,
                        GridType gridType = GridType::REGULAR_LATLON);
    CoordinateSystem(const LatLon& coords, GridType gridType = GridType::REGULAR_LATLON);
    CoordinateSystem(const CoordinateSystem& other);
    CoordinateSystem(CoordinateSystem&& other) noexcept;
    CoordinateSystem& operator=(const CoordinateSystem& other);
    CoordinateSystem& operator=(CoordinateSystem&& other) noexcept;
    ~CoordinateSystem() = default;

    bool operator==(const CoordinateSystem& other) const;
    bool operator!=(const CoordinateSystem& other) const;

    LatLon getCoords() const { return coordinates_; };
    GridType getGridType() const { return gridType_; };
    std::string getGridTypeString() const;

    static GridType stringToGridType(const std::string& gridTypeStr);

private:
    LatLon coordinates_;
    GridType gridType_;
};

#endif // COORDINATE_SYSTEM_HPP