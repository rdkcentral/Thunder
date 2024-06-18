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

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(test_stopWatch, Elapsed_WithoutDelay)
    {
        Thunder::Core::StopWatch stopWatch1, stopWatch2;
        uint64_t elapsed1 = stopWatch1.Elapsed();
        uint64_t elapsed2 = stopWatch2.Elapsed();
        EXPECT_EQ(elapsed1 / Thunder::Core::Time::MilliSecondsPerSecond, elapsed2 / Thunder::Core::Time::MilliSecondsPerSecond);
    }
    TEST(test_stopWatch, Elapsed_WithDelay)
    {
        int8_t wait = 1;
        Thunder::Core::StopWatch stopWatch1, stopWatch2;
        uint64_t elapsed1 = stopWatch1.Elapsed();
        EXPECT_EQ(stopWatch1.Elapsed() / Thunder::Core::Time::MicroSecondsPerSecond, (stopWatch1.Elapsed() / Thunder::Core::Time::MicroSecondsPerSecond));
        sleep(wait);
        uint64_t elapsed2 = stopWatch2.Elapsed();
        uint64_t elapsed3 = stopWatch1.Elapsed();
        EXPECT_EQ(elapsed2 / Thunder::Core::Time::MicroSecondsPerSecond, (elapsed1 / Thunder::Core::Time::MicroSecondsPerSecond) + wait);
        EXPECT_EQ(elapsed3 / Thunder::Core::Time::MicroSecondsPerSecond, (elapsed1 / Thunder::Core::Time::MicroSecondsPerSecond) + wait);
        EXPECT_EQ(elapsed3 / Thunder::Core::Time::MicroSecondsPerSecond, (elapsed2 / Thunder::Core::Time::MicroSecondsPerSecond));
    }
    TEST(test_stopWatch, Reset_WithoutDelay)
    {
        Thunder::Core::StopWatch stopWatch1, stopWatch2;
        uint64_t elapsed1 = stopWatch1.Elapsed();
        uint64_t reset1 = stopWatch1.Reset();
        uint64_t elapsed2 = stopWatch2.Elapsed();
        uint64_t reset2 = stopWatch2.Reset();

        // Microsecond comparison is not doing
        // instead simply checking reset is a value after elapsed one
        EXPECT_LE(elapsed1, reset1);
        EXPECT_LE(elapsed2, reset2);

        elapsed1 = stopWatch1.Elapsed();
        elapsed2 = stopWatch2.Elapsed();

        // Microsecond comparison is not doing
        // instead simply checking converting to millisecond
        EXPECT_EQ(elapsed1 / Thunder::Core::Time::MilliSecondsPerSecond, reset1 / Thunder::Core::Time::MilliSecondsPerSecond);
        EXPECT_EQ(elapsed2 / Thunder::Core::Time::MilliSecondsPerSecond, reset2 / Thunder::Core::Time::MilliSecondsPerSecond);
    }
    TEST(test_stopWatch, Reset_WithDelay)
    {
        int8_t wait = 1;
        Thunder::Core::StopWatch stopWatch1, stopWatch2;
        uint64_t elapsed1 = stopWatch1.Elapsed();
        uint64_t elapsed2 = stopWatch2.Elapsed();
        sleep(wait);
        uint64_t reset1 = stopWatch1.Reset();
        uint64_t reset2 = stopWatch2.Reset();

        EXPECT_EQ(reset1 / Thunder::Core::Time::MicroSecondsPerSecond, (elapsed1 / Thunder::Core::Time::MicroSecondsPerSecond) + wait);
        EXPECT_EQ(reset2 / Thunder::Core::Time::MicroSecondsPerSecond, (elapsed2 / Thunder::Core::Time::MicroSecondsPerSecond) + wait);

        sleep(wait);
        elapsed1 = stopWatch1.Elapsed();
        sleep(wait);
        elapsed2 = stopWatch2.Elapsed();

        EXPECT_EQ(elapsed1 / Thunder::Core::Time::MicroSecondsPerSecond, (reset1 / Thunder::Core::Time::MicroSecondsPerSecond));
        EXPECT_EQ(elapsed2 / Thunder::Core::Time::MicroSecondsPerSecond, (reset2 / Thunder::Core::Time::MicroSecondsPerSecond) + wait);
    }

} // Core
} // Tests
} // Thunder
