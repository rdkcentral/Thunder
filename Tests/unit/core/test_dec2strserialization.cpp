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

#include <functional>
#include <sstream>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include "core/core.h"

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(Dec2StrSerialization, serialization) {
        uint8_t object[] = {0xC0, 0xA8, 0x01, 0xFF};

        string output;
        ::Thunder::Core::ToDecString(object, sizeof(object), output, '.');

        EXPECT_EQ(output, "192.168.1.255");
    }

    TEST(Dec2StrSerialization, serialization_long) {
        uint8_t object[] = {0xC0, 0xA8, 0x01, 0xFF, 0x01, 0xFF};

        string output;
        ::Thunder::Core::ToDecString(object, sizeof(object), output, '.');

        EXPECT_EQ(output, "192.168.1.255.1.255");
    }

    TEST(Dec2StrSerialization, deserialization) {
        string str = "192.168.1.255";
        uint8_t expected[] = {0xC0, 0xA8, 0x01, 0xFF};
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, sizeof(expected)));
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(Dec2StrSerialization, deserialization_long) {
        string str = "192.168.1.255.1.255";
        uint8_t expected[] = {0xC0, 0xA8, 0x01, 0xFF, 0x01, 0xFF};
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, sizeof(expected));
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(Dec2StrSerialization, deserialization_max_capacity) {
        string str = "192.168.1.255";
        uint8_t expected[] = {0xC0, 0xA8, 0x01, 0xFF};
        uint8_t buffer[4] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, sizeof(expected));
        EXPECT_EQ(memcmp(expected, buffer, sizeof(expected)), 0);
    }

    TEST(Dec2StrSerialization, deserialization_truncated) {
        string str = "192.168.1.255";
        uint8_t expected[] = {0xC0, 0xA8, 0x01, 0xFF};
        uint8_t buffer[2] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, sizeof(buffer)); // !!!
        EXPECT_EQ(memcmp(expected, buffer, sizeof(buffer)), 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_1) {
        string str = "192.168.1.";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_2) {
        string str = "192.168..255";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_3) {
        string str = ".192.168.1.255";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_4) {
        string str = "292.168.1.255";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_5 {
        string str = "192.168.1.255.";
        uint8_t buffer[4] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_6) {
        string str = "192.168.1.255.355.";
        uint8_t buffer[4] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_7) {
        string str = "292.168.1.355";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

    TEST(Dec2StrSerialization, deserialization_negative_8) {
        string str = "C0.168.1.255";
        uint8_t buffer[8] = {0};

        uint16_t length = ::Thunder::Core::FromDecString(str, buffer, sizeof(buffer), '.');

        EXPECT_EQ(length, 0);
    }

} // Core
} // Tests
} // Thunder
