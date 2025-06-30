#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <GribMessage.hpp>
#include <stdexcept>

TEST_CASE("GribMessage Construction", "[GribMessage]") {

    SECTION("Full Construction") {
        TimeInfo forecastTime(2020, 8);
        TimeInfo ensInitTime(2020, 7, 30);
        EnsembleInfo ens(1, ensInitTime);
        std::vector<float> data = {1, 2, 3};
        CoordinateSystem coords(
            std::vector<float>{-90, 90},
            std::vector<float>{0, 360}
        );
        GribMessage gm(forecastTime, ens, data, coords, Variable::M10, Center::NCEP);

        REQUIRE(gm.getData() == data);
        REQUIRE(gm.getForecastTime() == forecastTime);
        REQUIRE(gm.getEnsembleInfo() == ens);
        REQUIRE(gm.getCoordinateSystem() == coords);
        REQUIRE(gm.getVariable() == Variable::M10);
        REQUIRE(gm.getCenter() == Center::NCEP);
    }

    SECTION("Copy Construction") {
        TimeInfo forecastTime(2020, 8);
        TimeInfo ensInitTime(2020, 7, 30);
        EnsembleInfo ens(1, ensInitTime);
        std::vector<float> data = {1, 2, 3};
        CoordinateSystem coords(
            std::vector<float>{-90, 90},
            std::vector<float>{0, 360}
        );
        GribMessage gm(forecastTime, ens, data, coords, Variable::M10, Center::NCEP);

        GribMessage copy(gm);

        REQUIRE(copy.getData() == data);
        REQUIRE(copy.getForecastTime() == forecastTime);
        REQUIRE(copy.getEnsembleInfo() == ens);
        REQUIRE(copy.getCoordinateSystem() == coords);
        REQUIRE(copy.getVariable() == Variable::M10);
        REQUIRE(copy.getCenter() == Center::NCEP);
    }

    SECTION("Move Construction") {
        TimeInfo forecastTime(2020, 8);
        TimeInfo ensInitTime(2020, 7, 30);
        EnsembleInfo ens(1, ensInitTime);
        std::vector<float> data = {1, 2, 3};
        CoordinateSystem coords(
            std::vector<float>{-90, 90},
            std::vector<float>{0, 360}
        );
        GribMessage gm(forecastTime, ens, data, coords, Variable::M10, Center::NCEP);

        GribMessage moved = std::move(gm);

        REQUIRE(moved.getData() == data);
        REQUIRE(moved.getForecastTime() == forecastTime);
        REQUIRE(moved.getEnsembleInfo() == ens);
        REQUIRE(moved.getCoordinateSystem() == coords);
        REQUIRE(moved.getVariable() == Variable::M10);
        REQUIRE(moved.getCenter() == Center::NCEP);
    }
}