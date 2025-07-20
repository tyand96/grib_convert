#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <stdexcept>
#include <filesystem>
#include <GribFile.hpp>

#include <iostream>
#include <GribMessage.hpp>

TEST_CASE("Constructors", "[GribFile]") {
    SECTION("Standard Constructor") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        // std::string filePath = "/Users/tyler/Documents/grib_convert/cpp/tests/test_files/10m_wind_speed.ecmwf.2021.01.grib";
        GribFile gf(filePath);

        REQUIRE(gf.getFilePath() == filePath);
        REQUIRE(gf.isValid());

        GribFile::Metadata metadata = gf.getMetadata();
        REQUIRE(metadata.centers == std::set<Center>({Center::ECMWF}));
        REQUIRE(metadata.variables == std::set<Variable>({Variable::M10}));
        REQUIRE(metadata.hasConsistentGrid);
    }

    SECTION("No validation") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        // std::string filePath = "/Users/tyler/Documents/grib_convert/cpp/tests/test_files/10m_wind_speed.ecmwf.2021.01.grib";
        GribFile gf(filePath, false);

        REQUIRE(gf.getFilePath() == filePath);
        REQUIRE_FALSE(gf.isValid());

        GribFile::Metadata metadata = gf.getMetadata();
        REQUIRE(metadata.centers == std::set<Center>({Center::ECMWF}));
        REQUIRE(metadata.variables == std::set<Variable>({Variable::M10}));
        REQUIRE(metadata.hasConsistentGrid);
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

TEST_CASE("NetCDF Conversion", "[GribFile]") {
    SECTION("Burst Monthly Conversion") {
        std::string filePath = "../test_files/10m_wind_speed.ecmwf.2021.01.grib";
        std::string outputPath = "../test_files/10m_wind_speed.ecmwf.2021.01.nc";
        GribFile gf(filePath, false);

        size_t counter = 0;
        for (auto it = gf.begin(); it != gf.end(); ++it) {
            Center center = it->getCenter();
            // std::cout << center_as_string(it->getCenter()) << std::endl;
            counter++;
        }
        std::cout << counter << std::endl;

        // std::shared_ptr<GribMessage> msg = gf.loadMessage(0);

        // REQUIRE(msg);


        
        // gf.toNetCDF(outputPath, 100);

        // // Check if the output file exists
        // REQUIRE(std::filesystem::exists(outputPath));

        // // Delete the output file after test
        // std::filesystem::remove(outputPath);
    }
}