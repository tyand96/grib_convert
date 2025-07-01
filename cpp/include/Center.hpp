#ifndef CENTER_HPP
#define CENTER_HPP

#include <string>

enum class Center {
    CMCC,
    DWD,
    ECCC,
    ECMWF,
    JMA,
    METEO_FRANCE,
    NCEP,
    UKMO,
    BOM
};

Center center_from_code(unsigned int centerCode);

std::string center_as_string(Center center);

#endif // CENTER_HPP