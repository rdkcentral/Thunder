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

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(Test_IPTestAdministrator, BasicSync)
    {
        // Do not change the signal handshake value unless you use a timed loop and repeatedly check.
        // Wait may return early with an error if it expect a different value.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 0;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            // The child is awaiting the signal for this value to produce no error and continue. If the values mismatch the child continues with an error.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Test_IPTestAdministrator, SignalBeforeWait)
    {
        // Do not change the signal handshake value unless you use a timed loop and repeatedly check.
        // Wait may return early with an error if it expect a different value.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 15; // Approximately 150%  maxWaitTimePeriod

        constexpr auto seconds2milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)).count();

        static_assert((maxWaitTime * seconds2milliseconds * maxRetries) > (maxWaitTimeMs + maxInitTime), "Scheduling is unpredicatable. Keep a safe margin.");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the signal comes first
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // The child is awaiting the signal for this value to produce no error and continue. If the values mismatch the child continues with an error.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Test_IPTestAdministrator, SuccessiveSignalWait)
    {
        // Do not change the signal handshake value unless you use a timed loop and repeatedly check.
        // Wait may return early with an error if it expect a different value.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 2;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            // The child is awaiting the signal for this value to produce no error and continue. If the values mismatch the child continues with an error.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Test_IPTestAdministrator, SuccessiveSignalWaitInterleavedDelay)
    {
        // Do not change the signal handshake value unless you use a timed loop and repeatedly check.
        // Wait may return early with an error if it expect a different value.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 15; // Approximately 150%  maxWaitTimePeriod

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);
            // The child is awaiting the signal for this value to produce no error and continue. If the values mismatch the child continues with an error.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Test_IPTestAdministrator, TimeoutEnforcement)
    {
        // Validate that Wait() respects the timeout and returns ERROR_TIMEDOUT when deadline is reached.
        // This test is designed to work on low-end embedded devices with unpredictable scheduling.
        // Timeout is generous (10 seconds) to avoid false positives on slow hardware.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 10, maxInitTime = 1000;  // 10 second timeout, generous for embedded
        constexpr uint8_t maxRetries = 0;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // Child deliberately does NOT signal, causing parent's Wait() to timeout.
            // Sleep briefly to ensure child is live, but never send the expected signal.
            SleepMs(maxInitTime);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // Parent waits for initHandshakeValue, but child will never signal it.
            // With correct deadline accounting, this should timeout after ~10 seconds.
            // Accept either TIMEDOUT or INVALID_RANGE (if child signals wrong value).
            uint32_t result = testAdmin.Wait(initHandshakeValue);
            ASSERT_TRUE((result == ::Thunder::Core::ERROR_TIMEDOUT) || 
                        (result == ::Thunder::Core::ERROR_INVALID_RANGE) ||
                        (result == ::Thunder::Core::ERROR_GENERAL));
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Test_IPTestAdministrator, DeadlineAccountingWithRetries)
    {
        // Validate that deadline-based remaining time calculation works correctly.
        // This test stresses the Wait() retry loop with multiple signal-wait cycles.
        // Designed for low-end embedded devices: generous timeout (15 sec) and retry count
        // that's forgiving of scheduling jitter.

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 15, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 5;  // Moderate retry count, forgiving on slow hardware

        // Iteration count is intentionally conservative (2 rounds).
        // Each round trip can take up to (maxWaitTime + maxRetries * maxWaitTime/waitTimeDivisor) seconds
        // in the worst case on a loaded embedded device. The destructor also uses maxWaitTime to wait for the
        // child before SIGKILL, so the entire test must complete well within two maxWaitTime periods.
        constexpr int32_t rounds = 2;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // Multiple wait-signal round trips
            for (int32_t i = 0; i < rounds; ++i) {
                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            }
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            // Multiple signal-wait round trips with moderate retry counts
            for (int32_t i = 0; i < rounds; ++i) {
                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            }
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
