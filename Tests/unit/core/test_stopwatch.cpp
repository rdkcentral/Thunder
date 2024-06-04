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

using namespace Thunder;
using namespace Thunder::Core;

TEST(test_stopWatch, Elapsed_WithoutDelay)
{
    StopWatch stopWatch1, stopWatch2;
    uint64_t elapsed1 = stopWatch1.Elapsed();
    uint64_t elapsed2 = stopWatch2.Elapsed();
    EXPECT_EQ(elapsed1 /Time::MilliSecondsPerSecond, elapsed2 / Time::MilliSecondsPerSecond);
}
TEST(test_stopWatch, Elapsed_WithDelay)
{
    int8_t wait = 1;
    StopWatch stopWatch1, stopWatch2;
    uint64_t elapsed1 = stopWatch1.Elapsed();
    EXPECT_EQ(stopWatch1.Elapsed() / Time::MicroSecondsPerSecond, (stopWatch1.Elapsed() / Time::MicroSecondsPerSecond));
    sleep(wait);
    uint64_t elapsed2 = stopWatch2.Elapsed();
    uint64_t elapsed3 = stopWatch1.Elapsed();
    EXPECT_EQ(elapsed2 / Time::MicroSecondsPerSecond, (elapsed1 / Time::MicroSecondsPerSecond) + wait);
    EXPECT_EQ(elapsed3 / Time::MicroSecondsPerSecond, (elapsed1 / Time::MicroSecondsPerSecond) + wait);
    EXPECT_EQ(elapsed3 / Time::MicroSecondsPerSecond, (elapsed2 / Time::MicroSecondsPerSecond));
}
TEST(test_stopWatch, Reset_WithoutDelay)
{
    StopWatch stopWatch1, stopWatch2;
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
    EXPECT_EQ(elapsed1 / Time::MilliSecondsPerSecond, reset1 / Time::MilliSecondsPerSecond);
    EXPECT_EQ(elapsed2 / Time::MilliSecondsPerSecond, reset2 / Time::MilliSecondsPerSecond);
}
TEST(test_stopWatch, Reset_WithDelay)
{
    int8_t wait = 1;
    StopWatch stopWatch1, stopWatch2;
    uint64_t elapsed1 = stopWatch1.Elapsed();
    uint64_t elapsed2 = stopWatch2.Elapsed();
    sleep(wait);
    uint64_t reset1 = stopWatch1.Reset();
    uint64_t reset2 = stopWatch2.Reset();

    EXPECT_EQ(reset1 / Time::MicroSecondsPerSecond, (elapsed1 / Time::MicroSecondsPerSecond) + wait);
    EXPECT_EQ(reset2 / Time::MicroSecondsPerSecond, (elapsed2 / Time::MicroSecondsPerSecond) + wait);

    sleep(wait);
    elapsed1 = stopWatch1.Elapsed();
    sleep(wait);
    elapsed2 = stopWatch2.Elapsed();

    EXPECT_EQ(elapsed1 / Time::MicroSecondsPerSecond, (reset1 / Time::MicroSecondsPerSecond));
    EXPECT_EQ(elapsed2 / Time::MicroSecondsPerSecond, (reset2 / Time::MicroSecondsPerSecond) + wait);
}
