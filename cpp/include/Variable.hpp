#ifndef VARIABLE_HPP
#define VARIABLE_HPP

#include <string>

enum class Variable {
    M10,
    T2
};

Variable variable_from_name(const std::string& varName);
Variable variable_from_code(const unsigned int varCode);
std::string variable_as_string(Variable variable);

inline std::string units(Variable variable) {
    switch (variable) {
        case Variable::M10:
            return "m/s";
        case Variable::T2:
            return "°C";
        default:
            throw std::invalid_argument("Unknown variable type.");
    }
}


#endif // VARIABLE_HPP