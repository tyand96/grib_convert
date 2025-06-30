#ifndef TIME_INFO_HPP
#define TIME_INFO_HPP

#include <string>

enum class Timezone {
    UTC,
    GMT
};

struct TimeInfo {
    unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    Timezone timezone;

    // Default Constructor
    TimeInfo() : year(0), month(0), day(0), hour(0), timezone(Timezone::UTC) {}

    // Constructor with common parameters
    TimeInfo(
        unsigned int y, unsigned int m, unsigned int d = 1, unsigned int h = 0,
        Timezone tz = Timezone::UTC
    ): year(y), month(m), day(d), hour(h), timezone(tz) {}

    std::string to_string() const;

    bool operator==(const TimeInfo& other) const;
    bool operator!=(const TimeInfo& other) const;
};

std::string timezone_to_string(Timezone tz);

#endif // TIME_INFO_HPP