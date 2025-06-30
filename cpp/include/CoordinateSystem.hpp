#ifndef COORDINATE_SYSTEM_HPP
#define COORDINATE_SYSTEM_HPP

#include <vector>

class CoordinateSystem {
public:
    struct LatLon {
        std::vector<float> latitudes;
        std::vector<float> longitudes;

        LatLon() = default;
        LatLon(const std::vector<float>& lats, const std::vector<float>& lons);

        bool operator==(const LatLon& other) const;
        bool operator!=(const LatLon& other) const;
    };

    CoordinateSystem() = default;
    CoordinateSystem(const std::vector<float>& latitudes, const std::vector<float>& longitudes);
    CoordinateSystem(const LatLon& coords);
    CoordinateSystem(const CoordinateSystem& other);
    CoordinateSystem(CoordinateSystem&& other) noexcept;
    CoordinateSystem& operator=(const CoordinateSystem& other);
    CoordinateSystem& operator=(CoordinateSystem&& other) noexcept;
    ~CoordinateSystem() = default;

    bool operator==(const CoordinateSystem& other) const;
    bool operator!=(const CoordinateSystem& other) const;

    LatLon getCoords() const { return coordinates_; };

private:
    LatLon coordinates_;


};

#endif // COORDINATE_SYSTEM_HPP