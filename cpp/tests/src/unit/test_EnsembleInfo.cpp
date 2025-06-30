#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <EnsembleInfo.hpp>
#include <stdexcept>

TEST_CASE("EnsembleInfo Creation", "[EnsembleInfo]") {
    SECTION("Default Constructor") {
        EnsembleInfo ensInf;
        TimeInfo ti;
        REQUIRE(ensInf.memberNumber == 0);
        REQUIRE(ensInf.initTime == ti);
    }

    SECTION("Specific Constructor") {
        TimeInfo ti(2005, 6);
        EnsembleInfo ens(5, ti);

        REQUIRE(ens.memberNumber == 5);
        REQUIRE(ens.initTime.year == 2005);
        REQUIRE(ens.initTime == ti);
    }
}

TEST_CASE("EnsembleInfo Equality", "[EnsembleInfo]") {
    SECTION("Valid Equality") {
        TimeInfo ti(2004, 7);
        EnsembleInfo ens1(6, ti);
        EnsembleInfo ens2(6, ti);

        REQUIRE(ens1 == ens2);
    }

    SECTION("Valid Inequality") {
        TimeInfo ti(2006, 8);
        EnsembleInfo ens1(4, ti);
        EnsembleInfo ens2(5, ti);

        REQUIRE(ens1 != ens2);
    }
}