#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <Center.hpp>

TEST_CASE("Center Creation", "[Center]") {
    SECTION("Creation from valid code") {
        unsigned int centerCode = 7U;
        Center center = center_from_code(centerCode);
        REQUIRE(center == Center::NCEP);
    }

    SECTION("Creation from invalid code.") {
        unsigned int invalidCode = 999U;
        REQUIRE_THROWS_AS(center_from_code(invalidCode), std::invalid_argument);
    }

    SECTION("Testing regular integer.") {
        int centerCode = 250;
        Center center = center_from_code(centerCode);
        REQUIRE(center == Center::CMCC);
    }

    SECTION("Testing negative integer.") {
        int centerCode = -7;
        REQUIRE_THROWS_AS(center_from_code(centerCode), std::invalid_argument);
    }
}

TEST_CASE("Parameterized center creation from code.", "[Center]") {
    auto [code, expected] = GENERATE(
        std::make_pair(2U, Center::BOM),
        std::make_pair(250U, Center::CMCC),
        std::make_pair(78U, Center::DWD),
        std::make_pair(54U, Center::ECCC),
        std::make_pair(98U, Center::ECMWF),
        std::make_pair(34U, Center::JMA),
        std::make_pair(85U, Center::METEO_FRANCE),
        std::make_pair(7U, Center::NCEP),
        std::make_pair(74U, Center::UKMO)
    );

    SECTION("Test Center Creation") {
        Center center = center_from_code(code);
        REQUIRE(center == expected);
    }
}