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

#ifdef __APPLE__
#include <time.h>
#endif

#include <iostream>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    // The flow / code is copied from 'CyclicBuffer::SignalLock' with the
    // pthread_cond_timedwait is replaced by SleepMs for emulation purpose.
    // This POSIX 'equivalent' has been identified to occasionally produce
    // erroneous values for 'instant' fallthrough. The 'chrono' based equalient
    // has yet to show this behaviour. This test eventually may assert. Moreover,
    // the 'chrono' equivalent is supported by multiple platforms as it uses the
    // C++11 library.

    uint32_t ChronoTimeDuration(uint32_t waitTime)
    {
        const auto start = std::chrono::steady_clock::now();
        const auto finish = start + std::chrono::milliseconds(waitTime);

        const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(finish.time_since_epoch());
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(finish.time_since_epoch());

        struct timespec structTime = { seconds.count(), (nanoseconds - std::chrono::duration_cast<std::chrono::nanoseconds>(seconds)).count() };

        VARIABLE_IS_NOT_USED struct timespec dummystructTime = structTime;

        // Do some work eg sleep / wait
        // SleepMs(waitTime);

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

        // Prevent overflow
        // Prevent trend due to jitter
        using common_t = std::common_type<std::chrono::milliseconds::rep, uint32_t>::type;

        const uint32_t result = static_cast<uint32_t>(std::min(static_cast<common_t>(elapsed.count()), static_cast<common_t>(waitTime)));

        return result;
    }

    uint32_t PosixTimeDuration(uint32_t waitTime)
    {
        struct timespec structTime = {0,0};

        uint32_t result{ 0 };

        if (clock_gettime(CLOCK_MONOTONIC, &structTime) == 0) {
            structTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
            structTime.tv_sec += (waitTime / 1000); // + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
            structTime.tv_nsec = structTime.tv_nsec % 1000000000;

            // Do some work eg sleep / wait
            // SleepMs(waitTime);

            struct timespec nowTime = {0,0};

            if (clock_gettime(CLOCK_MONOTONIC, &nowTime) == 0) {

                // Added '=' to make it well-defined for equal values

                if (nowTime.tv_nsec > structTime.tv_nsec) {
                    result = (nowTime.tv_sec - structTime.tv_sec) * 1000 + ((nowTime.tv_nsec - structTime.tv_nsec) / 1000000);
                } else {
                    result = (nowTime.tv_sec - structTime.tv_sec - 1) * 1000 + ((1000000000 - (structTime.tv_nsec - nowTime.tv_nsec)) / 1000000);
                }

                // We can not wait longer than the set time.
                result = std::min(result, waitTime);

            } else {
                // Error
                VARIABLE_IS_NOT_USED int errval { errno };
                ASSERT(false);
            }
        } else {
            // Error
            VARIABLE_IS_NOT_USED int errval { errno };
            ASSERT(false);
        }

        return result;
    }

    TEST(METROL_1189, Insomnia)
    {
        constexpr uint32_t waitTime{ ::Thunder::Core::infinite };

        // Educated guess, nearly 'instant'
        constexpr uint32_t thresholdDuration{ 10 };

        uint32_t durationPosix { 0 };
        uint32_t durationChrono { 0 };

        do {

            durationPosix = PosixTimeDuration(waitTime);
            ASSERT_GE(thresholdDuration, durationPosix);

            durationChrono = ChronoTimeDuration(waitTime);
            ASSERT_GE(thresholdDuration, durationChrono);

        } while (true);

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
