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

namespace WPEFramework {
namespace Core {
namespace Tests {

    TEST(Core_CyclicBuffer, DataExchange)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, VARIABLE_IS_NOT_USED maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string bufferName {"/tmp/SharedCyclicBuffer"};

        constexpr uint32_t memoryMappedFileRequestedSize = 30;//446;

        // https://en.wikipedia.org/wiki/Lorem_ipsum
        #define LOREM_IPSUM_TEXT "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
        const std::array<uint8_t, sizeof(LOREM_IPSUM_TEXT)> reference { "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum." };

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Writer
            Core::CyclicBuffer buffer {  bufferName
                                       ,   Core::File::USER_READ  // Not relevant for readers
                                         | Core::File::USER_WRITE // Open the existing file with write permission
                                           // Readers normally require readonly but the cyclic buffer administration is updated after read
//                                       | Core::File::CREATE     // Readers access exisitng underlying memory mapped files
                                         | Core::File::SHAREABLE  // Updates are visible to other processes, but a reader should not update any content except when read data is (automatically) removed
                                       , 0 // Readers do not specify sizes
                                       , false // Overwrite unread data
                                      };

            ASSERT_TRUE(   buffer.IsValid()
                        && buffer.Open() // It already should
                        && buffer.Storage().Name() == bufferName
                        && Core::File(bufferName).Exists()
                        && buffer.Storage().Exists()
                       );

            ASSERT_EQ(buffer.Size(), memoryMappedFileRequestedSize);

            uint32_t written = buffer.Write(reference.data(), std::min(sizeof(LOREM_IPSUM_TEXT), static_cast<size_t>(memoryMappedFileRequestedSize)) - 1);
            EXPECT_EQ(written, std::min(sizeof(LOREM_IPSUM_TEXT), static_cast<size_t>(memoryMappedFileRequestedSize)) - 1);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // Writer
            Core::CyclicBuffer buffer {   bufferName
                                        ,   Core::File::USER_READ  // Enable read permissions on the underlying file for other users
                                          | Core::File::USER_WRITE // Enable write permission on the underlying file
                                          | Core::File::CREATE // Create a new underlying memory mapped file
                                          | Core::File::SHAREABLE  // Allow other processes to access the content of the file
                                        , memoryMappedFileRequestedSize // Requested size
                                        , false // Overwrite unread data
                                     };

            ASSERT_TRUE(   buffer.IsValid()
                        && buffer.Open() // It already should
                        && buffer.Storage().Name() == bufferName
                        && Core::File(bufferName).Exists()
                        && buffer.Storage().Exists()
                       );

            ASSERT_EQ(buffer.Size(), memoryMappedFileRequestedSize);

            std::array<uint8_t, sizeof(LOREM_IPSUM_TEXT) + 1> data;
            data.fill('\0');

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint32_t read = buffer.Read(data.data(), std::min(sizeof(data), static_cast<size_t>(memoryMappedFileRequestedSize)) - 1);
            EXPECT_EQ(read, std::min(sizeof(LOREM_IPSUM_TEXT), static_cast<size_t>(memoryMappedFileRequestedSize)) - 1);

            bool result = true;
            for(size_t index = 0; index < read; index++) {
                result = result && reference[index] == data[index];
            }

            EXPECT_TRUE(result);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Tests
} // Core
} // WPEFramework
