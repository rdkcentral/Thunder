#include <time.h>
#include "Time.h"

namespace WPEFramework {
namespace Core {
    static constexpr uint32_t MilliSecondsPerSecond = 1000;
    static constexpr uint32_t MicroSecondsPerMilliSecond = 1000;
    static constexpr uint32_t NanoSecondsPerMicroSecond = 1000;
    static constexpr uint32_t MicroSecondsPerSecond = MilliSecondsPerSecond * MicroSecondsPerMilliSecond;

    // Start day of NTP time as days past the imaginary date 12/1/1 BC.
    // (This is the beginning of the Christian Era, or BCE.)
    static constexpr uint32_t DayNTPStarts = 693596;

    // Start day of the UNIX epoch (1970-01-01), also counting from BCE
    static constexpr uint32_t DayUNIXEpochStarts = 719163;

    // Difference in Seconds between UNIX and NTP epoch (25567).
    static constexpr uint32_t SecondsPerMinute = 60;
    static constexpr uint32_t MinutesPerHour = 60;
    static constexpr uint32_t HoursPerDay = 24;
    static constexpr uint32_t SecondsPerHour = SecondsPerMinute * MinutesPerHour;
    static constexpr uint32_t SecondsPerDay = SecondsPerHour * HoursPerDay;

    static constexpr uint32_t NTPToUNIXSeconds = (DayUNIXEpochStarts - DayNTPStarts) * SecondsPerDay;

    static uint8_t MonthFromString(const TCHAR entry[])
    {
        assert(nullptr != entry);
        // Case sensitive quick compare :-)
        uint32_t value = (static_cast<uint8_t>(entry[0]) << 16) | (static_cast<uint8_t>(entry[1]) << 8) | (static_cast<uint8_t>(entry[2]) << 0);

        switch (value) {
        case 'J' << 16 | 'a' << 8 | 'n':
            return (1);
        case 'F' << 16 | 'e' << 8 | 'b':
            return (2);
        case 'M' << 16 | 'a' << 8 | 'r':
            return (3);
        case 'A' << 16 | 'p' << 8 | 'r':
            return (4);
        case 'M' << 16 | 'a' << 8 | 'y':
            return (5);
        case 'J' << 16 | 'u' << 8 | 'n':
            return (6);
        case 'J' << 16 | 'u' << 8 | 'l':
            return (7);
        case 'A' << 16 | 'u' << 8 | 'g':
            return (8);
        case 'S' << 16 | 'e' << 8 | 'p':
            return (9);
        case 'O' << 16 | 'c' << 8 | 't':
            return (10);
        case 'N' << 16 | 'o' << 8 | 'v':
            return (11);
        case 'D' << 16 | 'e' << 8 | 'c':
            return (12);
        default:
            return (static_cast<uint8_t>(~0));
        }
    }

    static uint32_t YearFromString(const TCHAR entry[], const uint8_t maxLength, uint16_t& year)
    {
        assert(nullptr != entry);

        uint32_t index = 0;

        // Last but not least, lets get the year..
        // Find first digit
        while ((index < maxLength) && (::isdigit(entry[index]) == 0)) {
            index++;
        }

        // If we have at least two digits
        if (((index + 1) < maxLength) && (::isdigit(entry[index]) != 0) && (::isdigit(entry[index + 1]) != 0)) {
            // If we have four digits
            if (((index + 3) < maxLength) && (::isdigit(entry[index + 2]) != 0) && (::isdigit(entry[index + 3]) != 0)) {
                year = static_cast<uint16_t>(((entry[index] - '0') * 1000) + ((entry[index + 1] - '0') * 100) + ((entry[index + 2] - '0') * 10) + (entry[index + 3] - '0'));
                index += 4;
            }
            else {
                year = static_cast<uint16_t>((entry[index] - '0') * 10) + (entry[index + 1] - '0');

                if (year < 70) {
                    year += 2000;
                }
                else {
                    year += 1900;
                }
                index += 2;
            }
        }

        return (index);
    }

    static uint32_t TimeFromString(const TCHAR entry[], const uint8_t maxLength, uint8_t& hours, uint8_t& minutes, uint8_t& seconds)
    {
        assert(nullptr != entry);

        uint8_t index = 0;

        // Find first digit
        while ((index < maxLength) && (::isdigit(entry[index]) == 0)) {
            index++;
        }

        // Do we have hh:mm:ss or h:mm:ss
        if ((index + 5) < maxLength) {
            hours = static_cast<uint8_t>(entry[index] - '0');
            if (::isdigit(entry[index + 1]) != 0) {
                // hh:mm:ss
                hours = static_cast<uint8_t>((hours * 10) + (entry[index + 1] - '0'));
                index += 2;
            }
            else {
                // h:mm:ss
                index++;
            }

            // Now we should be on a colon
            if ((entry[index] == ':') && (::isdigit(entry[index + 1]) != 0)) {
                minutes = static_cast<uint8_t>(entry[index + 1] - '0');
                if (::isdigit(entry[index + 2]) != 0) {
                    // :mm:ss
                    minutes = static_cast<uint8_t>((minutes * 10) + (entry[index + 2] - '0'));
                    index += 3;
                }
                else {
                    // :m:ss ?
                    index += 2;
                }

                // And again we should hit a colon
                if (entry[index] == ':') {
                    if (((index + 1) < maxLength) && (::isdigit(entry[index + 1]) != 0)) {
                        seconds = static_cast<uint8_t>(entry[index + 1] - '0');
                        if (((index + 2) < maxLength) && (::isdigit(entry[index + 2]) != 0)) {
                            // :ss
                            seconds = static_cast<uint8_t>((seconds * 10) + (entry[index + 2] - '0'));
                            index += 3;
                        }
                        else {
                            // :s
                            index += 2;
                        }
                    }
                }
            }
        }

        return (index);
    }

    const TCHAR* Time::WeekDayName() const
    {
        static const TCHAR _weekDayNames[] = 
            _T("Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0???\0");

        uint8_t weekDay = DayOfWeek();
  
        ASSERT (weekDay <= 6);

        return (weekDay <= 6 ? &(_weekDayNames[weekDay * 4]) : &(_weekDayNames[7 * 4]));
    }
       
    const TCHAR* Time::MonthName() const
    {
        static const TCHAR _monthNames[] = 
            _T("???\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0");

        uint8_t month = Month();
  
        ASSERT (month >= 1 && month <= 12);

        return (month >= 1 && month <= 12 ? &(_monthNames[month * 4]) : &(_monthNames[0 * 4]));
    }

    bool Time::FromANSI(const string& buffer, const bool localTime)
    {

        // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format [18]
        uint32_t index = 4;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);

        // Skip space
        while ((index < buffer.size()) && (::isspace(buffer[index]) != 0)) {
            index++;
        }

        // Now we should be on the Month
        if ((index + 3) < static_cast<uint32_t>(buffer.size())) {
            month = MonthFromString(&(buffer.c_str()[index]));

            if (month != static_cast<uint8_t>(~0)) {
                index += 3;

                // Skip Space
                while ((index < static_cast<uint32_t>(buffer.size())) && (::isdigit(buffer[index]) == 0)) {
                    index++;
                }

                if ((index + 7) < buffer.size()) {
                    // Now we should be on the day
                    day = static_cast<uint8_t>(buffer[index] - '0');
                    if (::isdigit(buffer[index + 1]) != 0) {
                        day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                        index += 2;
                    }
                    else {
                        index++;
                    }

                    index += TimeFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), hours,
                        minutes, seconds);

                    index += YearFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), year);

                    if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) && (year != static_cast<uint8_t>(~0))) {
                        *this = Time(year, month, day, hours, minutes, seconds, 0, localTime);

                        return (true);
                    }
                }
            }
        }

        return (false);
    }

    bool Time::FromRFC1123(const string& buffer)
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        uint32_t index = 5;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);

        // Find first digit
        while ((index < buffer.size()) && (::isdigit(buffer[index]) == 0)) {
            index++;
        }

        if ((index + 14) < static_cast<uint32_t>(buffer.size())) {
            // Now we should be on the day
            day = static_cast<uint8_t>(buffer[index] - '0');
            if (::isdigit(buffer[index + 1]) != 0) {
                // dd
                day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                index += 2;
            }
            else {
                // d
                index++;
            }

            // Skip spaces
            while ((index < buffer.size()) && (::isspace(buffer[index]) != 0)) {
                index++;
            }

            // Now we should be on the month
            if ((index + 11) < buffer.size()) {
                month = MonthFromString(&(buffer.c_str()[index]));

                if (month != static_cast<uint8_t>(~0)) {
                    index += 3;

                    // Find next digit
                    while ((index < static_cast<uint32_t>(buffer.size())) && (::isdigit(buffer[index]) == 0)) {
                        index++;
                    }

                    if ((index + 7) < buffer.size()) {
                        index += YearFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                            year);

                        index += TimeFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                            hours, minutes, seconds);

                        if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) && (year != static_cast<uint8_t>(~0))) {
                            bool localTime = true;

                            // Seems like we have a valid time, let see if we need to change the timezone..
                            while ((index < static_cast<uint32_t>(buffer.size())) && (::isspace(buffer[index]) != 0)) {
                                index++;
                            }

                            if ((index + 2) < static_cast<uint32_t>(buffer.size())) {
                                uint32_t value = (static_cast<uint8_t>(buffer[index + 0]) << 16) | (static_cast<uint8_t>(buffer[index + 1]) << 8) | (static_cast<uint8_t>(buffer[index + 2]) << 0);
                                localTime = (value != (('G' << 16) | ('M' << 8) | ('T'))) && (value != (('U' << 16) | ('T' << 8) | ('C')));
                            }

                            *this = Time(year, month, day, hours, minutes, seconds, 0, localTime);

                            return (true);
                        }
                    }
                }
            }
        }

        return (false);
    }

    bool Time::FromRFC1036(const string& buffer)
    {
        // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
        uint32_t index = 6;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);

        while ((index < buffer.size()) && (::isdigit(buffer[index]) == 0)) {
            index++;
        }

        // Now we should be on the Month
        if ((index + 14) < static_cast<uint32_t>(buffer.size())) {
            // Now we should be on the day
            day = static_cast<uint8_t>(buffer[index] - '0');
            if (::isdigit(buffer[index + 1]) != 0) {
                day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                index += 2;
            }
            else {
                index++;
            }

            if ((buffer[index]) == '-') {
                ++index;
                month = MonthFromString(&(buffer.c_str()[index]));

                if ((month != static_cast<uint8_t>(~0)) && ((buffer[index + 3]) == '-')) {
                    index += 4;

                    if ((index + 7) < buffer.size()) {
                        index += YearFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                            year);

                        index += TimeFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                            hours, minutes, seconds);

                        if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) && (year != static_cast<uint8_t>(~0))) {
                            bool localTime = true;

                            // Seems like we have a valid time, let see if we need to change the timezone..
                            while ((index < static_cast<uint32_t>(buffer.size())) && (::isspace(buffer[index]) != 0)) {
                                index++;
                            }

                            if ((index + 2) < static_cast<uint32_t>(buffer.size())) {
                                uint32_t value = (static_cast<uint8_t>(buffer[index + 0]) << 16) | (static_cast<uint8_t>(buffer[index + 1]) << 8) | (static_cast<uint8_t>(buffer[index + 2]) << 0);
                                localTime = (value != (('G' << 16) | ('M' << 8) | ('T'))) && (value != (('U' << 16) | ('T' << 8) | ('C')));
                            }

                            *this = Time(year, month, day, hours, minutes, seconds, 0, localTime);

                            return (true);
                        }
                    }
                }
            }
        }

        return (false);
    }

#ifdef __WIN32__

    // jUST USED FOR A 1 TIME CALCULATION. THE RESULT IS FIXED SO "PASTED" INTO THE VALUE.
    static uint64_t OffsetTicks()
    {
        // Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
        SYSTEMTIME offsetTime;
        FILETIME fileTime;
        offsetTime.wYear = 1970;
        offsetTime.wMonth = 1;
        offsetTime.wDay = 1;
        offsetTime.wHour = 0;
        offsetTime.wMinute = 0;
        offsetTime.wSecond = 0;
        offsetTime.wMilliseconds = 0;
        ::SystemTimeToFileTime(&offsetTime, &fileTime);
        _ULARGE_INTEGER result;

        result.LowPart = fileTime.dwLowDateTime;
        result.HighPart = fileTime.dwHighDateTime;

        // Convert it to MicroSeconds since 1970
        return (result.QuadPart / 10);
    }

    // From the given system time to 1st of January 1970, 00:00:00
    static uint64_t OffsetTicksForEpoch = 11644473600000000ULL;

#endif

#ifdef __POSIX__

    // From the given system time to 1st of January 1970, 00:00:00
    static uint64_t OffsetTicksForEpoch = 0ULL;

#endif

// Invariant for both Linux and Windows: internal time stored is always according to local time specification, so GMT / UTC if local time  is false, local time otherwise.

#ifdef __WIN32__

    Time::Time(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second, const uint16_t millisecond, const bool localTime)
        : _isLocalTime(localTime)
    {
        _time.wYear = year;
        _time.wMonth = month % 12;
        _time.wDay = day % 31;
        _time.wHour = hour % 24;
        _time.wMinute = minute % 60;
        _time.wSecond = second % 60;
        _time.wMilliseconds = millisecond;
        _time.wDayOfWeek = static_cast<WORD>(~0);
    }

    // Uint64 is the time in MicroSeconds !!!
    Time::Time(const uint64_t time, bool localTime)
        : _time()
        , _isLocalTime(localTime)
    {
        FILETIME fileTime;
        _ULARGE_INTEGER result;

        result.QuadPart = (time + OffsetTicksForEpoch) * 10;

        fileTime.dwLowDateTime = result.LowPart;
        fileTime.dwHighDateTime = result.HighPart;

        ::FileTimeToSystemTime(&fileTime, &_time);
        if (IsLocalTime())
        {
            SYSTEMTIME convertedTime;
            SystemTimeToTzSpecificLocalTime(nullptr, &_time, &convertedTime);
            _time = convertedTime;
        }
    }

    Time::Time(const FILETIME& time, bool localTime /*= false*/)
        : _time()
        , _isLocalTime(localTime)
    {
        ::FileTimeToSystemTime(&time, &_time);
    }

    // Return the time in MicroSeconds, since since January 1, 1970 00:00:00 (UTC)...
    uint64_t Time::Ticks() const
    {
        // Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
        FILETIME fileTime {};
        if (IsLocalTime())
        {
            SYSTEMTIME convertedTime;
            TzSpecificLocalTimeToSystemTime(nullptr, &_time, &convertedTime);
            ::SystemTimeToFileTime(&convertedTime, &fileTime);
        }
        else
        {
            ::SystemTimeToFileTime(&_time, &fileTime);
        }
        _ULARGE_INTEGER result;

        result.LowPart = fileTime.dwLowDateTime;
        result.HighPart = fileTime.dwHighDateTime;
        // Return the time in MicroSeconds...
        uint64_t microSeconds = (result.QuadPart / 10);
        if (microSeconds > OffsetTicksForEpoch)
            microSeconds -= OffsetTicksForEpoch;
        else
            microSeconds = 0;
        return microSeconds;
    }

    string Time::Format(const TCHAR* formatter) const
    {
        TCHAR buffer[200];

        struct tm convertedTime;
        ::memset(&convertedTime, 0, sizeof(convertedTime));

        convertedTime.tm_year = _time.wYear - 1900;
        convertedTime.tm_mon = _time.wMonth - 1;
        convertedTime.tm_mday = _time.wDay;
        convertedTime.tm_hour = _time.wHour;
        convertedTime.tm_min = _time.wMinute;
        convertedTime.tm_sec = _time.wSecond;
        convertedTime.tm_wday = _time.wDayOfWeek;
        if (_time.wDayOfWeek == WORD(~0))
        {
            convertedTime.tm_wday = DayOfWeek();
        }
        convertedTime.tm_yday = DayOfYear();
        _tcsftime(buffer, sizeof(buffer), formatter, &convertedTime);

        return (string(buffer));
    }

    uint8_t Time::DayOfWeek() const
    {
        if (_time.wDayOfWeek == static_cast<WORD>(~0)) {
            SYSTEMTIME newTime;
            FILETIME fileTime;
            ::SystemTimeToFileTime(&_time, &fileTime);
            ::FileTimeToSystemTime(&fileTime, &newTime);

            _time.wDayOfWeek = newTime.wDayOfWeek;
        }

        return (static_cast<uint8_t>(_time.wDayOfWeek));
    }

    uint16_t Time::DayOfYear() const
    {
        struct tm convertedTime;
        convertedTime.tm_year = _time.wYear - 1900;
        convertedTime.tm_mon = _time.wMonth - 1;
        convertedTime.tm_mday = _time.wDay;
        convertedTime.tm_hour = _time.wHour;
        convertedTime.tm_min = _time.wMinute;
        convertedTime.tm_sec = _time.wSecond;

        time_t timeStamp = mktime(&convertedTime);
        convertedTime = *localtime(&timeStamp);

        return (static_cast<uint16_t>(convertedTime.tm_yday));
    }

    string Time::ToRFC1123(const bool localTime) const
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        TCHAR buffer[32];

        if (!IsValid())
            return string();

        const TCHAR* zone = (localTime == false ? _T("GMT") : _T(""));

        if (localTime != IsLocalTime())
        {
            SYSTEMTIME convertedTime;
            if (IsLocalTime())
            {
                TzSpecificLocalTimeToSystemTime(nullptr, &_time, &convertedTime);
            }
            else
            {
                SystemTimeToTzSpecificLocalTime(nullptr, &_time, &convertedTime);
            }
            Time converted(convertedTime, localTime);
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d %s"), converted.WeekDayName(),
                converted.Day(), converted.MonthName(), converted.Year(),
                converted.Hours(), converted.Minutes(), converted.Seconds(), zone);
        }
        else
#pragma warning(disable : 4996)
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d %s"), WeekDayName(), Day(), MonthName(), Year(),
                      Hours(), Minutes(), Seconds(), zone);
#pragma warning(default : 4996)

        return (string(buffer));
    }

    /* static */ Time Time::Now()
    {
        SYSTEMTIME systemTime;

        ::GetSystemTime(&systemTime);

        return (systemTime);
    }

#endif

#ifdef __POSIX__
    Time::Time(const struct timespec& time, bool localTime)
    {
        if (localTime) {
            localtime_r(&time.tv_sec, &_time);
        }
        else {
            gmtime_r(&time.tv_sec, &_time);
        }

        // Calculate ticks..
        _ticks = (static_cast<uint64_t>(time.tv_sec) * MicroSecondsPerSecond) + (time.tv_nsec / NanoSecondsPerMicroSecond) + OffsetTicksForEpoch;
    }

    // GMT specific version of mktime(), taken from BSD source code
    /* Number of days per month (except for February in leap years). */
    static const int monoff[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    static int is_leap_year(int year)
    {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }

    static int leap_days(int y1, int y2)
    {
        --y1;
        --y2;
        return (y2 / 4 - y1 / 4) - (y2 / 100 - y1 / 100) + (y2 / 400 - y1 / 400);
    }

    static time_t mktimegm(const struct tm* tm)
    {
        int year;
        time_t days;
        time_t hours;
        time_t minutes;
        time_t seconds;

        year = 1900 + tm->tm_year;
        days = 365 * (year - 1970) + leap_days(1970, year);
        days += monoff[tm->tm_mon];

        if (tm->tm_mon > 1 && is_leap_year(year))
            ++days;
        days += tm->tm_mday - 1;

        hours = days * 24 + tm->tm_hour;
        minutes = hours * 60 + tm->tm_min;
        seconds = minutes * 60 + tm->tm_sec;

        return seconds;
    }

    Time::Time(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second, const uint16_t millisecond, const bool localTime)
    {
        struct tm source {};

        source.tm_year = year - 1900;
        source.tm_mon = month - 1;
        source.tm_mday = day;
        source.tm_hour = hour;
        source.tm_min = minute;
        source.tm_sec = second;

        time_t flatTime{};
        if (localTime)
            flatTime = mktime(&source);
        else
            flatTime = mktimegm(&source);

        if (localTime) {
            localtime_r(&flatTime, &_time);
        }
        else {
            gmtime_r(&flatTime, &_time);
        }

        // Calculate ticks..
        _ticks = (flatTime * MicroSecondsPerSecond) + (millisecond * MicroSecondsPerMilliSecond) + OffsetTicksForEpoch;
    }

    Time::Time(const struct timeval& info)
    {
        _ticks = (static_cast<uint64_t>(info.tv_sec) * static_cast<uint64_t>(MicroSecondsPerSecond)) + static_cast<uint64_t>(info.tv_usec) + OffsetTicksForEpoch;

        // This is the seconds since 1970...
        struct tm* ptm = gmtime(&info.tv_sec);

        _time = *ptm;
    }
    Time::Time(const uint64_t time, const bool localTime /*= false*/)
        : _time()
        , _ticks(time)
    {
        // This is the seconds since 1970...
        time_t epochTimestamp = static_cast<time_t>((time - OffsetTicksForEpoch) / MicroSecondsPerSecond);

        if (localTime)
            localtime_r(&epochTimestamp, &_time);
        else
            gmtime_r(&epochTimestamp, &_time);
    }

    uint64_t Time::Ticks() const
    {
        return (_ticks);
    }

    uint8_t Time::DayOfWeek() const
    {
        return (static_cast<uint8_t>(_time.tm_wday));
    }

    uint16_t Time::DayOfYear() const
    {
        return (static_cast<uint16_t>(_time.tm_yday));
    }

    string Time::ToRFC1123(const bool localTime) const
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        TCHAR buffer[32];
        const TCHAR* zone = (localTime == false) ? _T("GMT") : _T("");

        if (!IsValid())
            return string();

        if (localTime != IsLocalTime()) {
            // We need to convert from local to GMT or vv
            time_t epochTimestamp;
            struct tm originalTime = _time;
            if (IsLocalTime())
                epochTimestamp = mktime(&originalTime);
            else
                epochTimestamp = mktimegm(&originalTime);
            timespec convertedTime{ epochTimestamp, 0 };
            Time converted(convertedTime, localTime);
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d %s"), converted.WeekDayName(),
                converted.Day(), converted.MonthName(), converted.Year(),
                converted.Hours(), converted.Minutes(), converted.Seconds(), zone);
        }
        else {
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d %s"), WeekDayName(), Day(), MonthName(), Year(), Hours(),
                Minutes(), Seconds(), zone);
        }

        return (string(buffer));
    }

    string Time::Format(const TCHAR* formatter) const
    {
        TCHAR buffer[200];

        _tcsftime(buffer, sizeof(buffer), formatter, &_time);

        return (string(buffer));
    }

    /* static */ Time Time::Now()
    {
        struct timeval currentTime;
        gettimeofday(&currentTime, nullptr);

        return (Time(currentTime));
    }

#endif

    string Time::ToRFC1123() const
    {
        return ToRFC1123(IsLocalTime());
    }

    Time& Time::Add(const uint32_t timeInMilliseconds)
    {
        // Calculate the new time !!
        uint64_t newTime = Ticks() + static_cast<uint64_t>(timeInMilliseconds) * MilliSecondsPerSecond;
        return (operator=(Time(newTime, IsLocalTime())));
    }

    Time& Time::Sub(const uint32_t timeInMilliseconds)
    {
        // Calculate the new time !!
        uint64_t newTime = Ticks() - static_cast<uint64_t>(timeInMilliseconds) * MilliSecondsPerSecond;
        return (operator=(Time(newTime, IsLocalTime())));
    }

    uint64_t Time::NTPTime() const
    {
        // static const uint64_t epoch = 94354848000000000LL;

        // Tick count starts at 1st January 1970 00:00.00, so might need adaptions for NTP
        uint32_t wholeSeconds = static_cast<uint32_t>(Ticks() / MicroSecondsPerSecond);
        wholeSeconds += NTPToUNIXSeconds;
        uint64_t seconds = static_cast<uint64_t>(wholeSeconds) << 32;
        seconds += static_cast<uint32_t>(MilliSeconds() * 4294967.296); // 2^32/10^3

        return (seconds);
    }

int32_t Time::DifferenceFromGMTSeconds() const
{
#ifdef __WIN32__
    TIME_ZONE_INFORMATION timeZoneInfo;
    GetTimeZoneInformation(&timeZoneInfo);

    return static_cast<int32_t>(-timeZoneInfo.Bias * 60);
#else
    return static_cast<int32_t>(Handle().tm_gmtoff);
#endif

}

}
} // namespace Core
