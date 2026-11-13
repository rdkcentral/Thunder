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

#ifdef __POSIX__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(Core_SharedBuffer, simpleSet)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 6, maxWaitTimeMs = 6000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 20;

        constexpr uint16_t administrationSize = 64;
        constexpr uint32_t bufferSize = 8 * 1024;

        const std::string bufferName {"testbuffer01"} ;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the parent can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::SharedBuffer buff01(bufferName.c_str(),
                ::Thunder::Core::File::USER_READ    |
                ::Thunder::Core::File::USER_WRITE   |
                ::Thunder::Core::File::USER_EXECUTE |
                ::Thunder::Core::File::GROUP_READ   |
                ::Thunder::Core::File::GROUP_WRITE  ,
                bufferSize,
                administrationSize);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buff01.RequestProduce(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            uint8_t* buffer = buff01.Buffer();

            ASSERT_TRUE(buffer != nullptr);
            EXPECT_EQ(buff01.Size(), bufferSize);

            buffer[0] = 42;
            buffer[1] = 43;
            buffer[2] = 44;

            EXPECT_EQ(buff01.Produced(), ::Thunder::Core::ERROR_NONE);

            // Data made available
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Buffer no longer used
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::SharedBuffer buff01(bufferName.c_str());

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buff01.RequestConsume(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            uint8_t* buffer = buff01.Buffer();

            ASSERT_TRUE(buffer != nullptr);
            EXPECT_EQ(buff01.Size(), bufferSize);

            EXPECT_EQ(buffer[0], 42);
            EXPECT_EQ(buffer[1], 43);
            EXPECT_EQ(buffer[2], 44);

            EXPECT_EQ(buff01.Consumed(), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_SharedBuffer, simpleSetReversed)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 6, maxWaitTimeMs = 6000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 20;

        VARIABLE_IS_NOT_USED constexpr uint16_t administrationSize = 64;
        constexpr uint32_t bufferSize = 8 * 1024;

        const std::string bufferName {"testbuffer02"} ;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // In extra scope, to make sure "buff01" is destructed before producer.

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::SharedBuffer buff01(bufferName.c_str());

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buff01.RequestConsume(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            uint8_t* buffer = buff01.Buffer();

            ASSERT_TRUE(buffer != nullptr);
            EXPECT_EQ(buff01.Size(), bufferSize);

            EXPECT_EQ(buffer[0], 42);
            EXPECT_EQ(buffer[1], 43);
            EXPECT_EQ(buffer[2], 44);

            EXPECT_EQ(buff01.Consumed(), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
           // a small delay so the child can be set up
            SleepMs(maxInitTime);

            uint16_t administrationSize = 64;
            uint32_t bufferSize = 8 * 1024;
            VARIABLE_IS_NOT_USED uint32_t result;

            ::Thunder::Core::SharedBuffer buff01(bufferName.c_str(),
                ::Thunder::Core::File::USER_READ    |
                ::Thunder::Core::File::USER_WRITE   |
                ::Thunder::Core::File::USER_EXECUTE |
                ::Thunder::Core::File::GROUP_READ   |
                ::Thunder::Core::File::GROUP_WRITE  ,
                bufferSize,
                administrationSize);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buff01.RequestProduce(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            uint8_t* buffer = buff01.Buffer();

            ASSERT_TRUE(buffer != nullptr);
            EXPECT_EQ(buff01.Size(), bufferSize);

            buffer[0] = 42;
            buffer[1] = 43;
            buffer[2] = 44;

            EXPECT_EQ(buff01.Produced(), Thunder::Core::ERROR_NONE);

            // Data made available
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Buffer no longer used
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

#ifdef __POSIX__
    TEST(Core_SharedBuffer, RejectsSymlinkedAdministrationFile)
    {
        char directoryTemplate[] = "/tmp/thunder-sharedbuffer-admin-XXXXXX";
        char* directory = ::mkdtemp(directoryTemplate);
        ASSERT_TRUE(directory != nullptr);

        const std::string bufferName = std::string(directory) + "/buffer";
        const std::string administrationName = bufferName + ".admin";
        const std::string targetName = std::string(directory) + "/target";
        constexpr char targetContents[] = "protected";
        constexpr uint32_t mode = ::Thunder::Core::File::USER_READ | ::Thunder::Core::File::USER_WRITE;

        int target = ::open(targetName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        ASSERT_TRUE(target >= 0);
        ASSERT_EQ(::write(target, targetContents, sizeof(targetContents) - 1), static_cast<ssize_t>(sizeof(targetContents) - 1));
        ASSERT_EQ(::close(target), 0);
        ASSERT_EQ(::symlink(targetName.c_str(), administrationName.c_str()), 0);

        {
            ::Thunder::Core::SharedBuffer buffer(bufferName.c_str(), mode, 4096, 64);

            EXPECT_FALSE(buffer.IsValid());
            EXPECT_EQ(buffer.RequestProduce(0), ::Thunder::Core::ERROR_UNAVAILABLE);
            EXPECT_EQ(buffer.Produced(), ::Thunder::Core::ERROR_UNAVAILABLE);
            EXPECT_EQ(buffer.RequestConsume(0), ::Thunder::Core::ERROR_UNAVAILABLE);
            EXPECT_EQ(buffer.Consumed(), ::Thunder::Core::ERROR_UNAVAILABLE);
            EXPECT_FALSE(buffer.Load());
        }

        struct stat targetStatus;
        ASSERT_EQ(::stat(targetName.c_str(), &targetStatus), 0);
        EXPECT_EQ(targetStatus.st_mode & 0777, 0600);

        char contents[sizeof(targetContents)] = {};
        target = ::open(targetName.c_str(), O_RDONLY);
        ASSERT_TRUE(target >= 0);
        ASSERT_EQ(::read(target, contents, sizeof(contents)), static_cast<ssize_t>(sizeof(targetContents) - 1));
        ASSERT_EQ(::close(target), 0);
        EXPECT_EQ(std::string(contents), targetContents);

        EXPECT_EQ(::unlink(bufferName.c_str()), 0);
        EXPECT_EQ(::unlink(administrationName.c_str()), 0);
        EXPECT_EQ(::unlink(targetName.c_str()), 0);
        EXPECT_EQ(::rmdir(directory), 0);
    }

    TEST(Core_SharedBuffer, PayloadSymlinkKeepsLegacyBehavior)
    {
        char directoryTemplate[] = "/tmp/thunder-sharedbuffer-payload-XXXXXX";
        char* directory = ::mkdtemp(directoryTemplate);
        ASSERT_TRUE(directory != nullptr);

        const std::string bufferName = std::string(directory) + "/buffer";
        const std::string administrationName = bufferName + ".admin";
        const std::string targetName = std::string(directory) + "/target";
        constexpr uint32_t mode = ::Thunder::Core::File::USER_READ | ::Thunder::Core::File::USER_WRITE;

        int target = ::open(targetName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        ASSERT_TRUE(target >= 0);
        ASSERT_EQ(::close(target), 0);
        ASSERT_EQ(::symlink(targetName.c_str(), bufferName.c_str()), 0);

        {
            ::Thunder::Core::File file(bufferName);
            EXPECT_TRUE(file.Create(mode));
            file.Close();
        }

        {
            ::Thunder::Core::SharedBuffer buffer(bufferName.c_str(), mode, 4096, 64);
            EXPECT_TRUE(buffer.IsValid());
            EXPECT_TRUE(buffer.Buffer() != nullptr);
            EXPECT_EQ(buffer.Size(), 4096u);
        }

        struct stat targetStatus;
        ASSERT_EQ(::stat(targetName.c_str(), &targetStatus), 0);
        EXPECT_EQ(targetStatus.st_size, 4096);

        EXPECT_EQ(::unlink(administrationName.c_str()), 0);
        EXPECT_EQ(::unlink(bufferName.c_str()), 0);
        EXPECT_EQ(::unlink(targetName.c_str()), 0);
        EXPECT_EQ(::rmdir(directory), 0);
    }
#endif

} // Core
} // Tests
} // Thunder
