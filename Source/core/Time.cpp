/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Time.h"
#include "Number.h"
#include <time.h>

namespace {
    // Start day of NTP time as days past the imaginary date 12/1/1 BC.
    // (This is the beginning of the Christian Era, or BCE.)
    constexpr uint32_t DayNTPStarts = 693596;

    // Start day of the UNIX epoch (1970-01-01), also counting from BCE
    constexpr uint32_t DayUNIXEpochStarts = 719163;

    constexpr uint32_t NTPToUNIXSeconds = (DayUNIXEpochStarts - DayNTPStarts) * Thunder::Core::Time::SecondsPerDay;

}


namespace Thunder {
namespace Core {

    static bool IsLeapYear(const uint16_t year)
    {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }

    static void SkipLeadingSpaces(const TCHAR buffer[], const uint8_t maxLength, uint32_t& index) {
        while ((index < maxLength) && (::isspace(buffer[index]) != 0)) {
            index++;
        }
        return;
    }

    static uint8_t WeekFromName(const TCHAR entry[])
    {
        assert(nullptr != entry);
        uint8_t day = static_cast<uint8_t>(~0);
        // Case sensitive quick compare :-)

        if (strcmp(entry, "Sunday") == 0) {
            day = (0);
        } else if (strcmp(entry, "Monday") == 0) {
            day = (1);
        } else if (strcmp(entry, "Tuesday") == 0) {
            day = (2);
        } else if (strcmp(entry, "Wednesday") == 0) {
            day = (3);
        } else if (strcmp(entry, "Thursday") == 0) {
            day = (4);
        } else if (strcmp(entry, "Friday") == 0) {
            day = (5);
        } else if (strcmp(entry, "Saturday") == 0) {
           day = (6);
        }
        return (day);
    }

    static uint8_t WeekFromAbbrevation(const TCHAR entry[])
    {
        assert(nullptr != entry);
        uint8_t day = static_cast<uint8_t>(~0);
        // Case sensitive quick compare :-)

        uint32_t value = (static_cast<uint8_t>(entry[0]) << 16) | (static_cast<uint8_t>(entry[1]) << 8) | (static_cast<uint8_t>(entry[2]) << 0);
        switch (value) {
        case 'S' << 16 | 'u' << 8 | 'n':
            day = (0);
            break;
        case 'M' << 16 | 'o' << 8 | 'n':
            day = (1);
            break;
        case 'T' << 16 | 'u' << 8 | 'e':
            day = (2);
            break;
        case 'W' << 16 | 'e' << 8 | 'd':
            day = (3);
            break;
        case 'T' << 16 | 'h' << 8 | 'u':
            day = (4);
            break;
        case 'F' << 16 | 'r' << 8 | 'i':
            day = (5);
            break;
        case 'S' << 16 | 'a' << 8 | 't':
            day = (6);
            break;
        default:
            break;
        }
        return (day);
    }

    static uint32_t WeekFromString(const string& buffer, const uint8_t delimeter, const bool fromName, uint8_t& wday) {
        uint32_t index = 0;
        SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);
        string weekDayName = buffer.substr(index, buffer.find_first_of(delimeter, index));
        if (weekDayName.size() > 0) {
            wday = (fromName ? WeekFromName(weekDayName.c_str()) : (weekDayName.size() == 3) ? WeekFromAbbrevation(weekDayName.c_str()) : static_cast<uint8_t>(~0));
            index += static_cast<uint32_t>(weekDayName.length()) + 1;
        }
        return index;
    }

    static uint32_t MonthFromString(const TCHAR entry[], const uint8_t maxLength, uint8_t& month)
    {
        assert(nullptr != entry);
        uint32_t index = 0;
        SkipLeadingSpaces(entry, maxLength, index);
        // Case sensitive quick compare :-)
        if ((index + 3) < maxLength) {
            uint32_t value = (static_cast<uint8_t>(entry[index]) << 16) | (static_cast<uint8_t>(entry[index + 1]) << 8) | (static_cast<uint8_t>(entry[index + 2]) << 0);
            index += 3;

            switch (value) {
            case 'J' << 16 | 'a' << 8 | 'n':
                month = (1);
                break;
            case 'F' << 16 | 'e' << 8 | 'b':
                month = (2);
                break;
            case 'M' << 16 | 'a' << 8 | 'r':
                month = (3);
                break;
            case 'A' << 16 | 'p' << 8 | 'r':
                month = (4);
                break;
            case 'M' << 16 | 'a' << 8 | 'y':
                month = (5);
                break;
            case 'J' << 16 | 'u' << 8 | 'n':
                month = (6);
                break;
            case 'J' << 16 | 'u' << 8 | 'l':
                month = (7);
                break;
            case 'A' << 16 | 'u' << 8 | 'g':
                month = (8);
                break;
            case 'S' << 16 | 'e' << 8 | 'p':
                month = (9);
                break;
            case 'O' << 16 | 'c' << 8 | 't':
                month = (10);
                break;
            case 'N' << 16 | 'o' << 8 | 'v':
                month = (11);
                break;
            case 'D' << 16 | 'e' << 8 | 'c':
                month = (12);
                break;
            default:
                month = (static_cast<uint8_t>(~0));
                break;
            }
        }
        return index;
    }

    static uint32_t YearFromString(const TCHAR entry[], const uint8_t maxLength, uint16_t& year)
    {
        assert(nullptr != entry);

        uint32_t index = 0;

        SkipLeadingSpaces(entry, maxLength, index);

        // If we have at least two digits
        if (((index + 1) < maxLength) && (::isdigit(entry[index]) != 0) && (::isdigit(entry[index + 1]) != 0)) {
            // If we have four digits
            if (((index + 3) < maxLength) && (::isdigit(entry[index + 2]) != 0) && (::isdigit(entry[index + 3]) != 0)) {
                year = static_cast<uint16_t>(((entry[index] - '0') * 1000) + ((entry[index + 1] - '0') * 100) + ((entry[index + 2] - '0') * 10) + (entry[index + 3] - '0'));
                index += 4;
            } else {
                year = static_cast<uint16_t>((entry[index] - '0') * 10) + (entry[index + 1] - '0');

                if (year < 70) {
                    year += 2000;
                } else {
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

        uint32_t index = 0;

        SkipLeadingSpaces(entry, maxLength, index);

        // Do we have hh:mm:ss or h:mm:ss
        if (((index + 5) <= maxLength) && (::isdigit(entry[index]) != 0)) {
            hours = static_cast<uint8_t>(entry[index] - '0');
            if (::isdigit(entry[index + 1]) != 0) {
                // hh:mm:ss
                hours = static_cast<uint8_t>((hours * 10) + (entry[index + 1] - '0'));
                index += 2;
            } else {
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
                } else {
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
                        } else {
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
#ifdef __POSIX__
    {
        return WeekDayName(TMHandle());
    }
    const TCHAR* Time::WeekDayName(const struct tm& time)
#endif
    {
        static const TCHAR _weekDayNames[] = _T("Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0???\0");

#ifdef __WINDOWS__
        uint8_t weekDay = DayOfWeek();
#else
        uint8_t weekDay = DayOfWeek(time);
#endif

        ASSERT(weekDay <= 6);

        return (weekDay <= 6 ? &(_weekDayNames[weekDay * 4]) : &(_weekDayNames[7 * 4]));
    }

    const TCHAR* Time::MonthName() const
#ifdef __POSIX__
    {
        return MonthName(TMHandle());
    }
    const TCHAR* Time::MonthName(const struct tm& time)
#endif
    {
        static const TCHAR _monthNames[] = _T("???\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0");

#ifdef __WINDOWS__
        uint8_t month = Month();
#else
        uint8_t month = Month(time);
#endif

        ASSERT(month >= 1 && month <= 12);

        return (month >= 1 && month <= 12 ? &(_monthNames[month * 4]) : &(_monthNames[0 * 4]));
    }

    bool Time::FromANSI(const string& buffer, const bool localTime)
    {
        // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format [18]
        uint32_t index = 0;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);
        uint8_t wday = static_cast<uint8_t>(~0);
        index += WeekFromString(buffer, ' ', false, wday);

        if (wday != static_cast<uint8_t>(~0)) {

            // Now we should be on the Month
            if ((index + 3) < static_cast<uint32_t>(buffer.size())) {
                index += MonthFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), month);

                if (month != static_cast<uint8_t>(~0)) {
                    SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);

                    if (((index + 7) < buffer.size()) && (::isdigit(buffer[index]) != 0)) {
                        // Now we should be on the day
                        day = static_cast<uint8_t>(buffer[index] - '0');
                        if (::isdigit(buffer[index + 1]) != 0) {
                            day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                            index += 2;
                        } else {
                            index++;
                        }

                        index += TimeFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), hours,
                            minutes, seconds);

                         index += YearFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), year);

                        if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) && (year != static_cast<uint16_t>(~0))) {
                            *this = Time(year, month, day, hours, minutes, seconds, 0, localTime);
                            return (true);
                        }
                    }
                }
            }
        }

        return (false);
    }

    bool Time::FromRFC1123(const string& buffer)
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        uint32_t index = 0;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);
        uint8_t wday = static_cast<uint8_t>(~0);
        index += WeekFromString(buffer, ',', false, wday);

        if (wday != static_cast<uint8_t>(~0)) {
           SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);

            if ((index + 14) < static_cast<uint32_t>(buffer.size())) {
                // Now we should be on the day
                day = static_cast<uint8_t>(buffer[index] - '0');
                if (::isdigit(buffer[index + 1]) != 0) {
                    // dd
                    day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                    index += 2;
                } else {
                    // d
                    index++;
                }

                // Now we should be on the month
                if ((index + 11) < buffer.size()) {
                    index += MonthFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), month);

                    if (month != static_cast<uint8_t>(~0)) {

                        if ((index + 7) < buffer.size()) {
                            index += YearFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                                year);

                            index += TimeFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index),
                                hours, minutes, seconds);

                            if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) && (year != static_cast<uint8_t>(~0))) {
                                bool localTime = true;


                                // Seems like we have a valid time, let see if we need to change the timezone..
                                SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);

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
        }

        return (false);
    }

    bool Time::FromRFC1036(const string& buffer)
    {
        // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
        uint32_t index = 0;
        uint16_t year = static_cast<uint16_t>(~0);
        uint8_t month = static_cast<uint8_t>(~0);
        uint8_t day = static_cast<uint8_t>(~0);
        uint8_t hours = static_cast<uint8_t>(~0);
        uint8_t minutes = static_cast<uint8_t>(~0);
        uint8_t seconds = static_cast<uint8_t>(~0);
        uint8_t wday = static_cast<uint8_t>(~0);
        index += WeekFromString(buffer, ',', true, wday);

        if (wday != static_cast<uint8_t>(~0)) {

            SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);
            if ((index + 14) <= static_cast<uint32_t>(buffer.size())) {
                // Now we should be on the day
                day = static_cast<uint8_t>(buffer[index] - '0');
                if (::isdigit(buffer[index + 1]) != 0) {
                    day = static_cast<uint8_t>((day * 10) + (buffer[index + 1] - '0'));
                    index += 2;
                } else {
                    index++;
                }

                // Now we should be on the Month
                if ((buffer[index]) == '-') {
                    ++index;
                    index += MonthFromString(&(buffer.c_str()[index]), static_cast<uint8_t>(buffer.size() - index), month);

                    if ((month != static_cast<uint8_t>(~0)) && ((buffer[index]) == '-')) {
                        index += 1;

                        if ((index + 7) < buffer.size()) {
                            index += YearFromString(&(buffer.c_str()[index]),
                                     static_cast<uint8_t>(buffer.size() - index), year);

                            index += TimeFromString(&(buffer.c_str()[index]),
                                     static_cast<uint8_t>(buffer.size() - index), hours, minutes, seconds);

                            if ((day != 0) && (seconds != static_cast<uint8_t>(~0)) &&
                                (year != static_cast<uint8_t>(~0))) {
                                bool localTime = true;

                                SkipLeadingSpaces(buffer.c_str(), static_cast<uint8_t>(buffer.size()), index);

                                if ((index + 2) < static_cast<uint32_t>(buffer.size())) {
                                    uint32_t value = (static_cast<uint8_t>(buffer[index + 0]) << 16) |
                                            (static_cast<uint8_t>(buffer[index + 1]) << 8) |
                                            (static_cast<uint8_t>(buffer[index + 2]) << 0);
                                    localTime = (value != (('G' << 16) | ('M' << 8) | ('T'))) &&
                                                (value != (('U' << 16) | ('T' << 8) | ('C')));
                                }

                                *this = Time(year, month, day, hours, minutes, seconds, 0, localTime);

                                return (true);
                            }
                        }
                    }
                }
            }
        }

        return (false);
    }

    bool Time::FromISO8601(const std::string& buffer)
    {
        // ISO8601 extended notation only, hour/minute/second must not be omitted
        // Variants supported:
        // 1994-11-06T08:49:37              ; local time
        // 1994-11-06T08:49:37Z             ; UTC time
        // 1994-11-06T08:49:37.123          ; local time with fractions of seconds
        // 1994-11-06T08:49:37.123Z         ; UTC time with fractions of seconds
        // 1994-11-06T08:49:37+01           ; time with hour offset
        // 1994-11-06T08:49:37+06:45        ; time with hour:minute offset
        // 1994-11-06T08:49:37.123+01       ; time with fractions of seconds and hour offset
        // 1994-11-06T08:49:37.123+06:45    ; time with fractions of seconds and hour:minute offset

        bool result = false;
        int year = -1;
        int month = -1;
        int day = -1;
        int hours = -1;
        int minutes = -1;
        int seconds = -1;
        int miliseconds = 0;
        int offset = 0;
        bool offsetNegative = false;
        bool localTime = false;

        if ((buffer.length() >= 19) && ::isdigit(buffer[0])) {
            const char *cbuffer = buffer.c_str();
            char *endptr = nullptr;

            year = std::strtol(cbuffer, &endptr, 10);
            if ((year >= 1582) && (year <= 9999) && *endptr == '-') { // 1582 = start of Gregorian calendar
                month = std::strtol(cbuffer + 5, &endptr, 10);
                if ((month >= 1) && (month <= 12) && (*endptr == '-')) {
                    day = std::strtol(cbuffer + 8, &endptr, 10);
                    if ((day >= 1) && (day <= 31) && (*endptr == 'T')) {
                        hours = std::strtol(cbuffer + 11, &endptr, 10);
                        if ((hours >= 0) && (hours <= 23) && (*endptr == ':')) {
                            minutes = std::strtol(cbuffer + 14, &endptr, 10);
                            if ((minutes >= 0) && (minutes <= 59) && (*endptr == ':')) {
                                seconds = std::strtol(cbuffer + 17, &endptr, 10);
                                if ((seconds >= 0) && (seconds <= 59)) {
                                    result = true; // date and time was OK
 
                                    // Handle fractions of seconds
                                    if (*endptr == '.') {
                                        if (buffer.length() >= static_cast<size_t>((endptr - cbuffer) + 1)) {
                                            uint32_t length = (static_cast<uint32_t>(buffer.length() -
                                                              static_cast<size_t>((endptr - cbuffer) + 1)));
                                            miliseconds = NumberType<uint32_t>(TextFragment(&(endptr[1]), length)).Value();
                                            length = static_cast<uint32_t>(NumberType<uint32_t>(TextFragment(&(endptr[1]), length)).Text().length());
                                            endptr += length + 1;
                                        } else {
                                            result = false;
                                        }
                                    }

                                    if (result == true) {
                                        if ((*endptr == ' ') || (*endptr == '\0')) {
                                            // No timezone offset information, assume it's local time
                                            localTime = true;
                                        } else if ((*endptr == '+') || (*endptr == '-')) {
                                            // Handle timezone
                                            offsetNegative = (*endptr == '-');

                                            if (buffer.length() >= static_cast<size_t>((endptr - cbuffer) + 3)) {
                                                int timezoneHr = std::strtol(endptr + 1, &endptr, 10);
                                                int timezoneMin = 0;
                                                if (*endptr == ':') {
                                                    if (buffer.length() >= static_cast<size_t>((endptr - cbuffer) + 3)) {
                                                        timezoneMin = std::strtol(endptr + 1, &endptr, 10);
                                                    } else {
                                                        result = false;
                                                    }
                                                }

                                                if (result == true) {
                                                    if ((*endptr != ' ') && (*endptr != '\0')) {
                                                        // Nothing more expected after timezone offset
                                                        result = false;
                                                    } else {
                                                        if ((timezoneHr >= -23) && (timezoneHr <= 23) &&
                                                                 (timezoneMin >= 0) && (timezoneMin <= 59)) {

                                                            offset = (timezoneHr * 60) + timezoneMin;
                                                        } else {
                                                            result = false;
                                                        }
                                                    }
                                                }
                                            }
                                            else {
                                                result = false;
                                            }
                                        }
                                        else if (*endptr != 'Z') {
                                            // Nothing else except time offset or 'Z' is allowed
                                            // at the end of the string
                                            result = false;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (result) {
            *this = Time(year, month, day, hours, minutes, seconds, miliseconds, localTime);

            if (offset != 0) {
                if (offsetNegative == true) {
                    Sub(offset * SecondsPerMinute * MilliSecondsPerSecond);
                } else {
                    Add(offset * SecondsPerMinute * MilliSecondsPerSecond);
                }
            }
        }

        return (result);
    }

#ifdef __WINDOWS__

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

#ifdef __WINDOWS__

    Time::Time(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second, const uint16_t millisecond, const bool localTime)
        : _time()
    {
        if (IsValidDateTime(year, month, day, hour, minute, second, millisecond) == true) {

            _time.wYear = year;
            _time.wMonth = month;
            _time.wDay = day;
            _time.wHour = hour;
            _time.wMinute = minute;
            _time.wSecond = second;
            _time.wMilliseconds = millisecond;
            _time.wDayOfWeek = static_cast<WORD>(~0);

            if (localTime) {
                SYSTEMTIME convertedTime;
                TzSpecificLocalTimeToSystemTime(nullptr, &_time, &convertedTime);
                _time = convertedTime;
            }
        }
    }

    /**
     * Get day count since Monday, January 1, 4713 BC
     * https://en.wikipedia.org/wiki/Julian_day
     */
    double Time::JulianDate() const {
        uint16_t year = _time.wYear;
        WORD month = _time.wMonth;
        WORD day = _time.wDay;
        WORD hour = _time.wHour;
        WORD minutes = _time.wMinute;
        WORD seconds = _time.wSecond;

        return JulianJDConverter(static_cast<uint16_t>(year), static_cast<uint8_t>(month), static_cast<uint8_t>(day), static_cast<uint8_t>(hour), static_cast<uint8_t>(minutes), static_cast<uint8_t>(seconds));
    }

    // Uint64 is the time in MicroSeconds !!!
    Time::Time(const microsecondsfromepoch time)
        : _time()
    {
        FILETIME fileTime;
        _ULARGE_INTEGER result;

        result.QuadPart = (time + OffsetTicksForEpoch) * 10;

        fileTime.dwLowDateTime = result.LowPart;
        fileTime.dwHighDateTime = result.HighPart;

        ::FileTimeToSystemTime(&fileTime, &_time);
    }

    Time::Time(const FILETIME& time, bool localTime /*= false*/)
        : _time()
    {
        ::FileTimeToSystemTime(&time, &_time);
        if (localTime) {
            SYSTEMTIME convertedTime;
            TzSpecificLocalTimeToSystemTime(nullptr, &_time, &convertedTime);
            _time = convertedTime;
        }
    }

    Time Time::ToLocal() const {
        FILETIME fileTime, localFileTime;
        SYSTEMTIME local;
        SystemTimeToFileTime(&_time, &fileTime);
        FileTimeToLocalFileTime(&fileTime, &localFileTime);
        FileTimeToSystemTime(&localFileTime, &local);
        return (Time(
            static_cast<uint16_t>(local.wYear), 
            static_cast<uint8_t>(local.wMonth),
            static_cast<uint8_t>(local.wDay),
            static_cast<uint8_t>(local.wHour),
            static_cast<uint8_t>(local.wMinute),
            static_cast<uint8_t>(local.wSecond),
            static_cast<uint16_t>(local.wMilliseconds), false));
    }

    Time Time::ToUTC() const {
        return (Time(
            static_cast<uint16_t>(_time.wYear),
            static_cast<uint8_t>(_time.wMonth),
            static_cast<uint8_t>(_time.wDay),
            static_cast<uint8_t>(_time.wHour),
            static_cast<uint8_t>(_time.wMinute),
            static_cast<uint8_t>(_time.wSecond),
            static_cast<uint16_t>(_time.wMilliseconds), true));
    }

    Time::microsecondsfromepoch Time::Ticks() const
    {
        // Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
        FILETIME fileTime{};
        ::SystemTimeToFileTime(&_time, &fileTime);
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
        if (_time.wDayOfWeek == WORD(~0)) {
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

    string Time::ToTimeOnly(const bool localTime) const {

        TCHAR buffer[32];

        if (!IsValid())
            return string();

        const TCHAR* zone = (localTime == false ? _T(" GMT") : nullptr);

        if (localTime) {
            SYSTEMTIME convertedTime;
            SystemTimeToTzSpecificLocalTime(nullptr, &_time, &convertedTime);
            Time converted(convertedTime, localTime);
            _stprintf(buffer, _T("%02d:%02d:%02d"), converted.Hours(), converted.Minutes(), converted.Seconds());
        } else
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
            _stprintf(buffer, _T("%02d:%02d:%02d"), Hours(), Minutes(), Seconds());
POP_WARNING()

        string value(buffer);
        if( zone != nullptr ) {
            value += zone;
        }

        return (value);
    }

    string Time::ToRFC1123(const bool localTime) const
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        TCHAR buffer[32];

        if (!IsValid())
            return string();

        const TCHAR* zone = (localTime == false ? _T(" GMT") : _T(""));

        if (localTime == true) {
            SYSTEMTIME convertedTime;
            SystemTimeToTzSpecificLocalTime(nullptr, &_time, &convertedTime);
           Time converted(convertedTime, localTime);
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d%s"), converted.WeekDayName(),
                converted.Day(), converted.MonthName(), converted.Year(),
                converted.Hours(), converted.Minutes(), converted.Seconds(), zone);
        } else
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d%s"), WeekDayName(), Day(), MonthName(), Year(),
                Hours(), Minutes(), Seconds(), zone);
POP_WARNING()

        return (string(buffer));
    }

    string Time::ToISO8601(const bool localTime) const
    {
        TCHAR buffer[32];

        if (!IsValid())
            return string();

        const TCHAR* zone = (localTime == false ? _T("Z") : _T(""));

        if (localTime == true) {
            SYSTEMTIME convertedTime;
            SystemTimeToTzSpecificLocalTime(nullptr, &_time, &convertedTime);

            Time converted(convertedTime, localTime);
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
            _stprintf(buffer, _T("%04d-%02d-%02dT%02d:%02d:%02d%s"), converted.Year(), converted.Month(), converted.Day(), converted.Hours(),
                converted.Minutes(), converted.Seconds(), zone);
POP_WARNING()
        } else
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)
            _stprintf(buffer, _T("%04d-%02d-%02dT%02d:%02d:%02d%s"), Year(), Month(), Day(), Hours(),Minutes(), Seconds(), zone);
POP_WARNING()

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

    // Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
    // Licensed under the BSD-3 License"
 

    // GMT specific version of mktime(), taken from BSD source code
    /* Number of days per month (except for February in leap years). */
    static const int monoff[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    static int leap_days(int y1, int y2)
    {
        --y1;
        --y2;
        return (y2 / 4 - y1 / 4) - (y2 / 100 - y1 / 100) + (y2 / 400 - y1 / 400);
    }

    /*
     * Code adapted from Python 2.4.1 sources (Lib/calendar.py).
     */
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

        if (tm->tm_mon > 1 && IsLeapYear(year))
            ++days;
        days += tm->tm_mday - 1;

        hours = days * 24 + tm->tm_hour;
        minutes = hours * 60 + tm->tm_min;
        seconds = minutes * 60 + tm->tm_sec;

        return seconds;
    }

    Time::Time(const struct timespec& time) 
        : _time(time)
    {
    }

    Time Time::ToLocal() const {
        //convert to localtime
        struct tm localtm{}; 
        localtime_r(&_time.tv_sec, &localtm);
        //now convert back to epoch time again but then with the localtime as UTC time...
        struct timespec localasutc{};
        localasutc.tv_sec = mktimegm(&localtm);
        localasutc.tv_nsec = _time.tv_nsec;

        return (Time(localasutc));
    }

    Time Time::ToUTC() const {
        //convert to UTC 
        struct tm utcaslocaltm{}; 
        gmtime_r(&_time.tv_sec, &utcaslocaltm);
        //now convert back to epoch time again 
        struct timespec asutc{};
        asutc.tv_sec = mktimegm(&utcaslocaltm);
        asutc.tv_nsec = _time.tv_nsec;

        return (Time(asutc));
    }

    Time::Time(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second, const uint16_t millisecond, const bool localTime)
        : _time()
    {
        struct tm source{};
        if (IsValidDateTime(year, month, day, hour, minute, second, millisecond) == true) {

            source.tm_year = year - 1900;
            source.tm_mon = month - 1;
            source.tm_mday = day;
            source.tm_hour = hour;
            source.tm_min = minute;
            source.tm_sec = second;
            source.tm_isdst = -1; // make sure dst is calculated automatically

            if (localTime == true) {
                _time.tv_sec = mktime(&source);
            } else {
                _time.tv_sec = mktimegm(&source);
            }

            _time.tv_nsec = millisecond * NanoSecondsPerMilliSecond;
        }
    }

    /**
     * Get day count since Monday, January 1, 4713 BC
     * https://en.wikipedia.org/wiki/Julian_day
     */
    double Time::JulianDate() const {
    
        struct tm source = TMHandle();

        uint16_t year = source.tm_year + 1900;
        uint8_t month = source.tm_mon + 1;
        uint8_t day = source.tm_mday;
        uint8_t hour = source.tm_hour;
        uint8_t minutes = source.tm_min;
        uint8_t seconds = source.tm_sec;

        return JulianJDConverter(year, month, day, hour, minutes, seconds);
    }

    Time::Time(const microsecondsfromepoch time)
        : _time()
    {
        // This is the seconds since 1970...
        time_t epochTimestamp = static_cast<time_t>((time - OffsetTicksForEpoch) / MicroSecondsPerSecond);

        _time.tv_sec = epochTimestamp; 

        _time.tv_nsec = (time % MicroSecondsPerSecond) * NanoSecondsPerMicroSecond;
    }

    Time::microsecondsfromepoch Time::Ticks() const
    {
        return ((static_cast<microsecondsfromepoch>(_time.tv_sec) * MicroSecondsPerSecond ) + (static_cast<microsecondsfromepoch>(_time.tv_nsec)/NanoSecondsPerMicroSecond));
    }

    uint8_t Time::DayOfWeek() const
    {
        return DayOfWeek(TMHandle());
    }

    uint16_t Time::DayOfYear() const
    {
        return DayOfYear(TMHandle());
    }

    string Time::ToRFC1123(const bool localTime) const
    {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        TCHAR buffer[32];
        const TCHAR* zone = (localTime == false) ? _T(" GMT") : _T("");

        if (!IsValid())
            return string();

        if (localTime == true) {
            struct tm localTime{};
            localtime_r(&_time.tv_sec, &localTime);
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d%s"), WeekDayName(localTime),
                Day(localTime), MonthName(localTime), Year(localTime),
                Hours(localTime), Minutes(localTime), Seconds(localTime), zone);
        } else {
            struct tm utcTime = TMHandle();
            _stprintf(buffer, _T("%s, %02d %s %04d %02d:%02d:%02d%s"), WeekDayName(utcTime), Day(utcTime), MonthName(utcTime), Year(utcTime), Hours(utcTime),
                Minutes(utcTime), Seconds(utcTime), zone);
        }

        return (string(buffer));
    }

    string Time::ToISO8601(const bool localTime) const
    {
        TCHAR buffer[32];
        const TCHAR* zone = (localTime == false) ? _T("Z") : _T("");

        if (!IsValid())
            return string();

        if (localTime == true) {
            struct tm localTime{};
            localtime_r(&_time.tv_sec, &localTime);
            _stprintf(buffer, _T("%04d-%02d-%02dT%02d:%02d:%02d%s"), Year(localTime), Month(localTime), Day(localTime), Hours(localTime),
                Minutes(localTime), Seconds(localTime), zone);
        } else {
            struct tm utcTime = TMHandle();
            _stprintf(buffer, _T("%04d-%02d-%02dT%02d:%02d:%02d%s"), Year(utcTime), Month(utcTime), Day(utcTime), Hours(utcTime),Minutes(utcTime),
                Seconds(utcTime), zone);
        }

        return (string(buffer));
    }

    string Time::ToTimeOnly(const bool localTime) const {
        // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
        TCHAR buffer[32];
        const TCHAR* zone = (localTime == false) ? _T("GMT") : nullptr;

        if (!IsValid())
            return string();

        if (localTime == true) {
            struct tm localTime{};
            localtime_r(&_time.tv_sec, &localTime);
            _stprintf(buffer, _T("%02d:%02d:%02d"), Hours(localTime), Minutes(localTime), Seconds(localTime));
        } else {
            struct tm utcTime = TMHandle();
            _stprintf(buffer, _T("%02d:%02d:%02d"), Hours(utcTime), Minutes(utcTime), Seconds(utcTime));
        }

        string value(buffer);
        if( zone != nullptr ) {
            value += zone;
        }

        return (value);
    }

    string Time::Format(const TCHAR* formatter) const
    {
        TCHAR buffer[200];

        struct tm tmtime = TMHandle(); // cannot get address from rvalue;
        _tcsftime(buffer, sizeof(buffer), formatter, &tmtime);

        return (string(buffer));
    }

    /* static */ Time Time::Now()
    {
        struct timespec currentTime{};
        clock_gettime(CLOCK_REALTIME, &currentTime);

        return (Time(currentTime));
    }


    struct tm Time::TMHandle() const 
    {
        struct tm tmtime{};
        gmtime_r(&_time.tv_sec, &tmtime);
        return tmtime;
    }

#endif

    string Time::ToRFC1123() const
    {
        return ToRFC1123(false);
    }

    string Time::ToISO8601() const
    {
        return ToISO8601(false);
    }

    Time& Time::Add(const uint32_t timeInMilliseconds)
    {
        // Calculate the new time !!
        uint64_t newTime = Ticks() + static_cast<uint64_t>(timeInMilliseconds) * MilliSecondsPerSecond;
        return (operator=(Time(newTime)));
    }

    Time& Time::Sub(const uint32_t timeInMilliseconds)
    {
        // Calculate the new time !!
        uint64_t newTime = Ticks() - static_cast<uint64_t>(timeInMilliseconds) * MilliSecondsPerSecond;
        return (operator=(Time(newTime)));
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

    inline bool Time::IsValidDate(const uint16_t year, const uint8_t month, const uint8_t day) const
    {
        uint8_t totalDays = 0;
        switch (month - 1) {
        case Month::Jan:
        case Month::Mar:
        case Month::May:
        case Month::July:
        case Month::Aug:
        case Month::Oct:
        case Month::Dec:
            totalDays = 31;
            break;

        case Month::Feb:
            totalDays = ((IsLeapYear(year) == true) ? 29 : 28);
            break;
        case Month::Apr:
        case Month::June:
        case Month::Sept:
        case Month::Nov:
            totalDays = 30;
            break;
        default:
            break;
        }
        return (((day > 0) && (day <= totalDays)) ? true : false);
    }
}
} // namespace Core
