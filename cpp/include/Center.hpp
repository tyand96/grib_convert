#ifndef CENTER_HPP
#define CENTER_HPP

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

#endif // CENTER_HPP