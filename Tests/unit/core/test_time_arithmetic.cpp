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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include <ctime>

namespace Thunder {
namespace Tests {
namespace Core {

TEST(TIME_ARITHMETIC, DISABLED_gmtime)
{
    constexpr uint32_t initialMilliSeconds{ 0 };

    uint32_t offsetMilliSeconds{ initialMilliSeconds };

    constexpr uint32_t maxOffsetMilliSeconds{ 0xFFFFFFFF };

    // Seconds since Epoch
    time_t timer{ 0 };

    std::tm result;

    for (; offsetMilliSeconds <= maxOffsetMilliSeconds; offsetMilliSeconds++) {

        result.tm_sec = -1;
        result.tm_min = -1;
        result.tm_hour = -1;
        result.tm_mday = -1;
        result.tm_mon = -1;
        result.tm_year = -1;
        result.tm_wday = -1;
        result.tm_yday = -1;
        result.tm_isdst = -1;

        timer = offsetMilliSeconds / ::Thunder::Core::Time::MilliSecondsPerSecond;

        ASSERT_NE(gmtime_r(&timer, &result), nullptr);

        // 0...60
        ASSERT_GE(result.tm_sec, 0);
        ASSERT_LE(result.tm_sec, 60);

        // 0..59
        ASSERT_GE(result.tm_min, 0);
        ASSERT_LE(result.tm_min, 59);

        // 0..23
        ASSERT_GE(result.tm_hour, 0);
        ASSERT_LE(result.tm_hour, 60);

        // 1..31
        ASSERT_GE(result.tm_mday, 0);
        ASSERT_LE(result.tm_mday, 60);

        // 0..11
        ASSERT_GE(result.tm_mon, 0);
        ASSERT_LE(result.tm_mon, 60);

        // Since 1900
        ASSERT_GE(result.tm_year, 0);

        // 0...6
        ASSERT_GE(result.tm_wday, 0);
        ASSERT_LE(result.tm_wday, 6);

        // 0...365
        ASSERT_GE(result.tm_yday, 0);
        ASSERT_LE(result.tm_yday, 365);

        ASSERT_EQ(result.tm_sec, timer % 60);

    }
}

TEST(TIME_ARITHMETIC, DISABLED_Ticks)
{
    constexpr ::Thunder::Core::Time::microsecondsfromepoch initialEpochMicroSeconds{ 0 };

    constexpr ::Thunder::Core::Time::microsecondsfromepoch maxTimespecNanoSeconds{ 999999999 };

    constexpr ::Thunder::Core::Time::microsecondsfromepoch maxEpochMicroSeconds{ maxTimespecNanoSeconds / ::Thunder::Core::Time::NanoSecondsPerMicroSecond };
    static_assert(maxEpochMicroSeconds <= std::numeric_limits<::Thunder::Core::Time::microsecondsfromepoch>::max(), "" );

    ::Thunder::Core::Time::microsecondsfromepoch epochMicroSeconds{ initialEpochMicroSeconds };

    for (; epochMicroSeconds <= maxEpochMicroSeconds; epochMicroSeconds++) {

        ::Thunder::Core::Time time{ epochMicroSeconds /* microsecondsfromepoch */};

        ASSERT_EQ(time.Ticks(), epochMicroSeconds);

    }
}

TEST(TIME_ARITHMETIC, AddTime)
{
    constexpr uint32_t initialMilliSeconds{ 1 };

    uint32_t offsetMilliSeconds{ initialMilliSeconds };

    // Subset of full set of stimuli
    constexpr uint32_t maxOffsetMilliSeconds{ 1000 /* 0xFFFFFFFF */};

    for (; offsetMilliSeconds <= maxOffsetMilliSeconds; offsetMilliSeconds++) {

        ::Thunder::Core::Time::microsecondsfromepoch epochStart{ ::Thunder::Core::Time::Now().Ticks() /* microsecondsfromepoch */ };

        ::Thunder::Core::Time timeStart{ epochStart /* microsecondsfromepoch */};

        // Calendar date seconds 
        const uint8_t timeStartSeconds{ timeStart.Seconds() };

        ASSERT_EQ(timeStart.Seconds(), static_cast<uint8_t>((epochStart / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond)) % 60));

        ASSERT_GE(epochStart, (static_cast<::Thunder::Core::Time::microsecondsfromepoch>(timeStartSeconds) * static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MilliSecondsPerSecond)));

        ASSERT_EQ(epochStart, timeStart.Ticks() /* microsecondsfromepoch */);

        // Also updates timeStartMicroSeconds
        ::Thunder::Core::Time& timeFinish{ timeStart.Add(offsetMilliSeconds /* milliseconds */ ) };

        const uint8_t timeFinishSeconds{ timeFinish.Seconds()};

        ASSERT_TRUE(timeStart == timeFinish);

        ASSERT_EQ(timeStart.Seconds(), timeFinish.Seconds());

        ::Thunder::Core::Time::microsecondsfromepoch epochFinish{ epochStart + (static_cast<::Thunder::Core::Time::microsecondsfromepoch>(offsetMilliSeconds) * static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerMilliSecond)) };

        ASSERT_LT(epochStart, epochFinish);

        ASSERT_EQ(offsetMilliSeconds, (epochFinish - epochStart) / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerMilliSecond));

        ASSERT_EQ(epochFinish, timeStart.Ticks() /* microsecondsfromepoch */);

        ASSERT_EQ(epochFinish, timeFinish.Ticks() /* microsecondsfromepoch */);

        // Calendar time (units) cannot be compared without some effort
        ASSERT_EQ((timeFinishSeconds >= timeStartSeconds ? timeFinishSeconds - timeStartSeconds : static_cast<uint8_t>(60) + timeFinishSeconds - timeStartSeconds), static_cast<uint8_t>(((epochFinish / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond)) - (epochStart / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond))) % static_cast<::Thunder::Core::Time::microsecondsfromepoch>(60)));

    }
}

TEST(TIME_ARITHMETIC, SubTime)
{
    constexpr uint32_t initialMilliSeconds{ 1 };

    uint32_t offsetMilliSeconds{ initialMilliSeconds };

    // Subset of full set of stimuli
    constexpr uint32_t maxOffsetMilliSeconds{ 1000 /* 0xFFFFFFFF */ };

    for (; offsetMilliSeconds <= maxOffsetMilliSeconds; offsetMilliSeconds++) {

        ::Thunder::Core::Time::microsecondsfromepoch epochStart{ ::Thunder::Core::Time::Now().Ticks() /* microsecondsfromepoch */ };

        ASSERT_GE(epochStart, 0x3E7FFFFFC18 /* maxOffsetMilliSeconds * 10000 */);

        ::Thunder::Core::Time timeStart{ epochStart /* microsecondsfromepoch */};

        // Calendar date seconds 
        const uint8_t timeStartSeconds{ timeStart.Seconds() };

        ASSERT_EQ(timeStart.Seconds(), static_cast<uint8_t>((epochStart / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond)) % 60));

        ASSERT_GE(epochStart, (static_cast<::Thunder::Core::Time::microsecondsfromepoch>(timeStartSeconds) * static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MilliSecondsPerSecond)));

        ASSERT_EQ(epochStart, timeStart.Ticks() /* microsecondsfromepoch */);

        // Also updates timeStartMicroSeconds
        ::Thunder::Core::Time& timeFinish{ timeStart.Sub(offsetMilliSeconds /* milliseconds */ ) };

        const uint8_t timeFinishSeconds{ timeFinish.Seconds()};

        ASSERT_TRUE(timeStart == timeFinish);

        ASSERT_EQ(timeStart.Seconds(), timeFinish.Seconds());

        ::Thunder::Core::Time::microsecondsfromepoch epochFinish{ epochStart - (static_cast<::Thunder::Core::Time::microsecondsfromepoch>(offsetMilliSeconds) * static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerMilliSecond)) };

        ASSERT_GT(epochStart, epochFinish);

        ASSERT_EQ(offsetMilliSeconds, (epochStart - epochFinish) / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerMilliSecond));

        ASSERT_EQ(epochFinish, timeStart.Ticks() /* microsecondsfromepoch */);

        ASSERT_EQ(epochFinish, timeFinish.Ticks() /* microsecondsfromepoch */);

        // Calendar time (units) cannot be compared without some effort
        ASSERT_EQ((timeStartSeconds >= timeFinishSeconds ? timeStartSeconds - timeFinishSeconds : timeStartSeconds + (static_cast<uint8_t>(60) - timeFinishSeconds)), static_cast<uint8_t>(((epochStart / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond)) - (epochFinish / static_cast<::Thunder::Core::Time::microsecondsfromepoch>(::Thunder::Core::Time::MicroSecondsPerSecond))) % static_cast<::Thunder::Core::Time::microsecondsfromepoch>(60)));

    }
}

} // Core
} // Tests
} // Thunder
