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
 
#ifndef __TIME_H
#define __TIME_H

#include "Portability.h"
#include "TextFragment.h"

namespace Thunder {
namespace Core {
 
    class EXTERNAL Time {
        friend class TimeAsLocal;

    private:
        enum Month {
            Jan,
            Feb,
            Mar,
            Apr,
            May,
            June,
            July,
            Aug,
            Sept,
            Oct,
            Nov,
            Dec
        };

    public:
        static constexpr uint32_t MilliSecondsPerSecond = 1000;
        static constexpr uint32_t MicroSecondsPerMilliSecond = 1000;
        static constexpr uint32_t NanoSecondsPerMicroSecond = 1000;
        static constexpr uint32_t NanoSecondsPerMilliSecond = MicroSecondsPerMilliSecond * MicroSecondsPerMilliSecond;
        static constexpr uint32_t MicroSecondsPerSecond = MilliSecondsPerSecond * MicroSecondsPerMilliSecond;

        // Difference in Seconds between UNIX and NTP epoch (25567).
        static constexpr uint32_t SecondsPerMinute = 60;
        static constexpr uint32_t MinutesPerHour = 60;
        static constexpr uint32_t HoursPerDay = 24;
        static constexpr uint32_t SecondsPerHour = SecondsPerMinute * MinutesPerHour;
        static constexpr uint32_t SecondsPerDay = SecondsPerHour * HoursPerDay;

        static constexpr uint32_t TicksPerMillisecond = 1000;

        // relative to 1st of January 1970, 00:00:00 in microseconds
        using microsecondsfromepoch = uint64_t;

#ifdef __POSIX__

public:
        //time always in UTC
        Time(const struct timespec& time);

#endif
        Time(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t seconds, const uint16_t milliseconds, const bool localTime);

#ifdef __WINDOWS__
        Time(const FILETIME& time, bool localTime = false);
        Time(const SYSTEMTIME& time, bool localTime = false)
            : _time(time)
        {
        }
#endif

        Time(const microsecondsfromepoch time);

#ifdef __WINDOWS__
        Time()
        {
            _time.wYear = 0;
            _time.wMonth = 0;
            _time.wDay = 0;
            _time.wHour = 0;
            _time.wMinute = 0;
            _time.wSecond = 0;
            _time.wMilliseconds = 0;
            _time.wDayOfWeek = 0;
        }
        bool IsValid() const
        {
            return ((_time.wYear != 0) && (_time.wMonth != 0) && (_time.wDay != 0));
        }
#endif

#ifdef __POSIX__
        Time()
            : _time{}
        {
        }

        bool IsValid() const
        {
            return (_time.tv_sec != 0);
        }
#endif

        Time(const Time& copy)
            : _time(copy._time)
        {
        }

        Time(Time&& move)
            : _time(std::move(move._time))
        {
        }

        ~Time() = default;

        Time& operator=(const Time& RHS)
        {
            _time = RHS._time;

            return (*this);
        }

        Time& operator=(Time&& move)
        {
            if (this != &move) {
                _time = std::move(move._time);
            }
            return (*this);
        }

    public:
        bool FromString(const string& buffer, const bool localTime = true)
        {
            if (buffer.size() >= 18) {
                if (buffer[10] == 'T') {
                    // 1994-11-06T08:49:37Z
                    return (FromISO8601(buffer));
                } else if (::isspace(buffer[3]) != 0) {
                    // Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format [18]
                    return (FromANSI(buffer, localTime));
                } else if (buffer[3] == ',') {
                    // Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
                    return (FromRFC1123(buffer));
                } else {
                    // Sunday, 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
                    return (FromRFC1036(buffer));
                }
            }

            return (false);
        }

        void ToString(string& text, const bool localTime = true) const
        {
            text = ToRFC1123(localTime);
        }
        const TCHAR* WeekDayName() const;
        const TCHAR* MonthName() const;

        uint32_t MilliSeconds() const
        {
#ifdef __WINDOWS__
            return (_time.wMilliseconds);
#else
            return ((_time.tv_nsec / NanoSecondsPerMilliSecond) % 1000);  
#endif
        }
        uint8_t Seconds() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint8_t>(_time.wSecond));
#else
            return Seconds(TMHandle());
#endif
        }
        uint8_t Minutes() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint8_t>(_time.wMinute));
#else
            return Minutes(TMHandle());
#endif
        }
        uint8_t Hours() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint8_t>(_time.wHour));
#else
            return Hours(TMHandle());
#endif
        }
        uint8_t Day() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint8_t>(_time.wDay));
#else
            return Day(TMHandle());
#endif
        }
        uint8_t Month() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint8_t>(_time.wMonth));
#else
            return Month(TMHandle());
#endif
        }
        uint32_t Year() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint32_t>(_time.wYear));
#else
            return Year(TMHandle());
#endif
        }

        uint8_t DayOfWeek() const;
        uint16_t DayOfYear() const;
        uint64_t NTPTime() const;

        double JulianDate() const;
        double JulianJDConverter(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minutes, const uint8_t seconds) const {
            // julian day number algorithm
            uint32_t JDN = (1461 * (year + 4800 + (month - 14) / 12)) / 4 + (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12 - (3 * ((year + 4900 + (month - 14) / 12) / 100)) / 4 + day - 32075;
            double time = (((hour - 12) * 60 * 60) + (minutes * 60) + seconds) / (static_cast<float>(24 * 60 * 60));

            // JDN to JD algorithm
            return (JDN + time);
        }

        // UTC time
        microsecondsfromepoch Ticks() const;

        bool FromRFC1123(const string& buffer);
        bool FromRFC1036(const string& buffer);
        bool FromANSI(const string& buffer, const bool localTime);
        bool FromISO8601(const string& buffer);
        string ToRFC1123() const;
        string ToRFC1123(const bool localTime) const;
        string ToISO8601() const;
        string ToISO8601(const bool localTime) const;
        string ToTimeOnly(const bool localTime) const;

        static Time Now();
        static bool FromString(const string& buffer, const bool localTime, Time& element)
        {
            return (element.FromString(buffer, localTime));
        }
        static bool ANSI(const string& buffer, const bool localTime, Time& element)
        {
            return (element.FromANSI(buffer, localTime));
        }
        static bool RFC1123(const string& buffer, Time& element)
        {
            return (element.FromRFC1123(buffer));
        }
        static bool RFC1036(const string& buffer, Time& element)
        {
            return (element.FromRFC1036(buffer));
        }
        static bool ISO8601(const string& buffer, Time& element)
        {
            return (element.FromISO8601(buffer));
        }

        Time& Add(const uint32_t timeInMilliseconds);
        Time& Sub(const uint32_t timeInMilliseconds);

        string Format(const TCHAR* formatter) const;

        bool operator<(const Time& rhs) const
        {
            return (Ticks() < rhs.Ticks());
        }
        bool operator<=(const Time& rhs) const
        {
            return (Ticks() <= rhs.Ticks());
        }
        bool operator>(const Time& rhs) const
        {
            return (Ticks() > rhs.Ticks());
        }
        bool operator>=(const Time& rhs) const
        {
            return (Ticks() >= rhs.Ticks());
        }
        bool operator==(const Time& rhs) const
        {
            return (Ticks() == rhs.Ticks());
        }
        bool operator!=(const Time& rhs) const
        {
            return (!(operator==(rhs)));
        }
        Time operator-(const Time& rhs) const
        {
            return (Time(Ticks() - rhs.Ticks()));
        }
        Time operator+(const Time& rhs) const
        {
            return (Time(Ticks() + rhs.Ticks()));
        }
        Time& operator-=(const Time& rhs)
        {
            return (operator=(Time(Ticks() - rhs.Ticks())));
        }
        Time& operator+=(const Time& rhs)
        {
            return (operator=(Time(Ticks() + rhs.Ticks())));
        }

#ifdef __WINDOWS__
        const SYSTEMTIME& Handle() const
        {
            return (_time);
        }
#else
        const struct timespec& Handle() const
        {
            return (_time);
        }
#endif

    private:

#ifdef __POSIX__

        static const TCHAR* WeekDayName(const struct tm& time);
        static const TCHAR* MonthName(const struct tm& time);
        static uint8_t Seconds(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_sec));
        }
        static uint8_t Minutes(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_min));
        }
        static uint8_t Hours(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_hour));
        }
        static uint8_t Day(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_mday));
        }
        static uint8_t Month(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_mon + 1));
        }
        static uint32_t Year(const struct tm& time)
        {
            return (static_cast<uint32_t>(time.tm_year + 1900));
        }
        static uint8_t DayOfWeek(const struct tm& time)
        {
            return (static_cast<uint8_t>(time.tm_wday));
        }
        static uint16_t DayOfYear(const struct tm& time)
        {
            return (static_cast<uint16_t>(time.tm_yday));
        }
        struct tm TMHandle() const;

#endif
        bool IsValidDateTime(const uint16_t year, const uint8_t month, const uint8_t day, const uint8_t hour, const uint8_t minute, const uint8_t second, const uint16_t millisecond) const
        {
            return (IsValidDate(year, month, day) && (hour < 24) &&
                (minute < 60) && (second < 60) && (millisecond < 1000));
        }

        bool IsValidDate(const uint16_t year, const uint8_t month, const uint8_t day) const;

        Time ToLocal() const;
        Time ToUTC() const;

    private:
#ifdef __WINDOWS__
        mutable SYSTEMTIME _time;
#else
        struct timespec _time;
#endif

    };

    class EXTERNAL TimeAsLocal {
    public:

        // creates a TimeAsLocal object with time stored in local time internally in stead of UTC as with the Time class
        TimeAsLocal(const Time& time) : _time(time.ToLocal())
        {
        }
        TimeAsLocal& operator=(const Time& time)
        {
            _time = time.ToLocal();
            return *this;
        }

        TimeAsLocal() = default;
        TimeAsLocal(TimeAsLocal&&) = default;
        TimeAsLocal(const TimeAsLocal&) = default;
        TimeAsLocal& operator=(TimeAsLocal&&) = default;
        TimeAsLocal& operator=(const TimeAsLocal&) = default;

        ~TimeAsLocal() = default;

        // returns a Time object that holds the UTC time again internally converted from the Local time in the TimeAsLocal
        // instance this method is called upon
        Time ToUTC() const 
        {
            return _time.ToUTC();
        }

        // operations
        void ToString(string& text) const
        {
            _time.ToString(text, false);
        }
        const TCHAR* WeekDayName() const 
        {
            return _time.WeekDayName();
        }
        const TCHAR* MonthName() const
        {
            return _time.MonthName();
        }
        uint32_t MilliSeconds() const
        {
            return _time.MilliSeconds();
        }
        uint8_t Seconds() const
        {
            return _time.Seconds();
        }
        uint8_t Minutes() const
        {
            return _time.Minutes();
        }
        uint8_t Hours() const
        {
            return _time.Hours();
        }
        uint8_t Day() const
        {
            return _time.Day();
        }
        uint8_t Month() const
        {
            return _time.Month();
        }
        uint32_t Year() const
        {
            return _time.Year();
        }
        uint8_t DayOfWeek() const
        {
            return _time.DayOfWeek();
        }
        uint16_t DayOfYear() const
        {
            return _time.DayOfYear();
        }
        uint64_t NTPTime() const
        {
            return _time.NTPTime();
        }
        double JulianDate() const
        {
            return _time.JulianDate();
        }
        TimeAsLocal& Add(const uint32_t timeInMilliseconds)
        {
            uint64_t newTime = Ticks() + static_cast<uint64_t>(timeInMilliseconds) * Time::MilliSecondsPerSecond;
            return (operator=(TimeAsLocal(newTime)));
        }
        TimeAsLocal& Sub(const uint32_t timeInMilliseconds)
        {
            uint64_t newTime = Ticks() - static_cast<uint64_t>(timeInMilliseconds) * Time::MilliSecondsPerSecond;
            return (operator=(TimeAsLocal(newTime)));
        }
        Time::microsecondsfromepoch Ticks() const
        {
            return _time.Ticks();
        }
        string ToRFC1123() const
        {
            return _time.ToRFC1123(false);
        }
        string ToISO8601() const
        {
            return _time.ToISO8601(false);
        }
        string ToTimeOnly() const
        {
            return _time.ToTimeOnly(false);
        }
        bool IsValid() const
        {
            return _time.IsValid();
        }

        // operators
        bool operator<(const TimeAsLocal& rhs) const
        {
            return (_time < rhs._time);
        }
        bool operator<=(const TimeAsLocal& rhs) const
        {
            return (_time <= rhs._time);
        }
        bool operator>(const TimeAsLocal& rhs) const
        {
            return (_time > rhs._time);
        }
        bool operator>=(const TimeAsLocal& rhs) const
        {
            return (_time >= rhs._time);
        }
        bool operator==(const TimeAsLocal& rhs) const
        {
            return (_time == rhs._time);
        }
        bool operator!=(const TimeAsLocal& rhs) const
        {
            return (!(operator==(rhs)));
        }
        TimeAsLocal operator-(const TimeAsLocal& rhs) const
        {
            return TimeAsLocal(Ticks() - rhs.Ticks());
        }
        TimeAsLocal operator+(const Time& rhs) const
        {
            return TimeAsLocal(Ticks() + rhs.Ticks());
        }
        TimeAsLocal& operator-=(const TimeAsLocal& rhs)
        {
            return (operator=(TimeAsLocal(Ticks() - rhs.Ticks())));
        }
        TimeAsLocal& operator+=(const TimeAsLocal& rhs)
        {
            return (operator=(TimeAsLocal(Ticks() + rhs.Ticks())));
        }

        // just in case this is needed as a Time object but be carefull as it returns a Time with the Internal time in LOcalTime not UTC!
        explicit operator Time&()
        {
            return _time;
        }
        explicit operator const Time&() const 
        {
            return _time;
        }

    private:
        TimeAsLocal(const Time::microsecondsfromepoch time) : _time(time)
        {
        }

    private:
        Time _time;
    };

}
} // namespace Core

#endif // __TIME_H
