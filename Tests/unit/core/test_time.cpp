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
#include <ctime>

using namespace WPEFramework;
using namespace WPEFramework::Core;

int32_t GetMachineTimeDifference(const string& zone)
{
    time_t defineTime = 1325376000;
    setenv("TZ", zone.c_str(), 1);
    tzset();
    struct tm *time = localtime(&defineTime);
    return time->tm_gmtoff;
}

std::string ExecuteCmd(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");

    EXPECT_TRUE(pipe != nullptr);
#ifdef __CORE_EXCEPTION_CATCHING__
    try {
#endif
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result = buffer;
            break;
        }
#ifdef __CORE_EXCEPTION_CATCHING__
    } catch (...) {
        pclose(pipe);
        throw;
    }
#endif
    pclose(pipe);

    return result;
}
std::string GetSystemTime(bool local)
{

    std::string command = std::string("date +\"%a, %d %b %Y %H:%M:%S\"");
    command.append((local != true) ? " -u" : "");
    std::string systemTime = ExecuteCmd(command.c_str());
    systemTime.erase(remove(systemTime.begin(), systemTime.end(), '\n'), systemTime.end()); 
    systemTime.append((local != true) ? " GMT" : "");
    return systemTime;
}

TEST(Core_Time, DISABLED_Ctor_TimeSpec)
{
#ifdef __POSIX__
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    Time time(ts);

    string local;
    time.ToString(local, true);
    string utc;
    time.ToString(utc, false);

    EXPECT_STREQ(local.c_str(), GetSystemTime(true).c_str());
    EXPECT_STREQ(utc.c_str(), GetSystemTime(false).c_str());
#endif
}
TEST(Core_Time, Ctor_Copy)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time1;
    Time orig(1969,11, 1, 9, 10, 11, 500, true);
    EXPECT_EQ(orig.IsValid(), true);
    time1 = orig;
    EXPECT_EQ(time1.IsValid(), true);
    Time time2(orig);
    EXPECT_EQ(time2.IsValid(), true);

    string timeString;
    EXPECT_EQ(orig.MilliSeconds(), 500u);
    orig.ToString(timeString, true);
    std::cout << "Localtime 1969-11-01 09:10:11.500 : " << timeString << " ms: " << orig.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 09:10:11");
    orig.ToString(timeString, false);
    std::cout << "UTC 1969-11-01 09:10:11.500 : " << timeString << " ms: " << orig.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 08:10:11 GMT");

    EXPECT_EQ(time1.MilliSeconds(), 500u);
    time1.ToString(timeString, true);
    std::cout << "Localtime 1969-11-01 09:10:11.500 : " << timeString << " ms: " << time1.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 09:10:11");
    time1.ToString(timeString, false);
    std::cout << "UTC 1969-11-01 09:10:11.500 : " << timeString << " ms: " << time1.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 08:10:11 GMT");

    EXPECT_EQ(time2.MilliSeconds(), 500u);
    time2.ToString(timeString, true);
    std::cout << "Localtime 1969-11-01 09:10:11.500 : " << timeString << " ms: " << time2.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 09:10:11");
    time2.ToString(timeString, false);
    std::cout << "UTC 1969-11-01 09:10:11.500 : " << timeString << " ms: " << time2.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sat, 01 Nov 1969 08:10:11 GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, MilliSeconds)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));
    time = Time(2004, 2, 29, 11, 30, 23, 100, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(100));
    time = Time(2000, 11, 2, 11, 30, 23, 823, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(823));
    time = Time(2000, 12, 2, 11, 30, 23, 999, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(999));
}
TEST(Core_Time, MilliSeconds_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 2, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));
    time = Time(2000, 2, 28, 11, 30, 23, 100, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(100));
    time = Time(2000, 3, 2, 11, 30, 23, 923, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(923));
    time = Time(2000, 5, 2, 11, 30, 23, 999, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(999));

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Seconds)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(1970, 2, 24, 12, 45, 58, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 58);
    time =  Time(2000, 1, 24, 14, 1, 59, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 59);
    time =  Time(2021, 2, 28, 23, 15, 0, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 0);
    time =  Time(2021, 3, 31, 0, 6, 39, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 39);
}
TEST(Core_Time, Seconds_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 2, 11, 46, 6, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 6);
    time =  Time(1970, 2, 13, 12, 1, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(2000, 1, 2, 15, 59, 0, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 0);
    time =  Time(2021, 4, 30, 1, 30, 59, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 59);
    time =  Time(2021, 6, 3, 22, 59, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 23);
    time =  Time(2021, 11, 30, 23, 33, 18, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 18);
    time =  Time(2021, 7, 3, 22, 59, 0, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Seconds(), 0);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Minutes)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(1971, 12, 31, 12, 45, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 45);
    time =  Time(2021, 4, 30, 1, 30, 56, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(2021, 2, 6, 23, 1, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 1);
    time =  Time(2021, 2, 26, 22, 45, 45, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 45);
    time =  Time(1981, 1, 30, 23, 0, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(1981, 1, 30, 23, 0, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 0);
}
TEST(Core_Time, Minutes_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 2, 11, 30, 30, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(1971, 11, 1, 12, 0, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2000, 1, 20, 15, 10, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 10);
    time =  Time(2021, 4, 30, 2, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 30);
    time =  Time(2021, 2, 28, 22, 0, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2021, 2, 28, 22, 0, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 0);
    time =  Time(2021, 2, 3, 23, 34, 18, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 34);
    time =  Time(1981, 1, 31, 23, 59, 59, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Minutes(), 59);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Hours)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 11);
    time =  Time(1972, 2, 29, 12, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 12);
    time =  Time(2000, 1, 26, 14, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 14);
    time =  Time(2021, 4, 30, 0, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 0);
    time =  Time(2021, 5, 31, 23, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 23);
    time =  Time(2021, 6, 30, 0, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 0);
    time =  Time(1981, 1, 17, 0, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 0);
}
TEST(Core_Time, Hours_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 2, 11, 56, 46, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 10);
    time =  Time(1971, 9, 12, 12, 10, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 11);
    time =  Time(2000, 2, 29, 15, 10, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 14);
    time =  Time(2021, 5, 28, 23, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 21);
    time =  Time(2021, 5, 28, 0, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 22);
    time =  Time(2021, 1, 10, 22, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 21);
    time =  Time(2021, 7, 31, 1, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Hours(), 23);
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Day)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 2);
    time =  Time(1971, 2, 25, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 25);
    time =  Time(2000, 4, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 30);
    time =  Time(2021, 2, 28, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 28);
    time =  Time(2021, 5, 31, 20, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 31);
    time =  Time(1981, 1, 31, 23, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 31);
}
TEST(Core_Time, Day_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 4, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 4);
    time =  Time(1976, 2, 29, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 29);
    time =  Time(2000, 1, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 31);
    time =  Time(2021, 4, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 30);
    time =  Time(2021, 2, 28, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 28);
    time =  Time(1981, 6, 24, 23, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Day(), 24);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, WeekDayName)
{
    Time time(2000, 1, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Sun");
    time =  Time(1971, 2, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
    time =  Time(2000, 3, 25, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Sat");
    time =  Time(2021, 4, 28, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Wed");
    time =  Time(2024, 2, 29, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Thu");
    time =  Time(1981, 7, 13, 1, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
}
TEST(Core_Time, WeekDayName_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time(2000, 1, 3, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Mon");
    time =  Time(1971, 2, 2, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Tue");
    time =  Time(2000, 3, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Fri");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Fri");
    time =  Time(2021, 2, 28, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Sun");
    time =  Time(1981, 1, 31, 23, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.WeekDayName(), "Sat");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Month)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(1971, 2, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 2);
    time =  Time(2000, 3, 31, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 3);
    time =  Time(2021, 4, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 4);
    time =  Time(2021, 7, 28, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 7);
    time =  Time(1981, 11, 30, 23, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 11);
}
TEST(Core_Time, Month_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 9, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 9);
    time =  Time(1971, 1, 2, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 1);
    time =  Time(2000, 8, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 8);
    time =  Time(2021, 11, 28, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 11);
    time =  Time(2021, 6, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 6);
    time =  Time(1981, 12, 31, 23, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Month(), 12);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, MonthName)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1971, 3, 2, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Mar");
    time =  Time(2000, 5, 31, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "May");
    time =  Time(2021, 4, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Apr");
    time =  Time(2021, 1, 12, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1983, 6, 30, 22, 59, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Jun");
    time =  Time(1981, 7, 27, 23, 59, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Jul");
}
TEST(Core_Time, MonthName_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 1, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Jan");
    time =  Time(1971, 2, 28, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Feb");
    time =  Time(2000, 8, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Aug");
    time =  Time(2021, 9, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Sep");
    time =  Time(2021, 10, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Oct");
    time =  Time(1981, 11, 26, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Nov");
    time =  Time(1990, 12, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_STREQ(time.MonthName(), "Dec");

   (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Year)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2000));
    time =  Time(1971, 2, 28, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(1971));
    time =  Time(2000, 4, 10, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2000));
    time =  Time(2021, 6, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2021));
    time =  Time(2022, 12, 31, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2022));
    time =  Time(1981, 9, 14, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(1981));
}
TEST(Core_Time, Year_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2000, 3, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2000));
    time =  Time(1971, 5, 25, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(1971));
    time =  Time(2002, 7, 20, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2002));
    time =  Time(2021, 8, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2021));
    time =  Time(2025, 12, 31, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(2025));
    time =  Time(1981, 10, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.Year(), static_cast<uint32_t>(1981));

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, DayOfYear)
{
    Time time(2000, 1, 1, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 0);
    time =  Time(2000, 2, 28, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 58);
    time =  Time(2000, 3, 25, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 84);
    time =  Time(2000, 4, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 120);
    time =  Time(2000, 5, 31, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 151);
    time =  Time(2000, 6, 30, 11, 30, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 181);
}
TEST(Core_Time, DayOfYear_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time time(2002, 7, 1, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 181);
    time =  Time(2000, 8, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 222);
    time =  Time(2000, 9, 25, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 268);
    time =  Time(2000, 10, 29, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 302);
    time =  Time(2000, 11, 30, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 334);
    time =  Time(2000, 12, 6, 11, 30, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfYear(), 340);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, DayOfWeek)
{
    // Thu Jan  1 01:01:01 2004
    Time time(2004, 1, 1, 1, 1, 1, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jan  1 11:30:24 2004
    time = Time(2004, 1, 1, 11, 30, 24, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jun 20 20:20:22 2019
    time =  Time(2019, 6, 20, 20, 20, 22, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Mon Jul  1 13:40:33 2019
    time =  Time(2019, 5, 31, 13, 40, 33, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 5);
    // Mon Aug 31 11:50:31 2015
    time =  Time(2015, 8, 31, 11, 50, 31, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Tue Sep  1 11:50:23 2015
    time =  Time(2015, 8, 31, 11, 50, 23, 21, false);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 1);
}
TEST(Core_Time, DayOfWeek_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    // Thu Jan  1 01:01:01 2004
    Time time(2004, 1, 1, 1, 1, 1, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Thu Jan  1 11:30:24 2004
    time = Time(2004, 5, 1, 11, 30, 24, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 6);
    // Thu Jun 20 20:20:22 2019
    time =  Time(2019, 6, 20, 20, 20, 22, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 4);
    // Mon Jul  1 13:40:33 2019
    time =  Time(2019, 7, 31, 13, 40, 33, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 3);
    // Mon Aug 31 11:50:31 2015
    time =  Time(2015, 8, 31, 11, 50, 31, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 1);
    // Tue Sep  1 11:50:23 2015
    time =  Time(2015, 9, 30, 11, 50, 23, 21, true);
    EXPECT_EQ(time.IsValid(), true);
    EXPECT_EQ(time.DayOfWeek(), 3);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Ctor_TimeValues_InvalidValues)
{
    Time time(2000, 1, 2, 11, 30, 23, 1000, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 30, 67, 21, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 100, 23, 21, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 25, 30, 23, 10, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 32, 11, 30, 6, 21, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 0, 11, 30, 23, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2001, 2, 29, 11, 30, 23, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2004, 2, 30, 11, 30, 23, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2003, 6, 31, 11, 30, 23, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 15, 2, 11, 1, 23, 21, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 0, 2, 11, 1, 23, 21, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1821, 1, 2, 11, 60, 23, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1871, 1, 2, 11, 0, 60, 100, false);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1871, 1, 2, 24, 0, 35, 100, false);
    EXPECT_EQ(time.IsValid(), false);

}
TEST(Core_Time, Ctor_TimeValues_LocalTimeEnabled_InvalidValues)
{
    Time time(2000, 1, 2, 11, 30, 23, 1000, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 30, 60, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 30, 67, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 60, 23, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 11, 100, 23, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 2, 24, 30, 23, 10, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 1, 32, 11, 30, 6, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1921, 1, 0, 11, 30, 23, 100, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1923, 2, 29, 11, 30, 23, 100, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 15, 2, 11, 1, 23, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 0, 2, 11, 1, 23, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(2000, 9, 31, 11, 1, 23, 21, true);
    EXPECT_EQ(time.IsValid(), false);
    time = Time(1821, 1, 2, 24, 30, 23, 100, true);
    EXPECT_EQ(time.IsValid(), false);
}
TEST(Core_Time, FromString_CurrentTime)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

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
    time.ToString(timeString1);
    EXPECT_EQ(time.FromString(timeString2, false), true);
    time.ToString(timeString2);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromString_TimeCtor)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time orig(1969,10, 3, 10, 11, 12, 666, true);
    string timeString;
    orig.ToString(timeString, true);

    Time time1;
    time1.FromString(timeString, true);
    orig.ToString(timeString, false);

    Time time2;
    time2.FromString(timeString, false);

    EXPECT_EQ(orig.MilliSeconds(), 666u);
    orig.ToString(timeString, true);
    std::cout << "Localtime 1969-10-03 10:11:12 666 : " << timeString << " ms: " << orig.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12");
    orig.ToString(timeString, false);
    std::cout << "UTC 1969-10-03 10:11:12 666 : " << timeString << " ms: " << orig.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12 GMT");

    time1.ToString(timeString, true);
    std::cout << "Localtime 1969-10-03 10:11:12 666 : " << timeString << " ms: " << time1.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12");
    time1.ToString(timeString, false);
    std::cout << "UTC 1969-10-03 10:11:12 666 : " << timeString << " ms: " << time1.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12 GMT");

    time2.ToString(timeString, true);
    std::cout << "LocalTime 1969-10-03 10:11:12 666 : " << timeString << " ms: " << time2.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12");
    time2.ToString(timeString, false);
    std::cout << "UTC 1969-10-03 10:11:12 666 : " << timeString << " ms: " << time2.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Fri, 03 Oct 1969 10:11:12 GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromString_ANSI)
{
    Time time;
    std::string timeString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("Sun Nov 6 08:49:37 1994", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sun Nov 6 12:49:37 1994", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 12:49:37");

    // Check time difference after set time zone to GST
    setenv("TZ", "GST", 1);
    tzset();
    EXPECT_EQ(time.FromString("Sun Nov 6 12:49:37 1994", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 12:49:37");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromString_ISO8601)
{
    Time time;
    std::string timeString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    // LocalTime false
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");

    // Check time difference after set time zone to GST
    setenv("TZ", "GST", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    // LocalTime false
    EXPECT_EQ(time.FromString("1994-11-06T08:49:37Z", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromString_RFC1036)
{
    Time time;
    std::string timeString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("Sunday, 06-Nov-94 08:49:37 GMT", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");

    // LocalTime false
    EXPECT_EQ(time.FromString("Sunday, 06-Nov-94 08:49:37 GMT", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromString_RFC1123)
{
    Time time;
    std::string timeString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 08:49:37 GMT", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 08:49:37 GMT", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");

    // Check time difference after set time zone to GST
    setenv("TZ", "GST", 1);
    tzset();

    // LocalTime true
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 08:49:37 GMT", true), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 08:49:37");
    // LocalTime false
    EXPECT_EQ(time.FromString("Sun, 06 Nov 1994 12:49:37 GMT", false), true);
    time.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 06 Nov 1994 12:49:37");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromStandard_ANSI_ValidFormat)
{
    Time time(Time::Now());
    EXPECT_EQ(time.FromANSI("Thu Aug11 23:47:20 2010", true), true);
    EXPECT_EQ(time.FromANSI("Sun Sep 02 23:47:20 2012", false), true);
    EXPECT_EQ(time.FromANSI("Sat Jul 13 23:47:20 2012", true), true);
    EXPECT_EQ(time.FromANSI("Sun Jun3 3:47:20 2012", false), true);
    EXPECT_EQ(time.FromANSI("Sun Jan01 23:7:20 2012", false), true);
    EXPECT_EQ(time.FromANSI("Thu Nov22 23:47:0 2012", true), true);
    EXPECT_EQ(time.FromANSI("Fri May   11   23:47:20 2012", false), true);
    EXPECT_EQ(time.FromANSI("Fri Apr06 3:7:0 2012", true), true);

    EXPECT_EQ(time.FromANSI("Thu Mar08 23:47:20 2012 GMT", false), true);
    EXPECT_EQ(time.FromANSI("Thu Mar08 23:47:20 2012 GMT", true), true);
    EXPECT_EQ(time.FromANSI("Sun Feb05 23:47:20 2012 UTC", false), true);
    EXPECT_EQ(time.FromANSI("Sun Feb05 23:47:20 2012 UTC", true), true);
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
    EXPECT_EQ(time.FromANSI("SAT Jan06T0a:49:37.123+06:45", false), false);
    EXPECT_EQ(time.FromANSI("Sunday Feb06T08:4a:37.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("Tues Mar0aT08:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("TMar0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("19Mar0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("No1v0608:49:5.123+06:45 UTC", true), false);
    EXPECT_EQ(time.FromANSI("No0608:49:5.123+06:45 UTC", true), false);
    EXPECT_EQ(time.FromANSI("  Sun Jan 8 TEST08:49:37.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("Sun 0608:49:37.803+06:45", true), false);
    EXPECT_EQ(time.FromANSI("Mon MarTEST0608:49:5a.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("Sun Mar608:49:5TEST.123+06:45", true), false);
    EXPECT_EQ(time.FromANSI("Sun Mar608:49:5.123+06:45 TEST GMT", true), false);
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
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123Z"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+01"), true);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06 45"), true);
    EXPECT_EQ(time.FromISO8601("1994-12-06T08:49:37.123 06:45"), true);
}
TEST(Core_Time, FromStandard_ISO8601_InvalidFormat)
{
    Time time(Time::Now());
    std::string timeString;
    // ToString is in RFC1123 format, hence it should fail
    EXPECT_EQ(time.FromISO8601(timeString), false);
    EXPECT_EQ(time.FromISO8601("1994-13-06T08:49:37.123+06:45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123-06:: 45"), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37."), false);
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37.123+06- 45"), false);
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
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 08:49:37"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:07"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:07"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:7 "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:09:7"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 GMT"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 GMT "), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7 UTC"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Apr-94 8:9:7 GMT"), true);
    EXPECT_EQ(time.FromRFC1036("Monday, 7-Nov-94 8:9:7"), true);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 08:49:37 PDT"), true);
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
    EXPECT_EQ(time.FromRFC1036("XXXX,06-Nov-08:11:49:37 "), false);
    EXPECT_EQ(time.FromRFC1036("XXXX-06-Nov-08:11:49:37 "), false);
    EXPECT_EQ(time.FromRFC1036("XXXX:06-Nov-08:11:49:37 "), false);
    EXPECT_EQ(time.FromRFC1036("XXXX, 06-Nov-08:11:49:37 "), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-TEST1994 08:49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, TEST06-Nov-1994 08:49:37"), false);
    EXPECT_EQ(time.FromRFC1036("Sunday, 06-Nov-1994 TEST08:49:37"), false);
}
TEST(Core_Time, FromStandard_RFC1123_ValidFormat)
{
    Time time;
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:49:37 GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov199408:49:37GMT"), true);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov199408:49:37"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27"), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 "), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 FF "), true);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 GMT"), true);
}
TEST(Core_Time, FromStandard_RFC1123_InvalidFormat)
{
    Time time;
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 ff 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994ff 08:49:37 GMT"), false);
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
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994:08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov1994TEST8:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("Sun,06Nov1994TEST8:49:37"), false);
    EXPECT_EQ(time.FromRFC1123("Sund, 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("TEST, 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, TEST 06 Nov 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov TEST 1994 08:49:37 GMT"), false);
    EXPECT_EQ(time.FromRFC1123("Sunday, 06 Nov 1994 08:59:27 GMT"), false);
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
    Time time =  Time(2018, 1, 23, 11, 30, 23, 21, false);
    std::string timeString;
    time.ToString(timeString, false);
    EXPECT_STREQ(timeString.c_str(), "Tue, 23 Jan 2018 11:30:23 GMT");
    EXPECT_EQ(time.FromRFC1123(timeString), true);

    time =  Time(2080, 12, 23, 11, 30, 23, 21, false);
    time.ToString(timeString, false);
    EXPECT_STREQ(timeString.c_str(), "Mon, 23 Dec 2080 11:30:23 GMT");
    EXPECT_EQ(time.FromRFC1123(timeString), true);
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
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 UTC"), true);
    time.ToString(timeString, false);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 06 Nov 1994 08:59:27 GMT"));
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, FromStandard_RFC1123_LocalTimeEnabled)
{
    Time time;
    std::string timeString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 08:59:27 UTC"), true);
    time.ToString(timeString, true);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 06 Nov 1994 08:59:27"));

    // Check time difference after set time zone to GST
    setenv("TZ", "GST", 1);
    tzset();

    // Set without timezone
    EXPECT_EQ(time.FromRFC1123("Sun, 06 Nov 1994 09:59:27 UTC"), true);
    time.ToString(timeString, true);
    EXPECT_STREQ(timeString.c_str(), _T("Sun, 06 Nov 1994 09:59:27"));

    // Set timezone back
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
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
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    time.ToString(timeString1, false);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, false);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
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
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    time.ToString(timeString1, true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    time.ToString(timeString2, true);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    time.ToString(timeString1, true);
    EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, ToStandard_ISO8601)
{
    Time time(Time::Now());
    string timeString;
    string timeISOString;
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    // localTime argument value as default
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T00:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601()).c_str(), "1994-11-06T08:49:37Z");

    // localTime argument value as true
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T08:49:37");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T00:49:37");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(true)).c_str(), "1994-11-06T08:49:37");

    // localTime argument value as true
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37Z"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T08:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T00:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T00:49:37Z");
    EXPECT_EQ(time.FromISO8601("1994-11-06T08:49:37"), true);
    EXPECT_STREQ((time.ToISO8601(false)).c_str(), "1994-11-06T08:49:37Z");

    // Time with empty value return null string
    EXPECT_STREQ((Time().ToISO8601(true)).c_str(), "");
    EXPECT_STREQ((Time().ToISO8601(false)).c_str(), "");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
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
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time(Time::Now());
    std::string timeString1, timeString2;
    timeString1 = time.ToRFC1123(true);
    EXPECT_EQ(time.FromRFC1123(timeString1), true);
    timeString2 = time.ToRFC1123(true);
    EXPECT_STREQ(timeString1.c_str(), timeString2.c_str());
    EXPECT_EQ(time.FromRFC1123(timeString2), true);
    timeString1 = time.ToRFC1123(true);
    EXPECT_STREQ(timeString2.c_str(), timeString1.c_str());
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Ticks_withoutInterval)
{
    Core::Time time1 = Core::Time::Now();
    Core::Time time2 = Core::Time::Now();
    uint64_t diff = time2.Ticks() - time1.Ticks();
    uint32_t intervalInSeconds = diff / Time::MicroSecondsPerSecond;
    EXPECT_EQ(intervalInSeconds, static_cast<uint32_t>(0));
}
TEST(Core_Time, Ticks_TimeCtor)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    Time orig(2016,10, 3, 10, 11, 12, 666, true);
    Time::microsecondsfromepoch flattime = orig.Ticks();

    std::cout << " Ticks for local 2016-10-03 10:11:12 666 (so UTC ticks) : " << flattime << std::endl;
    Time copyutc(flattime);
    string timeString;
    copyutc.ToString(timeString, true);
    std::cout << "Localtime 2016-10-03 10:11:12 666 : " << timeString << " ms: " << copyutc.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 03 Oct 2016 10:11:12");

    copyutc.ToString(timeString, false);
    std::cout << "UTC 2016-10-03 08:11:12 666 : " << timeString << " ms: " << copyutc.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 03 Oct 2016 08:11:12 GMT");
    EXPECT_EQ(copyutc.MilliSeconds(), 666u);

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, Ticks_withInterval)
{
    uint32_t givenIntervalInSeconds = 2;
    Core::Time time1 = Core::Time::Now();
    sleep(givenIntervalInSeconds);
    Core::Time time2 = Core::Time::Now();
    uint64_t diff = time2.Ticks() - time1.Ticks();
    uint32_t intervalInSeconds = diff / Time::MicroSecondsPerSecond;
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
    if (currentTime + timeTobeAdded >= 60) {
        expectedAddedTime = (currentTime + timeTobeAdded) - 60;
    } else {
        expectedAddedTime = (currentTime + timeTobeAdded);
    }

    newTime = time.Seconds();
    EXPECT_EQ(newTime, expectedAddedTime);
}
TEST(Core_Time, DISABLED_SubTime)
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
    if (currentTime <= timeTobeSubtracted) {
        expectedSubtractedTime = 60 - (timeTobeSubtracted - currentTime);
        expectedSubtractedTime = (expectedSubtractedTime == 60) ? 0 : expectedSubtractedTime;
    } else {
        expectedSubtractedTime = (currentTime - timeTobeSubtracted);
    }
    newTime = time.Seconds();
    EXPECT_EQ(newTime, expectedSubtractedTime);;
}
TEST(Core_Time, NTPTime)
{
    Time time(1970, 1, 1, 0, 0, 0, 1, true);
    const uint64_t ntpTime = time.NTPTime();
    EXPECT_GE(ntpTime/Time::MicroSecondsPerSecond, static_cast<uint32_t>(0));
    uint32_t timeTobeAdded = 4;
    time.Add(timeTobeAdded);

    EXPECT_GE(time.NTPTime()/Time::MicroSecondsPerSecond, ntpTime/Time::MicroSecondsPerSecond + (timeTobeAdded * 4));
}
TEST(Core_Time, Format)
{
    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.Format("%Y-%m-%d").c_str(), "2002-05-10");
    EXPECT_STREQ(time.Format("%d-%m-%Y").c_str(), "10-05-2002");
    time = Time(1970, 5, 31, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.Format("%Y-%m-%d").c_str(), "1970-05-31");
    EXPECT_STREQ(time.Format("%Y %m %d").c_str(), "1970 05 31");
    EXPECT_STREQ(time.Format("%Y:%m:%d").c_str(), "1970:05:31");

    time = Time(1969, 5, 31, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.Format("%Y-%m-%d").c_str(), "1969-05-31");
}
TEST(Core_Time, ToTimeOnly)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "11:30:23GMT");
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));
    time = Time(1970, 5, 31, 0, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "00:30:23GMT");
    time = Time(1970, 5, 31, 23, 0, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "23:00:23GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, ToTimeOnly_LocalTimeEnabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "11:30:23");
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));

    // Check time difference after set time zone to GST
    setenv("TZ", "GST", 1);
    tzset();

    // Check time after unset time zone
    time = Time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "11:30:23");
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(1970, 5, 31, 0, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "00:30:23");

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(1970, 5, 31, 22, 0, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "03:30:23");

    setenv("TZ", "GST", 1);
    tzset();
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    time = Time(80, 12, 23, 11, 30, 23, 21, false);
    //EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "03:30:23");

    setenv("TZ", "GST", 1);
    tzset();
    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();
    time = Time(80, 12, 23, 11, 30, 23, 21, false);
    //EXPECT_STREQ(time.ToTimeOnly(true).c_str(), "12:30:23");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, ToTimeOnly_LocalTimeDisabled)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time(2002, 5, 10, 11, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "11:30:23GMT");
    EXPECT_EQ(time.MilliSeconds(), static_cast<uint32_t>(21));

    setenv("TZ", "Africa/Algiers", 1);
    tzset();
    time = Time(1970, 5, 31, 0, 30, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "00:30:23GMT");

    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    time = Time(1970, 5, 31, 0, 0, 23, 21, false);
    EXPECT_STREQ(time.ToTimeOnly(false).c_str(), "00:00:23GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, TimeWithLocalTimeTrue_ConvertTo_UTC_And_Local)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    string timeString;
    Time time = Time(2021, 07, 05, 22, 10, 5, 999, true);
    EXPECT_EQ(time.MilliSeconds(), 999u);
    time.ToString(timeString, true);
    std::cout << "LocalTime 2021-07-05 22:10:05.999 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 05 Jul 2021 22:10:05");
    time.ToString(timeString, false);
    std::cout << "UTC 2021-07-05 20:10:05.999 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 05 Jul 2021 20:10:05 GMT");

    time = Time(2021,12, 26, 23, 59, 59, 1, true);
    EXPECT_EQ(time.MilliSeconds(), 1u);
    time.ToString(timeString, true);
    std::cout << "LocalTime 2021-12-26 23:59:59.001 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2021 23:59:59");
    time.ToString(timeString, false);
    std::cout << "UTC 2021-12-26 22:59:59.001 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2021 22:59:59 GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, TimeWithLocalTimeFalse_ConvertTo_UTC_And_Local)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    setenv("TZ", "Europe/Amsterdam", 1);
    tzset();

    string timeString;
    Time time = Time(2021, 07, 05, 22, 10, 5, 999, false);
    EXPECT_EQ(time.MilliSeconds(), 999u);
    time.ToString(timeString, true);
    std::cout << "LocalTime 2021-07-06 00:10:05.999 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Tue, 06 Jul 2021 00:10:05");
    time.ToString(timeString, false);
    std::cout << "UTC 2021-07-05 22:10:05.999 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 05 Jul 2021 22:10:05 GMT");

    time = Time(2021,12, 26, 23, 59, 59, 1, false);
    EXPECT_EQ(time.MilliSeconds(), 1u);
    time.ToString(timeString, true);
    std::cout << "LocalTime 2021-12-27 00:59:59.001 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Mon, 27 Dec 2021 00:59:59");
    time.ToString(timeString, false);
    std::cout << "UTC 2021-12-26 23:59:59.001 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2021 23:59:59 GMT");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, TimeAsLocal)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "Asia/Kolkata", 1);
    tzset();


    Time orig(2004, 12, 26, 8, 30, 45, 737, true);

    string timeString;
    orig.ToString(timeString, true);
    std::cout << "Reference local 2004-12-26 08:30:45 737 : " << timeString << " ms: " << orig.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 08:30:45");
    EXPECT_EQ(orig.MilliSeconds(), 737u);

    TimeAsLocal l1(orig);
    TimeAsLocal l2;
    l2 = orig;

    l1.ToString(timeString);
    std::cout << "AsLocal 1 tostring : " << timeString << " ms: " << l1.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 08:30:45 GMT");
    EXPECT_EQ(l1.MilliSeconds(), 737u);
    EXPECT_STREQ(l1.WeekDayName(), "Sun");
    EXPECT_STREQ(l1.MonthName(), "Dec");
    EXPECT_EQ(l1.DayOfYear(), 360u);
    EXPECT_EQ(l1.DayOfWeek(), 0u);
    EXPECT_EQ(l1.Year(), 2004u);
    EXPECT_EQ(l1.Month(), 12u);
    EXPECT_EQ(l1.Day(), 26u);
    
    std::cout << "WeekDayName:" << l1.WeekDayName() << " MonthName:" << l1.MonthName() << " DayOfYear:" << l1.DayOfYear() << " DayOfWeek:" << static_cast<uint32_t>(l1.DayOfWeek()) << std::endl;
    std::cout << "Year:" << static_cast<uint32_t>(l1.Year()) << " Month:" << static_cast<uint32_t>(l1.Month()) << " Day:" << static_cast<uint32_t>(l1.Day()) << " Hours:" << static_cast<uint32_t>(l1.Hours()) << " Minutes:" << static_cast<uint32_t>(l1.Minutes()) << " Seconds:" << static_cast<uint32_t>(l1.Seconds()) << " MilliSeconds:" << l1.MilliSeconds() << std::endl;

    l2.ToString(timeString);
    std::cout << "AsLocal 2 tostring : " << timeString << " ms: " << l2.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 08:30:45 GMT");
    EXPECT_EQ(l2.MilliSeconds(), 737u);
    EXPECT_STREQ(l2.WeekDayName(), "Sun");
    EXPECT_STREQ(l2.MonthName(), "Dec");
    EXPECT_EQ(l2.DayOfYear(), 360u);
    EXPECT_EQ(l2.DayOfWeek(), 0u);
    EXPECT_EQ(l2.Year(), 2004u);
    EXPECT_EQ(l2.Month(), 12u);
    EXPECT_EQ(l2.Day(), 26u);

    std::cout << "WeekDayName:" << l2.WeekDayName() << " MonthName:" << l2.MonthName() << " DayOfYear:" << l2.DayOfYear() << " DayOfWeek:" << static_cast<uint32_t>(l2.DayOfWeek()) << std::endl;
    std::cout << "Year:" << static_cast<uint32_t>(l2.Year()) << " Month:" << static_cast<uint32_t>(l2.Month()) << " Day:" << static_cast<uint32_t>(l2.Day()) << " Hours:" << static_cast<uint32_t>(l2.Hours()) << " Minutes:" << static_cast<uint32_t>(l2.Minutes()) << " Seconds:" << static_cast<uint32_t>(l2.Seconds()) << " MilliSeconds:" << l2.MilliSeconds() << std::endl;

    l1.Add((3600 * 3 * 1000) + 500); // add 3 hours and 500 ms
    l1.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 11:30:46 GMT");
    EXPECT_EQ(l1.MilliSeconds(), 237u);

    std::cout << "AsLocal + 3 hr and 500 ms  : " << timeString << " ms: " << l1.MilliSeconds() << std::endl;
    l1.Sub((3600 * 3 * 1000) + 500); // add 3 hours and 500 ms
    l1.ToString(timeString);
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 08:30:45 GMT");
    EXPECT_EQ(l1.MilliSeconds(), 737u);
    std::cout << "AsLocal - 3 hr and 500 ms  : " << timeString << " ms: " << l1.MilliSeconds() << std::endl;

    Time t1 = l1.ToUTC();
    orig.ToString(timeString, true);
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 08:30:45");
    EXPECT_EQ(t1.MilliSeconds(), 737u);
    std::cout << "Back to UTC local 2004-12-26 08:30:45 737 local: " << timeString << " ms: " << t1.MilliSeconds() << std::endl;
    orig.ToString(timeString, false);
    EXPECT_STREQ(timeString.c_str(), "Sun, 26 Dec 2004 03:00:45 GMT");
    std::cout << "Back to UTC local 2004-12-26 08:30:45 737 as UTC : " << timeString << " ms: " << t1.MilliSeconds() << std::endl;

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, JulianDate)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time1(2002, 5, 10, 11, 30, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 24, 22, true);
    // Doing simple comparison based on time difference
    EXPECT_GE(time2.JulianDate(), time1.JulianDate());

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, OperatorOverloadingEqual)
{
    Time time1(2002, 5, 10, 11, 30, 23, 22, true);
    Time time2(2002, 5, 10, 11, 30, 23, 22, true);
    EXPECT_EQ(time1 == time2, true);
    time1 = Time(1971, 5, 10, 11, 4, 24, 21, true);
    time2 = Time(1971, 5, 10, 11, 4, 24, 21, true);
    EXPECT_EQ(time1 == time2, true);
}
TEST(Core_Time, OperatorOverloadingEqual_CheckNotEqualValues)
{
    Time time1(2002, 5, 10, 11, 35, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 23, 22, true);
    EXPECT_EQ(time1 == time2, false);
    time1 = Time(1971, 5, 10, 11, 4, 24, 21, true);
    time2 = Time(1971, 5, 11, 11, 4, 24, 21, true);
    EXPECT_EQ(time1 == time2, false);
}
TEST(Core_Time, OperatorOverloadingNotEqual)
{
    Time time1(2001, 5, 10, 11, 30, 23, 20, true);
    Time time2(2002, 5, 10, 11, 30, 23, 22, true);
    EXPECT_EQ(time1 != time2, true);
    time1 = Time(1971, 5, 10, 11, 4, 24, 21, true);
    time2 = Time(1970, 5, 10, 11, 4, 24, 26, true);
    EXPECT_EQ(time1 != time2, true);
}
TEST(Core_Time, OperatorOverloadingNotEqual_CheckEqualValues)
{
    Time time1(2015, 12, 10, 11, 30, 23, 55, true);
    Time time2(2015, 12, 10, 11, 30, 23, 55, true);
    EXPECT_EQ(time1 != time2, false);
    time1 = Time(1971, 8, 11, 11, 4, 24, 21, true);
    time2 = Time(1971, 8, 11, 11, 4, 24, 21, true);
    EXPECT_EQ(time1 != time2, false);
}
TEST(Core_Time, OperatorOverloadingLessThan)
{
    // Difference in milliseconds
    Time time1(2002, 5, 10, 11, 30, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 23, 22, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in Seconds
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 30, 24, 21, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in minutes
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 35, 23, 22, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in hours
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 13, 30, 23, 21, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in days
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 12, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in months
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 6, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 < time2, true);

    // Difference in years
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2004, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 < time2, true);
}
TEST(Core_Time, OperatorOverlessGreaterThan_CheckNotLessThanValues)
{
    // Check with equals
    Time time1(2020, 2, 1, 14, 30, 35, 41, true);
    Time time2(2020, 2, 1, 14, 30, 35, 41, true);
    EXPECT_EQ(time1 > time2, false);

    // Check with greater values
    time1 = Time(2008, 5, 10, 11, 30, 25, 21, true);
    time2 = Time(2006, 5, 10, 11, 30, 26, 21, true);
    EXPECT_EQ(time1 < time2, false);
    time1 = Time(2002, 5, 10, 13, 40, 23, 21, true);
    time2 = Time(2002, 5, 10, 12, 35, 23, 22, true);
    EXPECT_EQ(time1 < time2, false);
    time1 = Time(2002, 5, 10, 11, 30, 23, 26, true);
    time2 = Time(2002, 5, 10, 11, 30, 23, 24, true);
    EXPECT_EQ(time1 < time2, false);
}
TEST(Core_Time, OperatorOverloadingLessThanEqual)
{
    // Difference in milliseconds
    Time time1(2002, 5, 10, 11, 30, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 23, 22, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in Seconds
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 30, 24, 21, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in minutes
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 35, 23, 22, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in hours
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 13, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in days
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 12, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in months
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 6, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, true);

    // Difference in years
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2004, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, true);

    // Using equal time
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, true);
}
TEST(Core_Time, OperatorOverloadingLessThanEqual_CheckNotLessThanEqualValues)
{
    // Check with greater values
    Time time1(2004, 5, 10, 11, 30, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 24, 21, true);
    EXPECT_EQ(time1 <= time2, false);
    time1 = Time(2002, 5, 10, 14, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 11, 35, 23, 22, true);
    EXPECT_EQ(time1 <= time2, false);
    time1 = Time(2004, 5, 10, 11, 30, 23, 22, true);
    time2 = Time(2004, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 <= time2, false);
}
TEST(Core_Time, OperatorOverloadingGreaterThan)
{
    // Difference in milliseconds
    Time time1(2002, 5, 10, 11, 30, 23, 23, true);
    Time time2(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in Seconds
    time1 = Time(2002, 5, 10, 11, 30, 25, 21, true);
    time2 = Time(2002, 5, 10, 11, 30, 24, 21, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in minutes
    time1 = Time(2002, 5, 10, 11, 40, 23, 22, true);
    time2 = Time(2002, 5, 10, 11, 35, 23, 22, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in hours
    time1 = Time(2002, 5, 10, 14, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 13, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in days
    time1 = Time(2002, 5, 24, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 12, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in months
    time1 = Time(2002, 7, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 6, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, true);

    // Difference in years
    time1 = Time(2010, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2004, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, true);
}
TEST(Core_Time, OperatorOverloadingGreaterThan_CheckNotGreaterThanValues)
{
    // Check with equals
    Time time1(2002, 5, 10, 11, 30, 23, 21, true);
    Time time2(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 > time2, false);

    // Check with lesser values
    time1 = Time(2004, 5, 10, 11, 30, 25, 21, true);
    time2 = Time(2006, 5, 10, 11, 30, 26, 21, true);
    EXPECT_EQ(time1 > time2, false);
    time1 = Time(2002, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 12, 35, 23, 22, true);
    EXPECT_EQ(time1 > time2, false);
    time1 = Time(2002, 5, 10, 11, 30, 23, 22, true);
    time2 = Time(2002, 5, 10, 11, 30, 23, 24, true);
    EXPECT_EQ(time1 > time2, false);
}
TEST(Core_Time, OperatorOverloadingGreaterThanEqual)
{
    // Difference in milliseconds
    Time time1(2002, 5, 10, 11, 30, 23, 23, true);
    Time time2(2002, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in Seconds
    time1 = Time(2002, 5, 10, 11, 30, 25, 21, true);
    time2 = Time(2002, 5, 10, 11, 30, 24, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in minutes
    time1 = Time(2002, 5, 10, 11, 40, 23, 22, true);
    time2 = Time(2002, 5, 10, 11, 35, 23, 22, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in hours
    time1 = Time(2002, 5, 10, 14, 30, 23, 21, true);
    time2 = Time(2002, 5, 10, 13, 30, 23, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in days
    time1 = Time(2002, 5, 24, 11, 30, 23, 21, true);
    time2 = Time(2002, 5, 12, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in months
    time1 = Time(2002, 7, 10, 11, 30, 23, 21, true);
    time2 = Time(2002, 6, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Difference in years
    time1 = Time(2010, 5, 10, 11, 30, 23, 21, true);
    time2 = Time(2004, 5, 10, 11, 30, 23, 21, true);
    EXPECT_EQ(time1 >= time2, true);

    // Using equal time
    time1 = Time(2012, 10, 1, 3, 45, 34, 10, true);
    time2 = Time(2012, 10, 1, 3, 45, 34, 10, true);
    EXPECT_EQ(time1 >= time2, true);
}
TEST(Core_Time, OperatorOverloadingGreaterThan_CheckNotGreaterThanEqualValues)
{
    // Check with lesser values
    Time time1(2004, 5, 10, 11, 30, 25, 21, true);
    Time time2(2006, 5, 10, 11, 30, 26, 21, true);
    EXPECT_EQ(time1 >= time2, false);
    time1 = Time(2000, 5, 9, 11, 30, 2, 21, true);
    time2 = Time(2002, 5, 10, 12, 35, 23, 22, true);
    EXPECT_EQ(time1 >= time2, false);
    time1 = Time(1980, 5, 10, 11, 30, 23, 22, true);
    time2 = Time(1980, 5, 10, 11, 30, 23, 24, true);
    EXPECT_EQ(time1 >= time2, false);
}
TEST(Core_Time, OperatorOverloadingAssignment)
{
    Time time1(2002, 5, 10, 11, 30, 23, 22, true);
    Time time2 = time1;
    EXPECT_EQ(time1 == time2, true);
    time2 = Time(1971, 5, 10, 11, 4, 24, 21, true);
    time1 = time2;
    EXPECT_EQ(time1 == time2, true);
}
TEST(Core_Time, OperatorOverloadingAddition)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time1(2002, 5, 10, 11, 30, 23, 22, true);
    Time time2;
    Time time3 = time1 + time2;
    EXPECT_EQ(time1 == time3, true);

    Time time4(2000, 5, 10, 11, 30, 23, 22, true);
    Time time5 = time3 + time4;
    string timeString;
    time5.ToString(timeString);

    // Time will be saved/set based on 1st of January 1970, 00:00:00 time base.
    // So the saved time values will be always the one subtracted with time base.
    // Hence the '+=' works only on the subtracted values and here the value
    // will be based on that differences
    EXPECT_STREQ(timeString.c_str(), "Thu, 16 Sep 2032 23:00:46");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, OperatorOverloadingSubtraction)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time1(2002, 8, 8, 11, 30, 23, 22, true);
    Time time2(2000, 3, 1, 8, 25, 22, 1, true);

    Time time3 = time1 - time2;
    string timeString;
    time3.ToString(timeString);

    // Time will be saved/set based on 1st of January 1970, 00:00:00 time base.
    // So the saved time values will be always the one subtracted with time base.
    // Hence the '-=' works only on the subtracted values and here the value
    // will be based on that differences
    EXPECT_STREQ(timeString.c_str(), "Fri, 09 Jun 1972 03:05:01");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, OperatorOverloadingAdditionWithAssignment)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time1(2002, 5, 10, 11, 30, 23, 22, true);
    Time time2;
    time2 += time1;
    EXPECT_EQ(time1 == time2, true);

    Time time3(2000, 5, 10, 11, 30, 23, 22, true);
    time3 += time2;
    string timeString;
    time3.ToString(timeString);

    // Time will be saved/set based on 1st of January 1970, 00:00:00 time base.
    // So the saved time values will be always the one subtracted with time base.
    // Hence the '+=' works only on the subtracted values and here the value
    // will be based on that differences
    EXPECT_STREQ(timeString.c_str(), "Thu, 16 Sep 2032 23:00:46");

    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
TEST(Core_Time, OperatorOverloadingSubtractionWithAssignment)
{
    char* currentZone = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();

    Time time1(2002, 8, 8, 11, 30, 23, 22, true);
    Time time2(2000, 3, 1, 8, 25, 22, 1, true);;
    time1 -= time2;

    string timeString;
    time1.ToString(timeString);
    // Time will be saved/set based on 1st of January 1970, 00:00:00 time base.
    // So the saved time values will be always the one subtracted with time base.
    // Hence the '-=' works only on the subtracted values and here the value
    // will be based on that differences
    EXPECT_STREQ(timeString.c_str(), "Fri, 09 Jun 1972 03:05:01");
    (currentZone == nullptr) ? unsetenv("TZ") : setenv("TZ", currentZone, 1);
    tzset();
}
#if 0
TEST(Core_Time, TimeHandle)
{
    Time time(2021,1, 1, 12, 00, 00, 333, true);

    string timeString;
    time.ToString(timeString, true);
    std::cout << "Reference local 2021-1-1 12:00:00 333 : " << timeString << " ms: " << time.MilliSeconds() << std::endl;
    EXPECT_STREQ(timeString.c_str(), "");
    EXPECT_EQ(time.MilliSeconds(), 333u);
    std::cout << " DayOfYear:" << static_cast<uint32_t>(time.TMHandle().tm_yday) << " DayOfWeek:" << static_cast<uint32_t>(time.TMHandle().tm_wday) << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_yday), 0u);
    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_wday), 0u);

    std::cout << "Year:" << static_cast<uint32_t>(time.TMHandle().tm_year) << " Month:" << static_cast<uint32_t>(time.TMHandle().tm_mon) << " Day:" << static_cast<uint32_t>(.timeTMHandle().tm_mday) << " Hours:";

    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_year), 0u);
    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_mon), 0u);
    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_mday), 0u);

    std::cout << static_cast<uint32_t>(time.TMHandle().tm_hour) << " Minutes:" << static_cast<uint32_t>(.time.TMHandle().tm_min) << " Seconds:" << static_cast<uint32_t>(time.TMHandle().tm_sec) << std::endl;

    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_hour), 0u);
    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_min), 0u);
    EXPECT_EQ(static_cast<uint32_t>(time.TMHandle().tm_sec), 0u);
}
#endif
