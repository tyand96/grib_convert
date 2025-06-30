#include <Variable.hpp>

#include <unordered_map>
#include <stdexcept>

Variable variable_from_name(const std::string& varName) {
    static const std::unordered_map<std::string, Variable> nameMap = {
        {"10m_wind_speed", Variable::M10},
        {"2m_temperature", Variable::T2}
    };

    auto it = nameMap.find(varName);
    if (it != nameMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Uknown variable name: " + varName);
}

Variable variable_from_code(const unsigned int varCode) {
    static const std::unordered_map<unsigned int, Variable> codeMap = {
        {207, Variable::M10},
        {167, Variable::T2}
    };

    auto it = codeMap.find(varCode);
    if (it != codeMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Uknown variable code: " + std::to_string(varCode));
}