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

    TEST(Core_DoorBell, simpleSet)
    {
        std::string fileName {"/tmp/doorbell01"};
        auto lambdaFunc = [fileName] (IPTestAdministrator & testAdmin) {
            ::Thunder::Core::DoorBell doorBell(fileName.c_str());

            EXPECT_EQ(doorBell.Wait(::Thunder::Core::infinite), ::Thunder::Core::ERROR_NONE);
            if (doorBell.Wait(::Thunder::Core::infinite) == ::Thunder::Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("First ring");
            }

            EXPECT_EQ(doorBell.Wait(::Thunder::Core::infinite), ::Thunder::Core::ERROR_NONE);
            if (doorBell.Wait(::Thunder::Core::infinite) == ::Thunder::Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("Second ring");
            }
            doorBell.Relinquish();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            ::Thunder::Core::DoorBell doorBell(fileName.c_str());
            ::SleepMs(10);
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        }
        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_DoorBell, simpleSetReversed)
    {
        std::string fileName {"/tmp/doorbell02"};
        auto lambdaFunc = [fileName] (IPTestAdministrator & testAdmin) {
            ::Thunder::Core::DoorBell doorBell(fileName.c_str());
            ::SleepMs(10);
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            ::Thunder::Core::DoorBell doorBell(fileName.c_str());
            EXPECT_EQ(doorBell.Wait(::Thunder::Core::infinite), ::Thunder::Core::ERROR_NONE);
            if (doorBell.Wait(::Thunder::Core::infinite) == ::Thunder::Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("First ring");
            }

            EXPECT_EQ(doorBell.Wait(::Thunder::Core::infinite), ::Thunder::Core::ERROR_NONE);
            if (doorBell.Wait(::Thunder::Core::infinite) == ::Thunder::Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("Second ring");
            }
            doorBell.Relinquish();
        }
        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
