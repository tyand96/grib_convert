#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <TimeInfo.hpp>
#include <stdexcept>

TEST_CASE("TimeInfo Creation", "[TimeInfo]") {
    SECTION("Fully Formed Creation") {
        TimeInfo ti(1996, 4, 23, 5, Timezone::GMT);
        REQUIRE(ti.year == 1996);
        REQUIRE(ti.month == 4);
        REQUIRE(ti.day == 23);
        REQUIRE(ti.hour == 5);
        REQUIRE(ti.timezone == Timezone::GMT);
    }

    SECTION("Default Creation") {
        TimeInfo ti;
        REQUIRE(ti.year == 0);
        REQUIRE(ti.month == 0);
        REQUIRE(ti.day == 0);
        REQUIRE(ti.hour == 0);
        REQUIRE(ti.timezone == Timezone::UTC);
    }

    SECTION("Partial Creation Args") {
        TimeInfo ti(2012, 4);
        REQUIRE(ti.year == 2012);
        REQUIRE(ti.month == 4);
        REQUIRE(ti.day == 1);
        REQUIRE(ti.hour == 0);
        REQUIRE(ti.timezone == Timezone::UTC);
    }
}

TEST_CASE("TimeInfo Comparisons", "[TimeInfo]") {
    SECTION("Valid Equality") {
        TimeInfo ti1(0, 0, 0, 0, Timezone::UTC);
        TimeInfo ti2;
        REQUIRE(ti1 == ti2);
    }

    SECTION("Invalid Equality") {
        TimeInfo ti1(1901, 4);
        TimeInfo ti2(2025, 4);
        REQUIRE(!(ti1 == ti2));
    }

    SECTION("Valid Inequality") {
        TimeInfo ti1(1900, 0, 0, 0, Timezone::GMT);
        TimeInfo ti2(1900, 0, 0, 0);
        REQUIRE(ti1 != ti2);
    }
}

TEST_CASE("Timezone String", "[TimeInfo]") {
    auto [tz, str] = GENERATE(
        std::make_pair(Timezone::UTC, "UTC"),
        std::make_pair(Timezone::GMT, "GMT")
    );

    SECTION("Timezone String") {
        std::string tz_str = timezone_to_string(tz);
        REQUIRE(tz_str == str);
    }
}

TEST_CASE("TimeInfo String", "[TimeInfo]") {
    SECTION("Generating correct time string.") {
        TimeInfo ti(1996, 4, 23, 5, Timezone::GMT);
        std::string expected = "1996-04-23 05:00 GMT";
        REQUIRE(ti.to_string() == expected);
    }
}