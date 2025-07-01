#include <Center.hpp>

#include <unordered_map>
#include <stdexcept>

Center center_from_code(unsigned int centerCode) {
    std::unordered_map<unsigned int, Center> codeMap = {
        {2U, Center::BOM},
        {250U, Center::CMCC},
        {78U, Center::DWD},
        {54U, Center::ECCC},
        {98U, Center::ECMWF},
        {34U, Center::JMA},
        {85U, Center::METEO_FRANCE},
        {7U, Center::NCEP},
        {74U, Center::UKMO}
    };

    auto it = codeMap.find(centerCode);
    if (it != codeMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Uknown center code: " + std::to_string(centerCode));
}

std::string center_as_string(Center center) {
    switch(center) {
        case (Center::BOM):
            return "bom";
        case (Center::CMCC):
            return "cmcc";
        case (Center::DWD):
            return "dwd";
        case (Center::ECCC):
            return "eccc";
        case Center::ECMWF:
            return "ecmwf";
        case Center::JMA:
            return "jma";
        case Center::METEO_FRANCE:
            return "meteo_france";
        case Center::NCEP:
            return "ncep";
        case Center::UKMO:
            return "ukmo";
        default:
            throw std::invalid_argument("Invalid center.");
    }
}