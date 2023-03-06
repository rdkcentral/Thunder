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

namespace WPEFramework {
namespace Tests {

    void CleanUpBuffer(string bufferName)
    {
        // TODO: shouldn't this be done producer-side?
        char systemCmd[1024];
        string command = "rm -rf ";
        snprintf(systemCmd, command.size() + bufferName.size() + 1, "%s%s", command.c_str(), bufferName.c_str());
        system(systemCmd);

        string ext = ".admin";
        snprintf(systemCmd, command.size() + bufferName.size() + ext.size() + 1, "%s%s%s", command.c_str(), bufferName.c_str(), ext.c_str());
        system(const_cast<char*>(systemCmd));
    }

    TEST(Core_SharedBuffer, simpleSet)
    {
        std::string bufferName {"testbuffer01"} ;
        auto lambdaFunc = [bufferName](IPTestAdministrator & testAdmin) {
            CleanUpBuffer(bufferName);

            uint16_t administrationSize = 64;
            uint32_t bufferSize = 8 * 1024;
            uint32_t result;

            Core::SharedBuffer buff01(bufferName.c_str(),
                Core::File::USER_READ    |
                Core::File::USER_WRITE   |
                Core::File::USER_EXECUTE |
                Core::File::GROUP_READ   |
                Core::File::GROUP_WRITE  ,
                bufferSize,
                administrationSize);
            result = buff01.RequestProduce(Core::infinite);
            EXPECT_EQ(result, Core::ERROR_NONE);

            testAdmin.Sync("setup producer");

            testAdmin.Sync("setup consumer");

            uint8_t * buffer = buff01.Buffer();
            EXPECT_EQ(buff01.Size(), bufferSize);

            buffer[0] = 42;
            buffer[1] = 43;
            buffer[2] = 44;

            result = buff01.Produced();
            EXPECT_EQ(result, Core::ERROR_NONE);

            testAdmin.Sync("consumer done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        // This side (tested) acts as client (consumer).
        IPTestAdministrator testAdmin(otherSide);
        {
            // In extra scope, to make sure "buff01" is destructed before producer.
            testAdmin.Sync("setup producer");

            uint32_t bufferSize = 8 * 1024;
            uint32_t result;
            Core::SharedBuffer buff01(bufferName.c_str());

            testAdmin.Sync("setup consumer");

            result = buff01.RequestConsume(Core::infinite);
            EXPECT_EQ(result, Core::ERROR_NONE);

            uint8_t * buffer = buff01.Buffer();
            EXPECT_EQ(buff01.Size(), bufferSize);

            EXPECT_EQ(buffer[0], 42);
            EXPECT_EQ(buffer[1], 43);
            EXPECT_EQ(buffer[2], 44);

            buff01.Consumed();
            EXPECT_EQ(result, Core::ERROR_NONE);
        }

        testAdmin.Sync("consumer done");

        Core::Singleton::Dispose();
    }

    TEST(Core_SharedBuffer, simpleSetReversed)
    {
        std::string bufferName {"testbuffer02"} ;
        auto lambdaFunc = [bufferName](IPTestAdministrator & testAdmin) {
            // In extra scope, to make sure "buff01" is destructed before producer.

            testAdmin.Sync("setup consumer");

            uint32_t bufferSize = 8 * 1024;
            uint32_t result;
            Core::SharedBuffer buff01(bufferName.c_str());

            testAdmin.Sync("setup producer");

            result = buff01.RequestConsume(Core::infinite);
            EXPECT_EQ(result, Core::ERROR_NONE);

            uint8_t * buffer = buff01.Buffer();
            EXPECT_EQ(buff01.Size(), bufferSize);

            EXPECT_EQ(buffer[0], 42);
            EXPECT_EQ(buffer[1], 43);
            EXPECT_EQ(buffer[2], 44);

            buff01.Consumed();
            EXPECT_EQ(result, Core::ERROR_NONE);

            testAdmin.Sync("producer done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        // This side (tested) acts as client (consumer).
        IPTestAdministrator testAdmin(otherSide);
        {
            CleanUpBuffer(bufferName);

            uint16_t administrationSize = 64;
            uint32_t bufferSize = 8 * 1024;
            uint32_t result;

            Core::SharedBuffer buff01(bufferName.c_str(),
                Core::File::USER_READ    |
                Core::File::USER_WRITE   |
                Core::File::USER_EXECUTE |
                Core::File::GROUP_READ   |
                Core::File::GROUP_WRITE  ,
                bufferSize,
                administrationSize);
            result = buff01.RequestProduce(Core::infinite);
            EXPECT_EQ(result, Core::ERROR_NONE);

            testAdmin.Sync("setup consumer");

            testAdmin.Sync("setup producer");

            uint8_t * buffer = buff01.Buffer();
            EXPECT_EQ(buff01.Size(), bufferSize);

            buffer[0] = 42;
            buffer[1] = 43;
            buffer[2] = 44;

            result = buff01.Produced();
            EXPECT_EQ(result, Core::ERROR_NONE);
        }

        testAdmin.Sync("producer done");

        CleanUpBuffer(bufferName);
        Core::Singleton::Dispose();
    }
} // Tests
} // WPEFramework
