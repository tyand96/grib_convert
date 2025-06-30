#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <CoordinateSystem.hpp>
#include <stdexcept>

TEST_CASE("LatLon Creation", "[CoordinateSystem]") {
    SECTION("Default Constructor") {
        CoordinateSystem::LatLon latlon;
        REQUIRE(latlon.latitudes.size() == 0);
        REQUIRE(latlon.longitudes.size() == 0);
    }

    SECTION("Constructor with lat/lons") {
        std::vector<float> lats = {1, 2, 3};
        std::vector<float> lons = {4, 5, 6};
        CoordinateSystem::LatLon latlons(lats, lons);
        
        REQUIRE(latlons.latitudes == lats);
        REQUIRE(latlons.longitudes == lons);
    }

    SECTION("Copy") {
        std::vector<float> lats = {1, 2, 3};
        std::vector<float> lons = {4, 5, 6};
        CoordinateSystem::LatLon latlons1(lats, lons);
        auto latlons2 = latlons1;
        
        REQUIRE(latlons1.latitudes == latlons2.latitudes);
        REQUIRE(latlons1.longitudes == latlons2.longitudes);
    }
}

TEST_CASE("Coordinate System Construction", "[CoordinateSystem]") {
    SECTION("Default") {
        CoordinateSystem coords;
        CoordinateSystem::LatLon latlons;

        REQUIRE(coords.getCoords() == latlons);
    }

    SECTION("Constructor with LatLon") {
        CoordinateSystem::LatLon latlons(
            std::vector<float>{1, 2, 3},
            std::vector<float>{4, 5, 6}
        );
        CoordinateSystem coords(latlons);

        REQUIRE(coords.getCoords() == latlons);
    }

    SECTION("Constructor with vectors") {
        std::vector<float> lats = {1, 2, 3};
        std::vector<float> lons = {4, 5, 6};
        CoordinateSystem coords(lats, lons);

        auto latlons = coords.getCoords();

        REQUIRE(latlons.latitudes == lats);
        REQUIRE(latlons.longitudes == lons);
    }

    SECTION("Copy") {
        CoordinateSystem coords(
            std::vector<float>{1,2,3},
            std::vector<float>{4,5,6}
        );

        auto coords2 = coords;

        REQUIRE(coords == coords2);
    }

    SECTION("Move") {
        std::vector<float> lats = {1,2,3};
        std::vector<float> lons = {4,5,6};

        CoordinateSystem coords(lats, lons);

        CoordinateSystem coords2 = std::move(coords);

        auto latlons = coords2.getCoords();

        REQUIRE(latlons.latitudes == lats);
        REQUIRE(latlons.longitudes == lons);
    }
}

TEST_CASE("Comparisons") {
    SECTION("Equality") {
        CoordinateSystem coords1(
            std::vector<float>{1,2,3},
            std::vector<float>{4,5,6}
        );

        CoordinateSystem coords2(
            std::vector<float>{1,2,3},
            std::vector<float>{4,5,6}
        );

        REQUIRE(coords1 == coords2);
    }

    SECTION("Inequality") {
        CoordinateSystem coords1(
            std::vector<float>{1,2,3},
            std::vector<float>{4,5,6}
        );

        CoordinateSystem coords2(
            std::vector<float>{1,2,7},
            std::vector<float>{4,5,6}
        );

        REQUIRE(coords1 != coords2);
    }
}