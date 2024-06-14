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
#include <functional>
#include <sstream>

#include <gtest/gtest.h>
#include "core/core.h"

namespace Thunder {
namespace Tests {

    TEST(HEX2StrSerialization, serialiation_working) {
        uint8_t object[] = {0xB1, 0x32, 0x01};
        const uint16_t lenght = sizeof(object);

        string output;
        Core::ToHexString(object, lenght, output);

        EXPECT_EQ(output, "b13201");
    }

    TEST(HEX2StrSerialization, deserialization_lowercase) {
        string str = "3b6a9e";
        uint8_t expected[] = {0x3b, 0x6a, 0x9e};
        uint8_t buffer[8] = {0};

        string output;
        uint16_t length = Core::FromHexString(str, buffer, 8);

        EXPECT_EQ(length, 3);
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(HEX2StrSerialization, deserialization_uppercase) {
        string str = "3B6A9E";
        uint8_t expected[] = {0x3b, 0x6a, 0x9e};
        uint8_t buffer[8] = {0};

        string output;
        uint16_t length = Core::FromHexString(str, buffer, 8);

        EXPECT_EQ(length, 3);
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(HEX2StrSerialization, deserialization_uneven) {
        string str = "B6A9E";
        uint8_t expected[] = {0x0b, 0x6a, 0x9e};
        uint8_t buffer[8] = {0};

        string output;
        uint16_t length = Core::FromHexString(str, buffer, 8);

        EXPECT_EQ(length, 3);
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(HEX2StrSerialization, no_buffer_overflow) {
        string str = "010203040506";
        uint8_t expected[] = {0x01, 0x02, 0xDE, 0xAD, 0xBE, 0xEF};
        uint8_t buffer[8]  = {0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};

        string output;
        uint16_t length = Core::FromHexString(str, buffer, 2);

        EXPECT_EQ(length, 2);
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }
}
}
