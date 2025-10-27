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

// The tests are 'equivalent' to the flow of ::Thunder::Core::CyclicBuffer without (see 
// TimedAccessWrong) and with (see Timed AccessGood) the proposed path applied. They 
// illustrate that no timely pthread_cond_wait and pthread_cond_timedwait signal is received
// and the 'full' time is spent waiting. However, without the patch applied the time 'spent'
// waiting is significantly reduced and unexpectedly defiates from the expected value 
// waitTime.

namespace Thunder {
namespace Tests {
namespace Core {

    template <typename DERIVED>
    class TimedAccess {
public :

        TimedAccess(const TimedAccess&) = delete;
        TimedAccess(TimedAccess&&) = delete;
        TimedAccess& operator=(const TimedAccess&) = delete;
        TimedAccess& operator=(TimedAccess&&) = delete;

        ~TimedAccess()
        {}

        uint32_t Lock(uint32_t waitTime = ::Thunder::Core::infinite)
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {
                result = static_cast<DERIVED*>(this)->LockImplementation(waitTime);
            }

            return result;
        }

        uint32_t Unlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {

                // Flow 'copied' at ::Thunder::Core::CyclicBuffer is:
                // 1. AdminLock()
                // 2. pthread_cond_signal
                // 3. AdminUnlock()
                //
                // Here assume Unlock is always succesful.

                result = ::Thunder::Core::ERROR_NONE;

            }

            return result;
        }

        int Error() const
        {
            return _errval;
        }

        constexpr uint32_t TimeScaleFactor() const
        {
            // Some arbitrary value
            constexpr uint32_t result{ 4 };
            return result;
        }

    protected :

        TimedAccess()
        {}

        uint32_t AdminLock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            // Here assume AdminLock is always succesful

            return result;
        }

        uint32_t AdminUnlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            // Here assume AdminUnlock is always succesful

            return result;
        }

        uint32_t SignalLock(const uint32_t waitTime)
        {
            uint32_t result{ 0 };

            if (_initialized != false) {

                // The actual initialization value
                result = static_cast<DERIVED*>(this)->SignalLockImplementation(waitTime);

                if (waitTime != ::Thunder::Core::infinite) {

                    // Flow 'copied' at ::Thunder::Core::CyclicBuffer is:
                    // 1. clock_gettime
                    // 2. adjust maximum time to wait
                    // 3. pthread_cond_timedwait
                    //
                    // Here assume it models some time is spent sleeping, eg, duration

                    struct timespec structTime{ 0, 0 };

                    if (clock_gettime(CLOCK_MONOTONIC, &structTime) != 0) {
                        _errval = errno;
                    }

                    // Simulate a wake up, eg, less time spent sleeping than waitTime,
                    // as a consequence of a 'received' pthread_cond_signal / pthread_cond_broadcast

                    SleepMs(waitTime / TimeScaleFactor());

                    struct timespec nowTime{ 0, 0 };

                    if (clock_gettime(CLOCK_MONOTONIC, &nowTime) != 0) {
                        _errval = errno;
                    }

                    if (nowTime.tv_nsec > structTime.tv_nsec) {
                        result = (nowTime.tv_sec - structTime.tv_sec) * 1000 + ((nowTime.tv_nsec - structTime.tv_nsec) / 1000000);
                    } else {
                        result = (nowTime.tv_sec - structTime.tv_sec - 1) * 1000 + ((1000000000 - (structTime.tv_nsec - nowTime.tv_nsec)) / 1000000);
                    }

                } else {

                }
            }

            return result;
        }

        int _errval{ 0 };

    private :

        bool Initialize()
        {
            bool result{ true };
            return result;
        }

        const bool _initialized{ Initialize() };
    };

    class TimedAccessWrong : public TimedAccess<TimedAccessWrong> {
    public :
        TimedAccessWrong(const TimedAccessWrong&) = delete;
        TimedAccessWrong(TimedAccessWrong&&) = delete;
        TimedAccessWrong& operator=(const TimedAccessWrong&) = delete;
        TimedAccessWrong& operator=(TimedAccessWrong&&) = delete;

        TimedAccessWrong() = default;
        ~TimedAccessWrong() = default;

        uint32_t LockImplementation(uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            uint32_t timeLeft{ waitTime };

            // In the original ::Thunder::Core::CyclicBuffer the flow is:
            // 1. AdminLock
            // 2. Do while
            // 3. AdminUnlock
            //
            // Here assume 1 and 3 always succeed to simplify code

            do {

                // Force the second branch for illustrative purpose

                if (false) {

                } else if (true) {

                    // In the original :: Thunder::Core::CyclicBuffer the flow is:
                    // 1. AdminUnlock
                    // 2. SignalLock
                    // 3. AdminLock
                    //
                    // Here assume 1 and 3 always succeed to simplify code

                    timeLeft = SignalLock(timeLeft);
                }

            } while (timeLeft > 0);

            return result;
        }

        uint32_t SignalLockImplementation(const uint32_t waitTime)
        {
            uint32_t result{ waitTime };
            return result;
        }

    private :
    };

    class TimedAccessGood : public TimedAccess<TimedAccessGood> {
    public :
        TimedAccessGood(const TimedAccessGood&) = delete;
        TimedAccessGood(TimedAccessGood&&) = delete;
        TimedAccessGood& operator=(const TimedAccessGood&) = delete;
        TimedAccessGood& operator=(TimedAccessGood&&) = delete;

        TimedAccessGood() = default;
        ~TimedAccessGood() = default;

        uint32_t LockImplementation(uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_TIMEDOUT };

            uint32_t timeLeft{ waitTime };

            // In the original ::Thunder::Core::CyclicBuffer the flow is:
            // 1. AdminLock
            // 2. Do while
            // 3. AdminUnlock
            //
            // Here assume 1 and 3 always succeed to simplify code

            do {

                // Force the second branch for illustrative purpose

                if (false) {

                } else if (true) {

                    // In the original :: Thunder::Core::CyclicBuffer the flow is:
                    // 1. AdminUnlock
                    // 2. SignalLock
                    // 3. AdminLock

                    // The proposed flow is:
                    // 1. SignalLock
                    // 2. AdminUnlock
                    // 3. AdminLock
                    //
                    // Here assume 2 and 3 always succeed to simplify code

                    waitTime = timeLeft;

                    timeLeft -= SignalLock(waitTime);
                }

            } while ((timeLeft > 0) && (waitTime > timeLeft));

            return result;
        }

        uint32_t SignalLockImplementation(VARIABLE_IS_NOT_USED const uint32_t waitTime)
        {
            uint32_t result{ 0 };
            return result;
        }

    private :

    };


    // Illustrate the result without the proposed patch applied
    // As a result, this test fails as is 'completes' significantly
    // prematurely.
    TEST(METROL_1192, AccessWrong)
    {
        constexpr uint32_t maxWaitTime{ 1000 };

        TimedAccessWrong access;

        ::Thunder::Core::StopWatch timer;

        uint64_t duration{ timer.Reset() };

        EXPECT_EQ(access.Lock(maxWaitTime), ::Thunder::Core::ERROR_TIMEDOUT);

        duration = timer.Elapsed();

        EXPECT_EQ(access.Error(), 0);

        EXPECT_EQ(access.Unlock(), ::Thunder::Core::ERROR_NONE);

        // GE: The code execution introduces a slight overhead / inaccuracy
        // For A TimedSacaleFactor() 4 the total duration is approximately 
        // 1/3 of maxWaitTime
        EXPECT_GE(duration / 1000, static_cast<uint64_t>(maxWaitTime));

        ::Thunder::Core::Singleton::Dispose();
    }

    // Illustrate the result with the proposed patch applied
    // As a result, this test succeeds.
    TEST(METROL_1192, AccessGood)
    {
        constexpr uint32_t maxWaitTime{ 1000 };

        TimedAccessGood access;

        ::Thunder::Core::StopWatch timer;

        uint64_t duration{ timer.Reset() };

        EXPECT_EQ(access.Lock(maxWaitTime), ::Thunder::Core::ERROR_TIMEDOUT);

        EXPECT_EQ(access.Error(), 0);

        EXPECT_EQ(access.Unlock(), ::Thunder::Core::ERROR_NONE);

        duration = timer.Elapsed();

        // GE: The code execution introduces a slight overhead / inaccuracy
        EXPECT_GE(duration / 1000, static_cast<uint64_t>(maxWaitTime));

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
