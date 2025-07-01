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

    friend struct std::hash<TimeInfo>;
};

std::string timezone_to_string(Timezone tz);

namespace std {
    template<>
    struct hash<TimeInfo> {
        size_t operator()(const TimeInfo& t) const {
            size_t h1 = std::hash<unsigned int>{}(t.year);
            size_t h2 = std::hash<unsigned int>{}(t.month);
            size_t h3 = std::hash<unsigned int>{}(t.day);
            size_t h4 = std::hash<unsigned int>{}(t.hour);
            size_t h5 = std::hash<int>{}(static_cast<int>(t.timezone));

            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
        }
    };
}

#endif // TIME_INFO_HPP