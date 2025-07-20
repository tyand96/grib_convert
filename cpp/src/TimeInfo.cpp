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

bool TimeInfo::operator<(const TimeInfo& other) const {
    // Create normalized copies for comparison
    TimeInfo thisUTC = this->toUTC();
    TimeInfo otherUTC = other.toUTC();

    if (thisUTC.year != otherUTC.year) {
        return thisUTC.year < otherUTC.year;
    }

    if (thisUTC.month != otherUTC.month) {
        return thisUTC.month < otherUTC.month;
    }

    if (thisUTC.day != otherUTC.day) {
        return thisUTC.day < otherUTC.day;
    }

    return thisUTC.hour < otherUTC.hour;
}

bool TimeInfo::operator>(const TimeInfo& other) const {
    return other < *this; // Use the less than operator for comparison
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

float TimeInfo::toFloat() const {
    // Convert to a float of number of hours since epoch
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900; // tm_year is years since 1900
    timeinfo.tm_mon = month - 1; // tm_mon is 0-11
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    timeinfo.tm_isdst = -1; // Not considering daylight saving time
    timeinfo.tm_gmtoff = timezone_to_offset(timezone);
    time_t epoch_time = timegm(&timeinfo); // Convert to UTC epoch time
    if (epoch_time == -1) {
        throw std::runtime_error("Failed to convert TimeInfo to epoch time.");
    }
    // Return the time in hours since epoch
    return static_cast<float>(epoch_time) / 3600.0f; // Convert seconds to hours
}

bool TimeInfo::isLeapYear() const {
    return (
        this->year % 4 == 0 &&
        (
            this->year % 100 != 0 || this->year % 400 == 0
        )
    );
}

TimeInfo TimeInfo::toUTC() const {
    if (this->timezone == Timezone::UTC) return *this;


    TimeInfo utcTime = *this;
    int offset = timezone_to_offset(utcTime.timezone);
    int newHour = static_cast<int>(hour) - offset;

    if (newHour >= 24) {
        utcTime.hour = newHour - 24;

        utcTime.day++;

        bool needToAdvanceMonth = false;
        switch (utcTime.month) {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                if (utcTime.day > 31) needToAdvanceMonth = true;
                break;
            case 4: case 6: case 9: case 11:
                if (utcTime.day > 30) needToAdvanceMonth = true;
                break;
            case 2:
                bool isLeapYear = utcTime.isLeapYear();
                if ((isLeapYear && utcTime.day > 29) || (!isLeapYear && utcTime.day > 28))
                    needToAdvanceMonth = true;
                break;
        }

        if (needToAdvanceMonth) {
            utcTime.day = 1;
            utcTime.month++;

            if (utcTime.month > 12) {
                utcTime.month = 1;
                utcTime.year++;
            }
        }
    } else if (newHour < 0) {
        utcTime.hour = newHour + 24;

        if (utcTime.day > 1) {
            utcTime.day--;
        } else {
            if (utcTime.month > 1) {
                utcTime.month--;
            } else {
                utcTime.month = 12;
                utcTime.year--;
            }

            // Set the day to the last day of the previous month
            switch (utcTime.month) {
                case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                    utcTime.day = 31;
                    break;
                case 4: case 6: case 9: case 11:
                    utcTime.day = 30;
                    break;
                case 2:
                    bool isLeapYear = utcTime.isLeapYear();
                    utcTime.day = isLeapYear ? 29 : 28;
                    break;
            }
        }
    } else {
        utcTime.hour = newHour;
    }

    return utcTime;
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

int timezone_to_offset(Timezone tz) {
    switch(tz) {
        case Timezone::UTC:
            return 0; // UTC is 0 offset
        case Timezone::GMT:
            return 0; // GMT is also 0 offset
        default:
            throw std::invalid_argument("Invalid timezone provided.");
    }
}