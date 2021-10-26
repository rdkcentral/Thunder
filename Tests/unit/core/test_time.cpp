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

TEST(Core_Time, timedetails)
{
    Time time(Time::Now());
    std::string timeString;
    time.ToString(timeString);
    time.ToRFC1123();
    time.Format("%d-%m-%Y");
    time.MilliSeconds();
    time.Seconds();
    time.Minutes();
    time.Hours();
    time.Day();
    time.MonthName();
    time.WeekDayName();
    time.NTPTime();
    time.DifferenceFromGMTSeconds();
    time.FromRFC1123(timeString);

    Time::Now().ToRFC1123();
    time.Add (2000);
    SystemInfo::Instance().SetTime(time);
    Time::Now().ToRFC1123();

    time.Sub (2000);
    SystemInfo::Instance().SetTime(time);
    Time::Now().ToRFC1123();

    struct tm setTime;
    ::memcpy(&setTime, &(time.Handle()), sizeof (setTime));
}
TEST(Core_Time, Month)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1921, 1, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(1000, 1, 260, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(2021, 1, 366, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1981, 1, 367, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Month(), 1);
}
TEST(Core_Time, Month_LocalTimeEnabled)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1921, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(1000, 1, 260, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(2021, 1, 366, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1981, 1, 367, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Month(), 1);
}
TEST(Core_Time, MonthName)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1921, 2, 255, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Oct");
    time =  Time(1000, 1, 260, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Sep");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Apr");
    time =  Time(2021, 1, 366, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1981, 2, 367, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.MonthName(), "Feb");
}
TEST(Core_Time, MonthName_LocalTimeEnabled)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1921, 2, 255, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Oct");
    time =  Time(1000, 1, 260, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Sep");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Apr");
    time =  Time(2021, 1, 366, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1981, 2, 367, 11, 30, 23, 21, true);
    EXPECT_STREQ(time.MonthName(), "Feb");
}
TEST(Core_Time, Year)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(1921, 1, 255, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 1921);
    time =  Time(1000, 1, 260, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 1000);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2021);
    time =  Time(2021, 1, 366, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 2022);
    time =  Time(1981, 1, 367, 11, 30, 23, 21, false);
    EXPECT_EQ(time.Year(), 1982);
}
TEST(Core_Time, Year_LocalTimeEnabled)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2000);
    time =  Time(1921, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 1921);
    time =  Time(1000, 1, 260, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 1000);
    time =  Time(2021, 1, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2021);
    time =  Time(2021, 1, 366, 11, 30, 23, 21, true);
    EXPECT_EQ(time.Year(), 2022);
    time =  Time(1981, 1, 367, 11, 30, 23, 21, true);
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
    time =  Time(2000, 1, 260, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 259);
    time =  Time(2000, 1, 366, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 365);
    time =  Time(2000, 1, 367, 11, 30, 23, 21, false);
    EXPECT_EQ(time.DayOfYear(), 0);
}
TEST(Core_Time, DayOfYear_LocalTimeEnabled)
{
    Time time(2002, 1, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 1, 0, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 364);
    time =  Time(2000, 1, 255, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 254);
    time =  Time(2000, 1, 260, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 259);
    time =  Time(2000, 1, 366, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 365);
    time =  Time(2000, 1, 367, 11, 30, 23, 21, true);
    EXPECT_EQ(time.DayOfYear(), 0);
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
//    tm time1 {1, 1, 1, 3, 1, 2004 - 1900};
//    time1.tm_isdst = -1;
//    time_t epoch = mktime(&time1);
//    printf("%s\n", asctime(gmtime(&epoch)));
//    printf("%s\n", ctime(&epoch));
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
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    // LocalTime true
    EXPECT_EQ(time.FromString(timeString2, true), true);
    time.ToString(timeString1);
    EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
    // LocalTime false
    EXPECT_EQ(time.FromString(timeString1, false), true);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    printf("%s:%s:%d timeString = %s\n", __FILE__, __func__, __LINE__, timeString1.c_str());
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

    time =  Time(70, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);

    time =  Time(80, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_EQ(time.FromRFC1123(timeString), true);
}
TEST(Core_Time, FromStandard_RFC1123_TimeStringCreatedWithLocalTimeEnabled)
{
    Time time;
    std::string timeString;
    time =  Time(80, 12, 23, 11, 30, 23, 21, true);
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
    Time time(80, 12, 23, 11, 30, 23, 21, true);
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
    Time time(80, 12, 23, 11, 30, 23, 21, true);
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
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    time.ToString(timeString1, true);
    EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
    //printf("%s:%s:%d timeString = %s\n", __FILE__, __func__, __LINE__, timeString1.c_str());
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
    Time time(80, 12, 23, 11, 30, 23, 21, true);
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
    Time time(80, 12, 23, 11, 30, 23, 21, true);
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
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    timeString1 = time.ToRFC1123(true);
    EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
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
    uint32_t givenIntervalInSeconds = 2;
    Core::Time time1 = Core::Time::Now();
    sleep(givenIntervalInSeconds);
    Core::Time time2 = Core::Time::Now();
    uint64_t diff = time2.Ticks() - time1.Ticks();
    uint32_t intervalInSeconds = diff / MicroSecondsPerSecond;
    EXPECT_EQ(intervalInSeconds, givenIntervalInSeconds);
}
