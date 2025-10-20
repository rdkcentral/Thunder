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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    // The 'creator' of the CyclicBuffer triggers an assert at CyclicBuffer at 45
    // as it is mandatory to pass File::USER_WRITE mode flag on creation of the 
    // underlying mapped file. Commit 4775257abbefbae28fdcfcce9b81f10aed5691f6
    // prevents that assert. It allows the user not to be aware of the implicit
    // requirement, and, avoids runtime overhead on enabled ASSERT macros. This
    // test should not fail with the patch applied.

    TEST(METROL_1165, WriteAccess)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, VARIABLE_IS_NOT_USED maxInitTime = 2000;
        constexpr uint8_t maxRetries = 100;

        const std::string bufferName {"/tmp/CyclicBuffer-metrol1165"};

        constexpr uint32_t bufferSize = 1024 * 1024;

        constexpr uint32_t bufferMode =   ::Thunder::Core::File::Mode::USER_READ 
//                                        | ::Thunder::Core::File::Mode::USER_WRITE
                                        | ::Thunder::Core::File::Mode::SHAREABLE
                                        ;

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, 0, false };

            EXPECT_TRUE(buffer.Open());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
           ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, bufferSize, false };

            EXPECT_TRUE(buffer.Open());

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    // A recursive lock tighly coupled to ownership. Distinct threads and
    // processes can typically simultaneoulsy try to acquire the lock, or, one can
    // try to acquire a lock while the other already has. Two running processes
    // might assert at CyclicBuffer 573 despite following the proper operation flow.
    // This test should not fail with the patch applied.

    TEST(METROL_1165, NoneZeroPid)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, VARIABLE_IS_NOT_USED maxInitTime = 2000;
        constexpr uint8_t maxRetries = 100;

        const std::string bufferName {"/tmp/CyclicBuffer-metrol1165"};

        constexpr uint32_t bufferSize = 1024 * 1024;

        constexpr uint32_t bufferMode =   ::Thunder::Core::File::Mode::USER_READ
//                                        | ::Thunder::Core::File::Mode::USER_WRITE
                                        | ::Thunder::Core::File::Mode::SHAREABLE
                                        ;

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, 0, false };

            EXPECT_TRUE(buffer.Open());

            while (true) {
                buffer.Lock();
                SleepMs(10);
                buffer.Unlock();
            }

        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
           ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, bufferSize, false };

            EXPECT_TRUE(buffer.Open());

            while (true) {
                buffer.Lock();
                SleepMs(10);
                buffer.Unlock();
            }
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
