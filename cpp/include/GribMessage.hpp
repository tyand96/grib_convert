#ifndef GRIB_MESSAGE_HPP
#define GRIB_MESSAGE_HPP

#include <vector>
#include <string>
#include <ctime>

#include "./CoordinateSystem.hpp"
#include "./Variable.hpp"
#include "./Center.hpp"
#include "./TimeInfo.hpp"
#include "./EnsembleInfo.hpp"

class GribMessage {
public:

    GribMessage(
        const TimeInfo& timeInfo,
        const EnsembleInfo& ensembleInfo,
        const std::vector<float>& data,
        const CoordinateSystem& coordinates,
        Variable variable,
        Center center
    );

    GribMessage(const GribMessage& other);

    GribMessage(GribMessage&& other) noexcept;

    GribMessage& operator=(const GribMessage& other);
    GribMessage& operator=(GribMessage&& other) noexcept;

    ~GribMessage() = default;
    

    const TimeInfo& getForecastTime() const { return forecastTime_; };
    const EnsembleInfo& getEnsembleInfo() const { return ensembleInfo_; };
    const std::vector<float>& getData() const { return data_; };
    const CoordinateSystem& getCoordinateSystem() const { return coordinates_; };
    const Variable& getVariable() const { return variable_; };
    const Center& getCenter() const { return center_; };


private:
    TimeInfo forecastTime_;
    EnsembleInfo ensembleInfo_;
    std::vector<float> data_;
    CoordinateSystem coordinates_;
    Variable variable_;
    Center center_;
};

#endif // GRIB_MESSAGE_HPP