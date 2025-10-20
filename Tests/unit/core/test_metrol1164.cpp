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

    // Both processes close the used buffer and hence the underlying mapped file
    // Commit d41b8db695e8fead30b872535a9da9d2bf7b8bf7 prevents an assert at 
    // CyclicBuffer.cpp:222 due to the return value of FilseSystem.h at 449
    // remove() returning -1 and errno set to NOENTi for the second call. This
    // test should not fail with the patch applied.

    TEST(METROL_1164, CreateDestroy)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, VARIABLE_IS_NOT_USED maxInitTime = 2000;
        VARIABLE_IS_NOT_USED constexpr uint8_t maxRetries = 100;

        const std::string bufferName {"/tmp/CyclicBuffer-metrol1164"};

        constexpr uint32_t bufferSize = 1024 * 1024;

        constexpr uint32_t bufferMode =   ::Thunder::Core::File::Mode::USER_READ 
                                        | ::Thunder::Core::File::Mode::USER_WRITE
                                        | ::Thunder::Core::File::Mode::SHAREABLE
                                        ;

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, 0, false };

            EXPECT_TRUE(buffer.Open());

            buffer.Close();

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
           ::Thunder::Core::CyclicBuffer buffer{ bufferName.c_str(), bufferMode, bufferSize, false };

            EXPECT_TRUE(buffer.Open());

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
