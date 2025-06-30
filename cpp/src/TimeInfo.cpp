#include <TimeInfo.hpp>

#include <stdexcept>
#include <cassert>

bool TimeInfo::operator==(const TimeInfo& other) const {
    return year == other.year &&
        month == other.month &&
        day == other.day &&
        hour == other.hour &&
        timezone == other.timezone;
}

bool TimeInfo::operator!=(const TimeInfo& other) const {
    return !(*this == other);
}

std::string TimeInfo::to_string() const {
    // Format: YYYY-MM-DD HH:00 TZN
    char buffer[21];
    snprintf(
        buffer,
        sizeof(buffer),
        "%04u-%02u-%02u %02u:00 %s",
        year, month, day, hour, timezone_to_string(timezone).c_str()
    );
    return std::string(buffer);
}

std::string timezone_to_string(Timezone tz) {
    std::string result;

    switch(tz) {
        case Timezone::UTC:
            result = "UTC";
            break;
        case Timezone::GMT:
            result = "GMT";
            break;
        default:
            throw std::invalid_argument("Invalid timezone provided.");
    }

    assert(result.length() == 3 && "Timezone string must be exactly 3 characters");
    return result;
}