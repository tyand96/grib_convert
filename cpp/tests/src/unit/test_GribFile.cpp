#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <stdexcept>
#include <GribFile.hpp>

TEST_CASE("Constructors", "[GribFile]") {
    SECTION("Standard Constructor") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        GribFile gf(filePath);

        REQUIRE(gf.getFilePath() == filePath);
    }

    SECTION("Copy Constructor") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        GribFile gf(filePath);

        GribFile copy(gf);

        REQUIRE(copy.getFilePath() == gf.getFilePath());

        auto copy2 = copy;

        REQUIRE(copy2 == gf);
    }

    SECTION("Move Constructor") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        GribFile gf(filePath);

        GribFile moved = std::move(gf);

        REQUIRE(moved.getFilePath() == filePath);
    }
}