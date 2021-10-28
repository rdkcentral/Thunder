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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;
static constexpr uint32_t MilliSecondsPerSecond = 1000;
static constexpr uint32_t MicroSecondsPerMilliSecond = 1000;
static constexpr uint32_t MicroSecondsPerSecond = MilliSecondsPerSecond * MicroSecondsPerMilliSecond;

TEST(Core_Time, MilliSeconds)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.MilliSeconds(), 21);
    time = Time(2000, 1, 2, 11, 30, 23, 100, false);
    EXPECT_EQ(time.MilliSeconds(), 100);
    time = Time(2000, 1, 2, 11, 30, 23, 1023, false);
    EXPECT_EQ(time.MilliSeconds(), 23);
    time = Time(2000, 1, 2, 11, 30, 23, 4999, false);
    EXPECT_EQ(time.Seconds(), 27);
    EXPECT_EQ(time.MilliSeconds(), 999);
}
TEST(Core_Time, MilliSeconds_LocalTimeEnabled)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, true);
    EXPECT_EQ(time.MilliSeconds(), 21);
    time = Time(2000, 1, 2, 11, 30, 23, 100, true);
    EXPECT_EQ(time.MilliSeconds(), 100);
    time = Time(2000, 1, 2, 11, 30, 23, 1023, true);
    EXPECT_EQ(time.MilliSeconds(), 23);
    time = Time(2000, 1, 2, 11, 30, 23, 10999, true);
    EXPECT_EQ(time.Seconds(), 33);
    EXPECT_EQ(time.MilliSeconds(), 999);
}
TEST(Core_Time, Seconds)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(1970, 2, 255, 12, 45, 60, 21, false);
    EXPECT_EQ(time.Seconds(), 0);
    time =  Time(2000, 1, 24, 14, 61, 59, 21, false);
    EXPECT_EQ(time.Seconds(), 59);
    time =  Time(2021, 4, 30, 0, 30, 66, 21, false);
    EXPECT_EQ(time.Seconds(), 6);
    time =  Time(2021, 2, 32, 23, 0, 0, 21, false);
    EXPECT_EQ(time.Seconds(), 0);
    time =  Time(2021, 2, 31, 24, 66, 70, 21, false);
    EXPECT_EQ(time.Seconds(), 10);
}
TEST(Core_Time, Seconds_LocalTimeEnabled)
{
    Time time(2000, 1, 2, 11, 66, 66, 21, true);
    EXPECT_EQ(time.Seconds(), 6);
    time =  Time(1970, 2, 255, 12, 0, 23, 21, true);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(2000, 1, 2, 15, 70, 60, 21, true);
    EXPECT_EQ(time.Seconds(), 0);
    time =  Time(2021, 4, 30, 0, 30, 59, 21, true);
    EXPECT_EQ(time.Seconds(), 59);
    time =  Time(2021, 2, 3, 22, 60, 23, 21, true);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(2021, 2, 66, 24, 33, 78, 21, true);
    EXPECT_EQ(time.Seconds(), 18);
}
TEST(Core_Time, Minutes)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(1971, 2, 255, 12, 45, 23, 21, false);
    EXPECT_EQ(time.Minutes(), 45);
    time =  Time(2000, 1, 23, 14, 61, 23, 21, false);
    EXPECT_EQ(time.Minutes(), 1);
    time =  Time(2021, 4, 30, 0, 30, 66, 21, false);
    EXPECT_EQ(time.Minutes(), 31);
    time =  Time(2021, 2, 6, 23, 0, 23, 21, false);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2021, 2, 26, 24, 66, 66, 21, false);
    EXPECT_EQ(time.Minutes(), 7);
    time =  Time(1981, 1, 36, 26, 60, 23, 21, false);
    EXPECT_EQ(time.Minutes(), 0);
}
TEST(Core_Time, Minutes_LocalTimeEnabled)
{
    Time time(2000, 1, 2, 11, 66, 66, 21, true);
    EXPECT_EQ(time.Minutes(), 7);
    time =  Time(1971, 2, 255, 12, 0, 23, 21, true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2000, 1, 20, 15, 70, 23, 21, true);
    EXPECT_EQ(time.Minutes(), 10);
    time =  Time(2021, 4, 30, 0, 30, 23, 21, true);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(2021, 2, 36, 22, 60, 23, 21, true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2021, 2, 3, 24, 33, 78, 21, true);
    EXPECT_EQ(time.Minutes(), 34);
    time =  Time(1981, 1, 37, 26, 59, 66, 21, true);
    EXPECT_EQ(time.Minutes(), 0);
}
TEST(Core_Time, Hours)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 11);
    time =  Time(1971, 2, 255, 12, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 12);
    time =  Time(2000, 1, 26, 14, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 14);
    time =  Time(2021, 4, 30, 0, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 0);
    time =  Time(2021, 2, 31, 23, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 23);
    time =  Time(2021, 2, 14, 24, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 0);
    time =  Time(1981, 1, 17, 26, 30, 23, 21, false);
    EXPECT_EQ(time.Hours(), 2);
}
TEST(Core_Time, Hours_LocalTimeEnabled)
{
    Time time(2000, 1, 2, 11, 66, 66, 21, true);
    EXPECT_EQ(time.Hours(), 12);
    time =  Time(1971, 2, 255, 12, 0, 23, 21, true);
    EXPECT_EQ(time.Hours(), 13);
    time =  Time(2000, 1, 21, 15, 70, 23, 21, true);
    EXPECT_EQ(time.Hours(), 16);
    time =  Time(2021, 4, 30, 0, 30, 23, 21, true);
    EXPECT_EQ(time.Hours(), 1);
    time =  Time(2021, 1, 10, 22, 30, 23, 21, true);
    EXPECT_EQ(time.Hours(), 22);
    time =  Time(2021, 2, 31, 24, 30, 23, 21, true);
    EXPECT_EQ(time.Hours(), 0);
}
TEST(Core_Time, Day)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 2);
    time =  Time(1971, 2, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 13);
    time =  Time(2000, 1, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 30);
    time =  Time(2021, 2, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 2);
    time =  Time(2021, 2, 10, 24, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 11);
    time =  Time(1981, 1, 32, 25, 30, 23, 21, false);
    EXPECT_EQ(time.Day(), 2);
}
TEST(Core_Time, Day_LocalTimeEnabled)
{
    Time time(2000, 1, 4, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 4);
    time =  Time(1971, 2, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 13);
    time =  Time(2000, 1, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 31);
    time =  Time(2021, 4, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 1);
    time =  Time(2021, 2, 33, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 5);
    time =  Time(1981, 1, 25, 48, 30, 23, 21, true);
    EXPECT_EQ(time.Day(), 27);
}
TEST(Core_Time, WeekDayName)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Sun");
    time =  Time(1971, 2, 255, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Wed");
    time =  Time(2000, 1, 34, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Thu");
    time =  Time(2021, 4, 32, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Sun");
    time =  Time(2021, 2, 29, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
    time =  Time(1981, 1, 13, 0, 30, 23, 21, false);
    EXPECT_STREQ(time.WeekDayName(), "Tue");
}
TEST(Core_Time, WeekDayName_LocalTimeEnabled)
{
    Time time(2000, 1, 3, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
    time =  Time(1971, 2, 254, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Tue");
    time =  Time(2000, 3, 31, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Fri");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Fri");
    time =  Time(2021, 2, 28, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Sun");
    time =  Time(1981, 1, 32, 24, 30, 23, 21, true);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
}
TEST(Core_Time, Month)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1971, 1, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(2000, 1, 32, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 2);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(2021, 2, 29, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 3);
    time =  Time(1981, 4, 30, 25, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 5);
}
TEST(Core_Time, Month_LocalTimeEnabled)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1971, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(2000, 5, 33, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 6);
    time =  Time(2021, 1,25, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(2021, 6, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 7);
    time =  Time(1981, 7, 30, 48, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 8);
}
TEST(Core_Time, MonthName)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1971, 3, 255, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Nov");
    time =  Time(2000, 5, 32, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Jun");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Apr");
    time =  Time(2021, 1, 12, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1981, 2, 27, 64, 64, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Mar");
}
TEST(Core_Time, MonthName_LocalTimeEnabled)
{
    Time time(2000, 1, 31, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1971, 2, 255, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Oct");
    time =  Time(2000, 8, 100, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Nov");
    time =  Time(2021, 9, 30, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Sep");
    time =  Time(2021, 10, 32, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Nov");
    time =  Time(1981, 2, 26, 80, 30, 23, 21, true);
}
TEST(Core_Time, Year)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(1971, 1, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 1971);
    time =  Time(2000, 1, 10, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2021);
    time =  Time(2021, 12, 33, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2022);
    time =  Time(1981, 9, 140, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 1982);
}
TEST(Core_Time, Year_LocalTimeEnabled)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(1971, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 1971);
    time =  Time(2000, 1, 200, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2021);
    time =  Time(2021, 12, 32, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2022);
    time =  Time(1981, 10, 100, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 1982);
}
TEST(Core_Time, DayOfYear)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 1, 0, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 364);
    time =  Time(2000, 1, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 254);
    time =  Time(2000, 2, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 60);
    time =  Time(2000, 12, 32, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 12, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 364);
}
TEST(Core_Time, DayOfYear_LocalTimeEnabled)
{
    Time time(2002, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 1, 0, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 364);
    time =  Time(2000, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 254);
    time =  Time(2000, 2, 29, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 59);
    time =  Time(2000, 12, 32, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 11, 67, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 5);
}
TEST(Core_Time, DayOfWeek)
{
    // Thu Jan  1 01:01:01 2004
    Time time(2004, 1, 1, 1, 1, 1, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jan  1 11:30:24 2004
    time = Time(2004, 1, 1, 11, 30, 24, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jun 20 20:20:22 2019
    time =  Time(2019, 6, 20, 20, 20, 22, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Mon Jul  1 13:40:33 2019
    time =  Time(2019, 6, 31, 13, 40, 33, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Mon Aug 31 11:50:31 2015
    time =  Time(2015, 8, 31, 11, 50, 31, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Tue Sep  1 11:50:23 2015
    time =  Time(2015, 8, 32, 11, 50, 23, 21, false);
    EXPECT_EQ(time.DayOfWeek(), 2);
}
TEST(Core_Time, DayOfWeek_LocalTimeEnabled)
{
    // Thu Jan  1 01:01:01 2004
    Time time(2004, 1, 1, 1, 1, 1, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jan  1 11:30:24 2004
    time = Time(2004, 1, 1, 11, 30, 24, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jun 20 20:20:22 2019
    time =  Time(2019, 6, 20, 20, 20, 22, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Mon Jul  1 13:40:33 2019
    time =  Time(2019, 6, 31, 13, 40, 33, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Mon Aug 31 11:50:31 2015
    time =  Time(2015, 8, 31, 11, 50, 31, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Tue Sep  1 11:50:23 2015
    time =  Time(2015, 8, 32, 11, 50, 23, 21, true);
    EXPECT_EQ(time.DayOfWeek(), 2);
}
TEST(Core_Time, FromString)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    time.ToString(timeString1);

    // LocalTime default (true)
    EXPECT_EQ(time.FromString(timeString1), true);
    time.ToString(timeString2);
    //FIXME: EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    // LocalTime true
    EXPECT_EQ(time.FromString(timeString2, true), true);
    time.ToString(timeString1);
    //FIXME: EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
    // LocalTime false

    EXPECT_EQ(time.FromString(timeString1, false), true);
    time.ToString(timeString1);
    EXPECT_EQ(time.FromString(timeString2, false), true);
    time.ToString(timeString2);
    //FIXME: EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
}
TEST(Core_Time, FromString_ANSI)
{
    Time time;
    std::string timeString;
    // LocalTime true
    EXPECT_EQ(time.FromString("Sun Nov 6 08:49:37 1994", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37 ");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sun Nov 6 12:49:37 1994", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 04:49:37 ");
}
TEST(Core_Time, FromString_ISO8601)
{
    Time time;
    std::string timeString;
    // LocalTime true
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
    // LocalTime false
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
}
TEST(Core_Time, FromString_RFC1036)
{
    Time time;
    std::string timeString;
    // LocalTime true
    EXPECT_EQ(time.FromString("Sunday, 06-Nov-94 08:49:37 GMT", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sunday, 06-Nov-94 08:49:37 GMT", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
}
TEST(Core_Time, FromString_RFC1123)
{
    Time time;
    std::string timeString;
    // LocalTime true
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 08:49:37 GMT", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 08:49:37 GMT", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 00:49:37 ");
}
TEST(Core_Time, FromStandard_ANSI_ValidFormat)
{
    Time time(Time::Now());
    EXPECT_EQ(time.FromANSI("2021Feb05 23:47:20", true), true);
    EXPECT_EQ(time.FromANSI("2021Feb05 23:47:20", false), true);
    EXPECT_EQ(time.FromANSI("1994Jan06 08:49:37.123+06:45", true), true);
    EXPECT_EQ(time.FromANSI("1994Jan06 08:49:37.123+06:45", false), true);
    EXPECT_EQ(time.FromANSI("1994Jan608:49:37.123+06:45", true), true);
    EXPECT_EQ(time.FromANSI("1994Jan6 8:49:37.123+06:45", true), true);
    EXPECT_EQ(time.FromANSI("1994Jan6 8:9:7.123+06:45", true), true);
    EXPECT_EQ(time.FromANSI("1994Jan0608:49:37.123+06:45", true), true);
    EXPECT_EQ(time.FromANSI("1994Mar0608:49:5.123+06:45 GMT", true), true);
    EXPECT_EQ(time.FromANSI("1994Mar0608:49:5.123+06:45 UTC", true), true);
    EXPECT_EQ(time.FromANSI("1994March0608:49:5.123+06:45 UTC", true), true);
}
TEST(Core_Time, FromStandard_ANSI_InvalidFormat)
{
    Time time(Time::Now());
    std::string timeString;
    time.ToString(timeString);

    // ToString is in RFC1123 format, hence it should fail
    EXPECT_EQ(time.FromANSI(timeString, true), false);
    EXPECT_EQ(time.FromANSI(timeString, false), false);

    EXPECT_EQ(time.FromANSI("1994Jan06T0a:49:37.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994Jan06T0a:49:37.123+06:45", false), false);
    EXPECT_EQ(time.FromANSI("1994Feb06T08:4a:37.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994Mar0aT08:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("T1994Mar0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994TMar0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994No1v0608:49:5.123+06:45 UTC", true), false);
    EXPECT_EQ(time.FromANSI("1994No0608:49:5.123+06:45 UTC", true), false);

    /* FIXME: Cross check the below TCs are valid case or not 
     * If date/time/zone preceded with any character it get discarded (character used: TEST)
    EXPECT_EQ(time.FromANSI("1994Jan06TEST08:49:37.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994Jan0608:49:37.803+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994MarTEST0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994Mar608:49:5TEST.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("1994Mar608:49:5.123+06:45 TEST GMT", true), false);
    */
}
TEST(Core_Time, FromStandard_ISO8601_ValidFormat)
{
    Time time(Time::Now());
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37+01"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37+06:45"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37+06:45 GMT"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37+06:45 UTC"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T8:49:37+06:45"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:9:37+06:45"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:7+06:45"), true);
    EXPECT_EQ(time.FromISO8601("2094-11-06T08:49:37+06:45"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37+06:45"), true);
    /*
     * FIXME: fractions of second/hour/minute are expected to work
     * currently it is failing with parsing
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123Z"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+01"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06:45"), true);
    */
    // FIXME: expecting this case should work: minute and second with single digit
    //EXPECT_EQ(time.FromISO8601("1994-11-06T08:9:7+06:45"), true);
}
TEST(Core_Time, FromStandard_ISO8601_InvalidFormat)
{
    Time time(Time::Now());
    std::string timeString;

    // ToString is in RFC1123 format, hence it should fail
    EXPECT_EQ(time.FromISO8601(timeString), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-12-06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-13-06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-12-06T08:49:37.123 06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123-06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37."), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06 45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+24:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+24:4"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06:45:"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06 T08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T 08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-0608:49:37+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-Jan-06T08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("94-11-06T08:49:37+06:45"), false);
    EXPECT_EQ(time.FromISO8601("100-11-06T08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("1000-11-06T08:49:37.123a"), false);
    EXPECT_EQ(time.FromISO8601("1994-Apr-06T08:49:37+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-April-06T08:49:37+06:45"), false);
}
TEST(Core_Time, FromStandard_RFC1036_ValidFormat)
{
    Time time(Time::Now());

    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-94 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:37 UTC"), true);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:37"), true);
    EXPECT_EQ(time.FromRFC1036("Mon, 06-Nov-1994 08:49:37"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 08:49:37"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:07"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:07"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:7 "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:7"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 GMT"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 GMT "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 UTC"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Apr-2094 8:9:7 GMT"), true);
    // FIXME: expect this also to be success: EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7"), true);

    // FIXME IRFC1036 working only if we precede the data with 4 characters with a seperator
    EXPECT_EQ(time.FromRFC1036("XXXX, 06-Nov-08:11:49:37 "), true);
    EXPECT_EQ(time.FromRFC1036("XXXX,06-Nov-08:11:49:37 "), true);
    EXPECT_EQ(time.FromRFC1036("XXXX-06-Nov-08:11:49:37 "), true);
    EXPECT_EQ(time.FromRFC1036("XXXX:06-Nov-08:11:49:37 "), true);
}
TEST(Core_Time, FromStandard_RFC1036_InvalidFormat)
{
    Time time(Time::Now());
    std::string timeString;
    time.ToString(timeString, true);

    // ToString is in RFC1123 format, hence it should fail
    EXPECT_EQ(time.FromRFC1036(timeString), false);

    EXPECT_EQ(time.FromRFC1036("Wed, 27 Oct 2021 00:05:01 "), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06Nov94 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06:Nov:94 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1036("1994-06-Nov 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 1994-06-Nov 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1036("19941105T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:TEST49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:TEST37"), false);
    EXPECT_EQ(time.FromRFC1036("1994-11-06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1036("1994-11-Jan-08:49:07+06:45"), false);
    EXPECT_EQ(time.FromRFC1036("1994Jan07T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1036("1994-Jan-09T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1036("Monday, 07-November-2014 08:49:37 GMT"), false);

    /* FIXME: Cross check the below TCs are valid case or not
     * If date/year/time/zone preceded with any character it get discard
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-TEST1994 08:49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, TEST06-Nov-1994 08:49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 TEST08:49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:37 TEST GMT"), false);
    */
}
TEST(Core_Time, FromStandard_RFC1123_ValidFormat)
{
    Time time;
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov199408:49:37GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov199408:49:37"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 ff 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994ff 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 "), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 FF "), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sunday, 06 Nov 1994 08:59:27 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994:08:49:37 GMT"), true);
}
TEST(Core_Time, FromStandard_RFC1123_InvalidFormat)
{
    Time time;
    EXPECT_EQ(time.FromRFC1123("1994Jan06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1123("1994Jan0aT08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 0f:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08f:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:ff49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:49f:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:49:f37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06-Nov-1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("06Nov1994:::08:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("Sun,Nov1994:::08:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("06 Nov 1994 08:59:27 GMT"), false);
    /* FIXME: Cross check the below TCs are valid case or not
     * If date/month/year/time/zone preceded with any character it get discard
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov1994TEST8:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:49:37 ff GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov1994TEST8:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("Sund, 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("TEST, 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, TEST 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov TEST 1994 08:49:37 GMT"), false);
    */

}
TEST(Core_Time, FromStandard_RFC1123_TimeStringCreatedFromTime)
{
    Time time =  Time(2018, 1, 23, 11, 30, 23, 21, false);
    std::string timeString;
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Tue, 23 Jan 2018 11:30:23 GMT"));
 
    time =  Time(2018, 2, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Fri, 23 Feb 2018 11:30:23 GMT"));

    time =  Time(2018, 3, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Fri, 23 Mar 2018 11:30:23 GMT"));

    time =  Time(2018, 4, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Mon, 23 Apr 2018 11:30:23 GMT"));

    time =  Time(2018, 5, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Wed, 23 May 2018 11:30:23 GMT"));
     
    time =  Time(2018, 6, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Sat, 23 Jun 2018 11:30:23 GMT"));

    time =  Time(2018, 7, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Mon, 23 Jul 2018 11:30:23 GMT"));

    time =  Time(2018, 8, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Thu, 23 Aug 2018 11:30:23 GMT"));

    time =  Time(2018, 9, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 23 Sep 2018 11:30:23 GMT"));
    
    time =  Time(2018, 10, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Tue, 23 Oct 2018 11:30:23 GMT"));
    
    time =  Time(2018, 11, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    time.FromRFC1123(timeString);
    EXPECT_STREQ(timeString.c_str(), _T("Fri, 23 Nov 2018 11:30:23 GMT"));

    time =  Time(2018, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 23 Dec 2018 11:30:23 GMT"));

    time =  Time(1970, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);

    time =  Time(1980, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);
}
TEST(Core_Time, FromStandard_RFC1123_TimeStringCreatedFromTime_InvalidCase)
{
    Time time =  Time(18, 1, 23, 11, 30, 23, 21, false);
    std::string timeString;
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), false);

    time =  Time(80, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), false);
}
TEST(Core_Time, FromStandard_RFC1123_TimeStringCreatedWithLocalTimeEnabled)
{
    Time time;
    std::string timeString;
    time =  Time(2000, 12, 23, 11, 30, 23, 21, true);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);
}
TEST(Core_Time, FromStandard_RFC1123_LocalTimeDisabled)
{
    Time time;
    std::string timeString;
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 UTC"), true);
    time.ToString(timeString, false);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 06 Nov 1994 08:59:27 GMT"));
}
TEST(Core_Time, FromStandard_RFC1123_LocalTimeEnabled)
{
    Time time;
    std::string timeString;
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 UTC"), true);
    time.ToString(timeString, true);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 06 Nov 1994 00:59:27 "));
}
TEST(Core_Time, FromStandard_RFC1123_TimeConversion_WithLocalTimeDisabled)
{
    Time time(1980, 12, 23, 11, 30, 23, 21, true);
    std::string timeString1, timeString2;
    time.ToString(timeString1, false);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, false);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
}
TEST(Core_Time, FromStandard_RFC1123_TimeConversion_WithCurrentTime_And_LocalTimeDisabled)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    time.ToString(timeString1, false);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, false);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
}
TEST(Core_Time, FromStandard_RFC1123_TimeConversion_WithLocalTimeEnabled)
{
    Time time(1980, 12, 23, 11, 30, 23, 21, true);
    std::string timeString1, timeString2;
    time.ToString(timeString1, true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, true);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());

}
TEST(Core_Time, FromStandard_RFC1123_TimeConversion_WithLocalTimeEnabled_And_LocalTimeDisabled)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    time.ToString(timeString1, true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, true);
    //FIXME: EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    time.ToString(timeString1, true);
    //FIXME: EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
}
TEST(Core_Time, ToStandard_ISO8601)
{
    Time time(Time::Now());
    string timeString;
    string timeISOString;

    // localTime argument value as default
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T16:49:37Z");

    // localTime argument value as true
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T00:49:37");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T00:49:37");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T08:49:37");

    // localTime argument value as true
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T16:49:37Z");

    // Time with empty value return null string
    EXPECT_STREQ((Time().ToISO8601(true)).c_str(), "");
    EXPECT_STREQ((Time().ToISO8601(false)).c_str(), "");
}
TEST(Core_Time, ToStandard_RFC1123_WithLocalTimeDisabled)
{
    Time time(1980, 12, 23, 11, 30, 23, 21, true);
    std::string timeString1, timeString2;
    timeString1 = time.ToRFC1123(false);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    timeString2 = time.ToRFC1123(false);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
}
TEST(Core_Time, ToStandard_RFC1123_WithCurrentTime_And_LocalTimeDisabled)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    timeString1 = time.ToRFC1123(false);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    timeString2 = time.ToRFC1123(false);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
}
TEST(Core_Time, ToStandard_RFC1123_WithLocalTimeEnabled)
{
    Time time(2000, 12, 23, 11, 30, 23, 21, true);
    std::string timeString1, timeString2;
    timeString1 = time.ToRFC1123(true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    timeString2 = time.ToRFC1123(true);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());

}
TEST(Core_Time, ToStandard_RFC1123_WithLocalTimeEnabled_And_LocalTimeDisabled)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    timeString1 = time.ToRFC1123(true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    timeString2 = time.ToRFC1123(true);
    //FIXME: EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    timeString1 = time.ToRFC1123(true);
    //FIXME: EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
}
TEST(Core_Time, Ticks_withoutInterval)
{
    Core::Time time1 = Core::Time::Now();
    Core::Time time2 = Core::Time::Now();
    uint64_t diff = time2.Ticks() - time1.Ticks();
    uint32_t intervalInSeconds = diff / MicroSecondsPerSecond;
    EXPECT_EQ(intervalInSeconds, 0);
}
TEST(Core_Time, Ticks_withInterval)
{
    uint32_t givenIntervalInSeconds = 0;//2;
    Core::Time time1 = Core::Time::Now();
    sleep(givenIntervalInSeconds);
    Core::Time time2 = Core::Time::Now();
    uint64_t diff = time2.Ticks() - time1.Ticks();
    uint32_t intervalInSeconds = diff / MicroSecondsPerSecond;
    EXPECT_EQ(intervalInSeconds, givenIntervalInSeconds);
}
TEST(Core_Time, AddTime)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    time.ToString(timeString1);
    uint32_t currentTime = time.MilliSeconds();
    uint32_t timeTobeAdded = 2;
    time.Add(timeTobeAdded);
    uint32_t newTime = time.MilliSeconds();
    time.ToString(timeString2);
    EXPECT_EQ(newTime, currentTime + timeTobeAdded);
    currentTime = time.Seconds();
    timeTobeAdded = 3;
    time.Add(timeTobeAdded * 1000);
    time.ToString(timeString1);
    EXPECT_STRNE(timeString1.c_str(), timeString2.c_str());
    uint32_t expectedAddedTime = 0;
    if (currentTime + timeTobeAdded > 60) {
        expectedAddedTime = (currentTime + timeTobeAdded) - 60;
    } else {
        expectedAddedTime = (currentTime + timeTobeAdded);
    }

    newTime = time.Seconds();
    EXPECT_EQ(newTime, expectedAddedTime);
}
TEST(Core_Time, SubTime)
{
    Time time(Time::Now());
    std::string timeString1, timeString2;
    time.ToString(timeString1);
    uint32_t currentTime = time.MilliSeconds();
    uint32_t timeTobeSubtracted = 4;
    time.Sub(timeTobeSubtracted);
    uint32_t newTime = time.MilliSeconds();
    time.ToString(timeString2);
    EXPECT_EQ(newTime, currentTime - timeTobeSubtracted);
    currentTime = time.Seconds();
    timeTobeSubtracted = 6;
    time.Sub(timeTobeSubtracted * 1000);
    time.ToString(timeString1);
    EXPECT_STRNE(timeString1.c_str(), timeString2.c_str());
    uint32_t expectedSubtractedTime = 0;
    if (currentTime < timeTobeSubtracted) {
        expectedSubtractedTime = 60 - (currentTime - timeTobeSubtracted);
    } else {
        expectedSubtractedTime = (currentTime - timeTobeSubtracted);
    }
    newTime = time.Seconds();
    EXPECT_EQ(newTime, expectedSubtractedTime);;
}
TEST(Core_Time, NTPTime)
{
    const uint64_t ntpTime = 9487658348292704567ULL;
    Time time(1970, 1, 1, 0, 0, 0, 1, true);
    EXPECT_EQ(time.NTPTime()/MicroSecondsPerSecond, ntpTime/MicroSecondsPerSecond);
    uint32_t timeTobeAdded = 4;
    time.Add(timeTobeAdded);

    EXPECT_GE(time.NTPTime()/MicroSecondsPerSecond, ntpTime/MicroSecondsPerSecond + (timeTobeAdded * 4));
}
TEST(Core_Time, DifferenceFromGMTSeconds)
{
    string currentZone = ((getenv("TZ") != nullptr) ? getenv("TZ") : "");
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    Time time(80, 12, 23, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), 0);

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(2006, 11, 44, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), 0);
    setenv("TZ", currentZone.c_str(), 1);
}
TEST(Core_Time, DifferenceFromGMTSeconds_LocalTimeEnabled)
{
    string currentZone = ((getenv("TZ") != nullptr) ? getenv("TZ") : "");
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    Time time(80, 12, 23, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), -28800);

    setenv("TZ", "America/New_York", 1);
    tzset();
    time = Time(80, 12, 23, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), -18000);

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(2006, 11, 44, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), 19800);

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(2006, 11, 44, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DifferenceFromGMTSeconds(), 3600);
    setenv("TZ", currentZone.c_str(), 1);
}
TEST(Core_Time, Format)
{
    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.Format("%Y-%m-%d").c_str(), "2002-05-10");
    EXPECT_STREQ(time.Format("%d-%m-%Y").c_str(), "10-05-2002");
    time = Time(1970, 5, 32, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.Format("%Y-%m-%d").c_str(), "1970-06-01");
    EXPECT_STREQ(time.Format("%Y %m %d").c_str(), "1970 06 01");
    EXPECT_STREQ(time.Format("%Y:%m:%d").c_str(), "1970:06:01");
    // Format conversion will not support year below 1970
    time = Time(1969, 5, 32, 11, 30, 23, 21, false);
    EXPECT_STRNE(time.Format("%Y-%m-%d").c_str(), "1969-06-01");
}
TEST(Core_Time, ToTimeOnly)
{
    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "11:30:23GMT");
    EXPECT_EQ(time.MilliSeconds(), 21);
    time = Time(1970, 5, 32, 24, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "00:30:23GMT");
    time = Time(1970, 5, 32, 24, 60, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "01:00:23GMT");
}
TEST(Core_Time, ToTimeOnly_LocalTimeEnabled)
{
    string currentZone = ((getenv("TZ") != nullptr) ? getenv("TZ") : "");

    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "04:30:23");
    EXPECT_EQ(time.MilliSeconds(), 21);

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "00:30:23");

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 60, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "06:30:23");

    setenv("TZ", currentZone.c_str(), 1);
}
TEST(Core_Time, ToTimeOnly_And_TimeConstructor_WithLocalTimeEnabled)
{
    string currentZone = ((getenv("TZ") != nullptr) ? getenv("TZ") : "");

    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    Time time(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "12:30:23");
    EXPECT_EQ(time.MilliSeconds(), 21);

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 30, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "00:30:23");

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 60, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "01:00:23");

    setenv("TZ", currentZone.c_str(), 1);
}
TEST(Core_Time, ToTimeOnlyWithLocalTimeDisabled_And_TimeConstructor_WithLocalTimeEnabled)
{
    string currentZone = ((getenv("TZ") != nullptr) ? getenv("TZ") : "");

    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    Time time(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "19:30:23GMT");
    EXPECT_EQ(time.MilliSeconds(), 21);

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 30, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "00:30:23GMT");

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(1970, 5, 32, 24, 60, 23, 21, true);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "19:30:23GMT");

    setenv("TZ", currentZone.c_str(), 1);
}
TEST(Core_Time, ToLocal)
{
    Time time(2002, 5, 10, 11, 30, 23, 21, true);
    time.ToLocal();
    //FIXME: it has to be true, but always false, since the value for tm_zone is either UTC or GMT
    //EXPECT_EQ(time.IsLocalTime(), true);
    std::string timeString;
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Fri, 10 May 2002 11:30:23 ");

    time = Time(1970, 5, 32, 24, 30, 23, 21, true);
    time.ToLocal();
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Tue, 02 Jun 1970 00:30:23 ");
    //FIXME: it has to be true, but always false, since the value for tm_zone is either UTC or GMT
    //EXPECT_EQ(time.IsLocalTime(), true);
}
TEST(Core_Time, ToUTC)
{
    Time time(2002, 5, 10, 11, 30, 23, 21, true);
    time.ToUTC();
    std::string timeString;
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Fri, 10 May 2002 11:30:23 ");
    EXPECT_EQ(time.IsLocalTime(), false);

    time = Time(1970, 5, 32, 24, 30, 23, 21, true);
    time.ToUTC();
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Tue, 02 Jun 1970 00:30:23 ");
    EXPECT_EQ(time.IsLocalTime(), false);
}
