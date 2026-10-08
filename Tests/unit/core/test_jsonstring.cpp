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
#include <string>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

TEST(JSONString, NonAsciiCodePointWithLongRemainder)
{
    // ToCodePoint() takes a uint8_t length; the bytes remaining in the string
    // (including the multi-byte character itself) must not be truncated to it.
    // With exactly 256, 512, ... bytes remaining the character was serialized
    // from its first byte only (e.g. "\u0003" instead of "\u00E9").
    const struct {
        std::string character;
        std::string escaped;
    } characters[] = {
        { "\xC3\xA9", R"(\u00E9)" },               // U+00E9, 2 bytes
        { "\xE6\x97\xA5", R"(\u65E5)" },           // U+65E5, 3 bytes
        { "\xF0\x9F\x98\x80", R"(\uD83D\uDE00)" }  // U+1F600, 4 bytes (surrogate pair)
    };

    for (const auto& character : characters) {
        for (const uint32_t remaining : { 255u, 256u, 257u, 258u, 511u, 512u, 513u }) {
            const std::string tail(remaining - character.character.length(), 'x');
            const std::string value = character.character + tail;
            const std::string expected = "\"" + character.escaped + tail + "\"";

            ::Thunder::Core::JSON::String json;
            json = value;

            std::string textOut;
            EXPECT_TRUE(json.ToString(textOut));
            EXPECT_EQ(textOut, expected) << "character bytes: " << character.character.length() << ", remaining: " << remaining;

            ::Thunder::Core::OptionalType<::Thunder::Core::JSON::Error> result;
            ::Thunder::Core::JSON::String roundTrip;
            EXPECT_TRUE(roundTrip.FromString(textOut, result));
            EXPECT_FALSE(result.IsSet());
            EXPECT_EQ(roundTrip.Value(), value) << "character bytes: " << character.character.length() << ", remaining: " << remaining;
        }
    }
}

} } } // Thunder::Tests::Core
