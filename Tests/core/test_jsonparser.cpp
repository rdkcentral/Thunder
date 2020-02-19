/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "JSON.h"

#define QUIRKS_MODE

namespace WPEFramework {
namespace Tests {

    enum class JSONTestEnum {
        ONE,
        TWO
    };

    struct TestCaseBase {
        virtual ~TestCaseBase() = 0;
    };

    TestCaseBase::~TestCaseBase() {}

    template <typename T>
    class PrimitiveJson : public TestCaseBase, public Core::JSON::Container {
    public:
        static_assert(std::is_base_of<Core::JSON::IElement, T>::value, "You have to derive from Core::JSON::IElement");

        explicit PrimitiveJson()
            : Core::JSON::Container()
        {
        }

        void Init(const std::string& name)
        {
            Construct(name);
            InitValue(&_value, name);
        }

        ~PrimitiveJson() override {}

        PrimitiveJson(const PrimitiveJson&) = delete;
        PrimitiveJson& operator=(const PrimitiveJson&) = delete;

        const T& Value() const
        {
            return _value;
        }

        void Construct(const std::string& name)
        {
            Add(name.c_str(), &_value);
        }

    private:
        template <typename U>
        void InitValue(U* value, const std::string& name)
        {
        }

        template <typename U>
        void InitValue(PrimitiveJson<U>* value, const std::string& name)
        {
            value->Construct(name);
        }

        T _value;
    };

    template <typename T>
    void Execute(T& test, const std::string& testJSON, bool valid)
    {
        static_assert(std::is_base_of<TestCaseBase, T>::value, "This is to be run against TCs");
        Core::OptionalType<Core::JSON::Error> error;
        const bool result = test.FromString(testJSON, error);
        EXPECT_EQ(valid, result);
        EXPECT_NE(valid, error.IsSet());
    }

    struct TestData {
        std::string key;
        std::string keyToPutInJson;
        std::string keyValueSeparator = ":";
        std::string value;
        std::string valueToPutInJson;
    };

    template <typename T>
    void ExecutePrimitiveJsonTest(const TestData& data, bool valid, std::function<void(const T&)> verifyResult)
    {
        PrimitiveJson<T> test{};
        test.Init(data.key);
        std::stringstream testJSON;
        testJSON << "{";
        testJSON << data.keyToPutInJson + data.keyValueSeparator + data.valueToPutInJson;
        testJSON << "}";
        Execute<PrimitiveJson<T>>(test, testJSON.str(), valid);
        if (verifyResult)
            verifyResult(test.Value());
    }

    TEST(JSONParser, NoValue)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, NoValueNoSep)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.keyValueSeparator.clear();
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, NoKey)
    {
        TestData data;
        data.key = "[\"key\"]";
        data.keyToPutInJson = data.key;
        data.keyValueSeparator.clear();
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, KeyAndComma)
    {
        TestData data;
        data.key = "\"key\",";
        data.keyToPutInJson = data.key;
        data.keyValueSeparator.clear();
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, SpuriousCommaAtTheEnd)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\",";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, SpuriousColonAtTheEnd)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\":";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, SpuriousColonInKey)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\" :";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, DoubleComma)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\",,";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, Array)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "[\"foo\", \"bar\"]";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, true, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                ASSERT_EQ(2u, v.Length());
                EXPECT_NE(string{}, v[0].Value());
                EXPECT_NE(string{}, v[1].Value());
            });
    }

    TEST(JSONParser, NullArray)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "null";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, true, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_TRUE(v.IsNull());
            });
    }

    TEST(JSONParser, IntendedNullArrayButMissed)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "nulk";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, false, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_EQ(0u, v.Length());
            });
    }

    TEST(JSONParser, ArrayWithCommaOnly)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "[,]";
        data.valueToPutInJson = "[,]";
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, false, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_EQ(0u, v.Length());
            });
    }

    TEST(JSONParser, WronglyOpenedArray)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.valueToPutInJson = "(\"Foo\"]";
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, false, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_EQ(0u, v.Length());
            });
    }

    TEST(JSONParser, WronglyClosedArray1)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.valueToPutInJson = "[\"Foo\"}";
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, false, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_EQ(0u, v.Length());
            });
    }

    TEST(JSONParser, WronglyClosedArray2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.valueToPutInJson = "[\"Foo\")";
        ExecutePrimitiveJsonTest<Core::JSON::ArrayType<Core::JSON::String>>(
            data, false, [](const Core::JSON::ArrayType<Core::JSON::String>& v) {
                EXPECT_EQ(0u, v.Length());
            });
    }

    TEST(JSONParser, String)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, NullString)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "null";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_TRUE(v.IsNull());
        });
    }

    TEST(JSONParser, String_JsonObjectArrayCharIn_1)
    {
        TestData data;
        data.key = "key[";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, String_JsonObjectArrayCharIn_2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value}";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, String_JsonObjectArrayCharIn_3)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = ":]";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringNoQuotes)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = data.value;
        const bool expected =
#ifdef QUIRKS_MODE
            true
#else
            false
#endif
            ;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, expected, [&data](const Core::JSON::String& v) {
#ifdef QUIRKS_MODE
            EXPECT_EQ(data.value, v.Value());
#else
            EXPECT_EQ(string{}, v.Value());
#endif
        });
    }

    TEST(JSONParser, StringLeftQuoteOnly)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, StringRightQuoteOnly)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = data.value + "\"";
#ifndef QUIRKS_MODE
        const bool expected = false;
#else
        const bool expected = true;
#endif
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, expected, [&data](const Core::JSON::String& v) {
#ifndef QUIRKS_MODE
            EXPECT_EQ(string{}, v.Value());
#else
            EXPECT_EQ(data.value + "\"", v.Value());
#endif
        });
    }

    TEST(JSONParser, KeyNoQuotes)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = data.key;
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, KeyLeftQuoteOnly)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key;
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, KeyRightQuoteOnly)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, PositiveNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "123";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::DecUInt8>(data, true, [](const Core::JSON::DecUInt8& v) {
            EXPECT_EQ(123u, v.Value());
        });
    }

    TEST(JSONParser, NullNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "null";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::DecUInt8>(data, true, [](const Core::JSON::DecUInt8& v) {
            EXPECT_TRUE(v.IsNull());
        });
    }

    TEST(JSONParser, NegativeNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "-123";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::DecSInt8>(data, true, [](const Core::JSON::DecSInt8& v) {
            EXPECT_EQ(-123, v.Value());
        });
    }

    TEST(JSONParser, PositiveNumberWithSign)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "+123";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::DecSInt8>(data, false, [](const Core::JSON::DecSInt8& v) {
            EXPECT_NE(123, v.Value());
        });
    }

    TEST(JSONParser, InvalidNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "123g";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::DecUInt8>(data, false, [](const Core::JSON::DecUInt8& v) {
            EXPECT_EQ(0u, v.Value());
        });
    }

    TEST(JSONParser, PositiveHexNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "0X7B";
        data.valueToPutInJson = "\"" + data.value + "\"";;
        ExecutePrimitiveJsonTest<Core::JSON::HexUInt8>(data, true, [](const Core::JSON::HexUInt8& v) {
            EXPECT_EQ(123u, v.Value());
        });
    }

    TEST(JSONParser, NegativeHexNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "-0X7B";
        data.valueToPutInJson = "\"" + data.value + "\"";;
        ExecutePrimitiveJsonTest<Core::JSON::HexSInt8>(data, true, [](const Core::JSON::HexSInt8& v) {
            EXPECT_EQ(-123, v.Value());
        });
    }

    TEST(JSONParser, PositiveOctNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "0173";
        data.valueToPutInJson = "\"" + data.value + "\"";;
        ExecutePrimitiveJsonTest<Core::JSON::OctUInt8>(data, true, [](const Core::JSON::OctUInt8& v) {
            EXPECT_EQ(123u, v.Value());
        });
    }

    TEST(JSONParser, NegativeOctNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "-0173";
        data.valueToPutInJson = "\"" + data.value + "\"";;
        ExecutePrimitiveJsonTest<Core::JSON::OctSInt8>(data, true, [](const Core::JSON::OctSInt8& v) {
            EXPECT_EQ(-123, v.Value());
        });
    }

    // FIXME: Disabled because JSON Parser does not support exponential notation
    // yet is supports hex so 'E' or 'e' in number is interpreted as hex.
    TEST(DISABLED_JSONParser, ExponentialNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "1e2";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::DecUInt8>(data, false, [](const Core::JSON::DecUInt8& v) {
#ifndef QUIRKS_MODE
            EXPECT_EQ(100u, v.Value());
#else
            EXPECT_EQ(0x1E2, v.Value());
#endif
        });
    }

    // FIXME: Parser does not support floating points.

    TEST(JSONParser, StringWithEscapeChars)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        const char rawUnescaped[] = { 'v', 'a', 'l', 'u', 'e', ' ', '"', '\b', '\n', '\f', '\r', '\\', 'u', '0', '0', 'b', '1', '/', '\\', '\0' };
        data.value = "value \\\"\\b\\n\\f\\r\\u00b1\\/\\\\";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [rawUnescaped](const Core::JSON::String& v) {
            EXPECT_TRUE(memcmp(rawUnescaped, v.Value().c_str(), sizeof(rawUnescaped)) == 0);
        });
    }

    TEST(JSONParser, StringWithInvalidEscapeChars1)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value\\z";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    // FIXME: Parser does not parse unicode codepoints.
    TEST(DISABLED_JSONParser, StringWithInvalidEscapeChars2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value\\uZZZZ";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, Object)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value";
        data.valueToPutInJson = "{\"" + data.key + "\":\"" + data.value + "\"}";
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, true, [&data](const PrimitiveJson<Core::JSON::String>& v) {
            EXPECT_TRUE(v.HasLabel(data.key));
            EXPECT_EQ(data.value, v.Value().Value());
        });
    }

    TEST(JSONParser, EmptyObject)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.valueToPutInJson = "{\"" + data.key + "\":\"" + data.value + "\"}";
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, true, [&data](const PrimitiveJson<Core::JSON::String>& v) {
            EXPECT_TRUE(v.HasLabel(data.key));
            EXPECT_EQ(string{}, v.Value().Value());
        });
    }

    TEST(JSONParser, NullObject)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "null";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, true, [](const PrimitiveJson<Core::JSON::String>& v) {
            EXPECT_TRUE(v.IsNull());
        });
    }

    TEST(JSONParser, IntentedNullObjectButMissed)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "nill";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, false, nullptr);
    }

    TEST(JSONParser, Unmatched1)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\"" + data.key + "\":\"value\"";
        data.valueToPutInJson = "[" + data.value + "}";
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, false, nullptr);
    }

    TEST(JSONParser, Unmatched2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\"" + data.key + "\":\"value\"";
        data.valueToPutInJson = "{" + data.value + "]";
        ExecutePrimitiveJsonTest<PrimitiveJson<Core::JSON::String>>(data, false, nullptr);
    }

    // Extensions:
    TEST(JSONParser, BufferNotNullNotBase64)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "nill";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::Buffer>(data, false, nullptr);
    }

#ifdef QUIRKS_MODE
    TEST(JSONParser, OpaqueObject)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "{\"" + data.key + "\":{\"key2\":[\"value\"]}}";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, DeeplyNestedOpaqueObject)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "{\"" + data.key + "\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":"
                                        "{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":"
                                        "{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":{\"key2\":"
                                        "{\"key2\":{\"key2\":{\"key2\":{\"key2\":\"value\"}}}}}}}}}}}}}}}}}}}}}}}}}";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, nullptr);
    }

    TEST(JSONParser, MalformedOpaqueObject1)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "{\"" + data.key + "\":{\"key2\":[\"value\"]";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }

    TEST(JSONParser, MalformedOpaqueObject2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "{\"" + data.key + "\":{\"key2\":[\"value\"}}}";
        data.valueToPutInJson = data.value;
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
        });
    }
#endif

    TEST(JSONParser, EnumValue)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "two";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::EnumType<JSONTestEnum>>(data, true, [&data](const Core::JSON::EnumType<JSONTestEnum>& v) {
            EXPECT_EQ(JSONTestEnum::TWO, v.Value());
        });
    }

    TEST(JSONParser, InvalidEnumValue)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "three";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::EnumType<JSONTestEnum>>(data, false, nullptr);
    }

} // Tests

ENUM_CONVERSION_BEGIN(Tests::JSONTestEnum){ WPEFramework::Tests::JSONTestEnum::ONE, _TXT("one") },
    { WPEFramework::Tests::JSONTestEnum::TWO, _TXT("two") },
    ENUM_CONVERSION_END(Tests::JSONTestEnum)

} // WPEFramework

#undef QUIRKS_MODE
