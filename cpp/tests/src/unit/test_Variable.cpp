#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <Variable.hpp>
#include <stdexcept>

#include <string>

TEST_CASE("Variable Creation", "[Variable]") {
    SECTION("Creation from valid name.") {
        std::string name = "10m_wind_speed";
        Variable var = variable_from_name(name);
        REQUIRE(var == Variable::M10);
    }

    SECTION("Creation from invalid name.") {
        std::string invalidName = "invalid_name";
        REQUIRE_THROWS_AS(variable_from_name(invalidName), std::invalid_argument);
    }

    SECTION("Creation from valid code.") {
        unsigned int varCode = 167;
        Variable var = variable_from_code(varCode);
        REQUIRE(var == Variable::T2);
    }

    SECTION("Creation from invalid code.") {
        unsigned int invalidCode = 999;
        REQUIRE_THROWS_AS(variable_from_code(invalidCode), std::invalid_argument);
    }
}

TEST_CASE("Parameterized variable creation from name", "[Variable]") {
    auto [name, expected] = GENERATE(
        std::make_pair("10m_wind_speed", Variable::M10),
        std::make_pair("2m_temperature", Variable::T2)
    );

    SECTION("Test variable creation") {
        Variable var = variable_from_name(name);
        REQUIRE(var == expected);
    }
}

TEST_CASE("Parameterized variable creation from code", "[Variable]") {
    auto [code, expected] = GENERATE(
        std::make_pair(207u, Variable::M10),
        std::make_pair(167u, Variable::T2)
    );

    SECTION("Test variable creation") {
        Variable var = variable_from_code(code);
        REQUIRE(var == expected);
    }
}