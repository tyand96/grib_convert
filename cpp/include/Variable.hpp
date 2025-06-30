#ifndef VARIABLE_HPP
#define VARIABLE_HPP

#include <string>

enum class Variable {
    M10,
    T2
};

Variable variable_from_name(const std::string& varName);
Variable variable_from_code(const unsigned int varCode);


#endif // VARIABLE_HPP