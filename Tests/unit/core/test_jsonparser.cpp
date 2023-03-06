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

#include "../IPTestAdministrator.h"
#include <core/core.h>
#include "JSON.h"

namespace WPEFramework {
    enum class JSONTestEnum {
        ENUM_1,
        ENUM_2,
        ENUM_3,
        ENUM_4
    };

    ENUM_CONVERSION_BEGIN(JSONTestEnum)
        { JSONTestEnum::ENUM_1, _TXT("enum_1") },
        { JSONTestEnum::ENUM_2, _TXT("enum_2") },
        { JSONTestEnum::ENUM_3, _TXT("enum_3") },
        { JSONTestEnum::ENUM_4, _TXT("enum_4") },
    ENUM_CONVERSION_END(JSONTestEnum)

namespace Tests {

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
            EXPECT_EQ(2u, v.Length());
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
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }
    
    TEST(JSONParser, GreekString)
    {
        TestData data;
        data.key = "λόγους";
        data.keyToPutInJson = "\"" + data.key + "\"";
        printf("     input key :   %zd --- = %s \n", data.key.length(), data.key.c_str());
        data.value = "Φίλιππον ὁρῶ";
        printf("     input value :   %zd --- = %s \n", data.value.length(), data.value.c_str());
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            printf("     output value :   %zd --- = %s \n", v.Value().length(), v.Value().c_str());
            EXPECT_EQ(data.value, v.Value());
        });
    }


    TEST(JSONParser, GeorgianString)
    {
        TestData data;
        data.key = "კონფერენციაზე";
        printf("     input key :   %zd --- = %s \n", data.key.length(), data.key.c_str());
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "კონფერენცია შეჰკრებს";
        printf("     input value :   %zd --- = %s \n", data.value.length(), data.value.c_str());
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            printf("     output value :   %zd --- = %s \n", v.Value().length(), v.Value().c_str());
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, RussianString)
    {
        TestData data;
        data.key = "Зарегистрируйтесь";
        printf("     input key :   %zd --- = %s \n", data.key.length(), data.key.c_str());
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Десятую Международную";
        printf("     input value :   %zd --- = %s \n", data.value.length(), data.value.c_str());
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            printf("     output value :   %zd --- = %s \n", v.Value().length(), v.Value().c_str());
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, FrenchString)
    {
        TestData data;
        data.key = "suis ravi de";
        printf("     input key :   %zd --- = %s \n", data.key.length(), data.key.c_str());
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "suis heureuse";
        printf("     input value :   %zd --- = %s \n", data.value.length(), data.value.c_str());
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            printf("     output value :   %zd --- = %s \n", v.Value().length(), v.Value().c_str());
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, ThaiString)
    {
        TestData data;
        data.key = "ต้องรบราฆ่าฟันจนบรรลัย";
        printf("     input key :   %zd --- = %s \n", data.key.length(), data.key.c_str());
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "ใช้สาวนั้นเป็นชนวนชื่นชวนใจ";
        printf("     input value :   %zd --- = %s \n", data.value.length(), data.value.c_str());
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            printf("     output value :   %zd --- = %s \n", v.Value().length(), v.Value().c_str());
            EXPECT_EQ(data.value, v.Value());
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

        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [](const Core::JSON::String& v) {
            EXPECT_EQ(string{}, v.Value());
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
    TEST(JSONParser, ExponentialNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "1e2";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::DecUInt8>(data, false, [](const Core::JSON::DecUInt8& v) {
            EXPECT_EQ(100u, v.Value());
        });
    }

    TEST(JSONParser, FloatingPointNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "1.34f";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            std::string res = value.str();
            res += 'f';
            EXPECT_EQ(data.value, res.c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_1_decimal_1_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "1.3";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_1_decimal_2_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "1.35";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_1_decimal_3_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "2.349";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_2_decimal_1_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "48.3";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_2_decimal_2_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "48.39";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_2_decimal_3_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "48.398";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_3_decimal_1_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "489.3";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_3_decimal_2_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "489.38";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, FloatingPointNumberWith_3_decimal_3_floatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "489.389";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Float>(data, true, [data](const Core::JSON::Float& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumber)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "6.61914e+6";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_1_Decimal_1_FloatingPoint)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "3.5";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_1_Decimal_2_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "3.56";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_1_Decimal_3_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "3.567";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_2_Decimal_1_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "32.5";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_2_Decimal_2_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "32.59";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_2_Decimal_3_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "32.598";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_3_Decimal_1_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "326.5";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_3_Decimal_2_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "326.56";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, DoubleNumberWith_3_Decimal_3_FloatingPoints)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "326.545";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::Double>(data, true, [data](const Core::JSON::Double& v) {
            std::ostringstream value;
            value << v.Value();
            EXPECT_EQ(data.value, value.str().c_str());
        });
    }

    TEST(JSONParser, StringWithEscapeSequence)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\n solution \n for \n string \n serialization\n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceSinglequote)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\' solution \n for \n string \n serialization\'\n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceDoublequote)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\" solution \n for \n string \n serialization\"\n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceQuestionmark)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Is this a solution \?";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceBackslash)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking backslash \\";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceAudiblebell)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking audible bell \a";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceBackspace)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking backspace \b";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceFormfeed)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking form feed \f";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceLinefeed)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking line feed \n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeCarriagereturn)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking carriage return \r";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeHorizontaltab)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking horizontal tab \t";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeSequenceVerticaltab)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "Checking vertical tab \v";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEscapeChars)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = R"(value\\uZZZZ)";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            Core::JSON::String value;
            value.FromString(data.valueToPutInJson);

            EXPECT_EQ(value.Value(), v.Value());
        });
    }

    TEST(JSONParser, StringWithEmbeddedNewLines)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\n solution \n for \n string \n serialization\n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEmbeddedTab)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\t solution \t for \t string \t serialization\t";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
        });
    }

    TEST(JSONParser, StringWithEmbeddedCarriageReturn)
    {
        TestData data;
        data.key = "teststring";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "\n solution \t for \r string \t serialization\n";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, [&data](const Core::JSON::String& v) {
            EXPECT_EQ(data.value, v.Value());
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

    TEST(JSONParser, StringWithInvalidEscapeChars2)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "value \"\b\n\f\r\u00b1/\"";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, false, [] (const Core::JSON::String& v) {
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
        ExecutePrimitiveJsonTest<Core::JSON::String>(data, true, nullptr);
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

    TEST(JSONParser, EnumValue)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "enum_2";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::EnumType<JSONTestEnum>>(data, true, [&data](const Core::JSON::EnumType<JSONTestEnum>& v) {
            EXPECT_EQ(JSONTestEnum::ENUM_2, v.Value());
        });
    }

    TEST(JSONParser, InvalidEnumValue)
    {
        TestData data;
        data.key = "key";
        data.keyToPutInJson = "\"" + data.key + "\"";
        data.value = "enum_5";
        data.valueToPutInJson = "\"" + data.value + "\"";
        ExecutePrimitiveJsonTest<Core::JSON::EnumType<JSONTestEnum>>(data, false, nullptr);
    }

    TEST(JSONParser, Variant)
    {
        WPEFramework::Core::JSON::Variant variant;
        WPEFramework::Core::JSON::Variant variant1(std::numeric_limits<int32_t>::min());

        WPEFramework::Core::JSON::Variant variant2(std::numeric_limits<int64_t>::min());
        WPEFramework::Core::JSON::Variant variant3(std::numeric_limits<uint32_t>::min());
        WPEFramework::Core::JSON::Variant variant4(std::numeric_limits<uint64_t>::min());
        WPEFramework::Core::JSON::Variant variant5(true);

        //EXPECT_EQ(variant1.Number(), 0); //TODO
        EXPECT_EQ(variant2.Number(), 0);
        EXPECT_EQ(variant3.Number(), 0);
        EXPECT_EQ(variant4.Number(), 0);

        const TCHAR text[] = "varient";
        WPEFramework::Core::JSON::Variant variant6(text);
        EXPECT_STREQ(variant6.String().c_str(), text);

        WPEFramework::Core::JSON::Variant variant6_new = text;
        EXPECT_STREQ(variant6_new.String().c_str(), text);

        std::string msg = "varient";
        WPEFramework::Core::JSON::Variant variant7(msg);
        EXPECT_STREQ(variant7.String().c_str(), msg.c_str());

        WPEFramework::Core::JSON::Variant variant8(variant4);

        WPEFramework::Core::JSON::VariantContainer container;
        WPEFramework::Core::JSON::Variant val1(10);
        WPEFramework::Core::JSON::Variant val2(20);
        WPEFramework::Core::JSON::Variant val3(30);
        container.Set("key1", val1);
        container.Set("key2", val2);
        container.Set("key3", val3);
        WPEFramework::Core::JSON::Variant variant9(container);
        msg = "name=key1 type=Object value={\n    name=key1 type=Number value=10\n    name=key2 type=Number value=20\n    name=key3 type=Number value=30\n}\n";
        EXPECT_STREQ(variant9.GetDebugString("key1").c_str(), msg.c_str());

        variant2.Boolean(true);
        WPEFramework::Core::JSON::Variant variant10 = variant4;

        variant5.Content();
        EXPECT_TRUE(variant5.Boolean());
        EXPECT_FALSE(variant6.Boolean());

        EXPECT_STREQ(variant6.String().c_str(),"varient");

        variant1.Number(std::numeric_limits<uint32_t>::min());
        variant6.String("Updated");
        EXPECT_STREQ(variant6.String().c_str(),"Updated");

        variant6.Boolean(true);

        WPEFramework::Core::JSON::Variant variant11;
        variant11.Object(container);
        msg = "name=key1 type=Object value={\n    name=key1 type=Number value=10\n    name=key2 type=Number value=20\n    name=key3 type=Number value=30\n}\n";
        EXPECT_STREQ(variant11.GetDebugString("key1").c_str(), msg.c_str());

        WPEFramework::Core::JSON::VariantContainer variantContainer = variant11.Object();
        EXPECT_EQ(variantContainer.Get("key1").String(), "10");
    }

    TEST(JSONParser, VariantContainer)
    {
        WPEFramework::Core::JSON::VariantContainer container;

        WPEFramework::Core::JSON::Variant val1(10);
        WPEFramework::Core::JSON::Variant val2(20);
        WPEFramework::Core::JSON::Variant val3(30);
        WPEFramework::Core::JSON::Variant val4(40);
        WPEFramework::Core::JSON::Variant val5(50);
        WPEFramework::Core::JSON::Variant val6(60);

        container.Set("key1", val1);
        container.Set("key2", val2);
        container.Set("key3", val3);
        container.Set("key4", val4);
        container.Set("key5", val5);
        container.Set("key6", val6);

        EXPECT_EQ(val1.Number(), 10);
        EXPECT_EQ((container.Get("key1")).String(), "10");
        EXPECT_EQ((container["key5"]).String(), "50");

        WPEFramework::Core::JSON::Variant variant1 = container["key5"];
        const WPEFramework::Core::JSON::Variant variant2 =  container["key5"];

        EXPECT_EQ(variant1.String(), "50");
        EXPECT_EQ(variant2.String(), "50");

        std::string serialize = "{\"key1\":\"hello\"}";
        WPEFramework::Core::JSON::VariantContainer container1(serialize);
        std::string text;
        container1.ToString(text);
        EXPECT_STREQ(text.c_str(),serialize.c_str());

        serialize = "\"key1\":\"hello\""; //Trigger a call to ErrorDisplayMessage() with purposefully created error condition.
        WPEFramework::Core::JSON::VariantContainer errorContainer(serialize);
        errorContainer.ToString(text);

        const TCHAR serialized[] = "{\"key2\":\"checking\"}";
        WPEFramework::Core::JSON::VariantContainer container2(serialized);
        container2.ToString(text);
        EXPECT_STREQ(text.c_str(), serialized);

        WPEFramework::Core::JSON::VariantContainer container_new(container2);
        WPEFramework::Core::JSON::VariantContainer container_copy = container_new;

        std::string debugString = "            name=key1 type=Number value=10\n            name=key2 type=Number value=20\n            name=key3 type=Number value=30\n            name=key4 type=Number value=40\n            name=key5 type=Number value=50\n            name=key6 type=Number value=60\n";
        EXPECT_STREQ(container.GetDebugString(3).c_str(), debugString.c_str());
    }

    TEST(JSONParser, VariantDebugStringNumber)
    {
        WPEFramework::Core::JSON::Variant variant(10);
        std::string debugString = "            [0] name=hello type=Number value=10\n";
        EXPECT_STREQ(variant.GetDebugString("hello",3,0).c_str(), debugString.c_str());

        WPEFramework::Core::JSON::Variant variant1 = std::numeric_limits<uint32_t>::min();
        EXPECT_EQ(variant1.Number(), 0);

        WPEFramework::Core::JSON::Variant variant2 = 10;
        EXPECT_EQ(variant2.Number(), 10);
    }

    TEST(JSONParser, VariantDebugStringEmpty)
    {
        WPEFramework::Core::JSON::Variant variant;

        std::string debugString = "    [0] name=hello type=Empty value=null\n";
        EXPECT_STREQ(variant.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());
    }

    TEST(JSONParser, VariantDebugStringBoolean)
    {
        WPEFramework::Core::JSON::Variant variant(true);

        std::string debugString =  "    [0] name=hello type=Boolean value=true\n";
        EXPECT_STREQ(variant.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());

        WPEFramework::Core::JSON::Variant variant1 = true;
        EXPECT_STREQ(variant1.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());
    }

    TEST(JSONParser, VariantDebugStringString)
    {
        WPEFramework::Core::JSON::Variant variant("Variant");

        std::string debugString = "    [0] name=hello type=String value=Variant\n";
        EXPECT_STREQ(variant.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());

        WPEFramework::Core::JSON::Variant variant1 = "Variant";
        EXPECT_STREQ(variant1.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());
    }

    TEST(JSONParser, VariantDebugStringArray)
    {
        WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::Variant> array;
        array.Add(WPEFramework::Core::JSON::Variant(10));
        WPEFramework::Core::JSON::Variant variant(array);

        std::string debugString = "    [0] name=hello type=String value=[10]\n";
        EXPECT_STREQ(variant.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());
        WPEFramework::Core::JSON::Variant variant1 = WPEFramework::Core::JSON::Variant(array);
        EXPECT_STREQ(variant1.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());

        WPEFramework::Core::JSON::Variant variant2;
        variant2.Array(array);

        WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::Variant> result;
        result = variant2.Array();
        WPEFramework::Core::JSON::Variant variant3(result);

        EXPECT_STREQ(variant3.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());;
    }

    TEST(JSONParser, VariantDebugStringObject)
    {
        WPEFramework::Core::JSON::VariantContainer container;
        WPEFramework::Core::JSON::Variant val1(10);
        WPEFramework::Core::JSON::Variant val2(20);
        WPEFramework::Core::JSON::Variant val3(30);

        container.Set("key1", val1);
        container.Set("key2", val2);
        container.Set("key3", val3);
        WPEFramework::Core::JSON::Variant variant(container);

        std::string debugString = "    [0] name=hello type=Object value={\n        name=key1 type=Number value=10\n        name=key2 type=Number value=20\n        name=key3 type=Number value=30\n   }\n";
        EXPECT_STREQ(variant.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());

        WPEFramework::Core::JSON::Variant variant1 = container;
        EXPECT_STREQ(variant1.GetDebugString("hello", 1, 0).c_str(), debugString.c_str());
    }

    TEST(JSONParser, VariantContainerWithElements)
    {
        std::list<std::pair<string, WPEFramework::Core::JSON::Variant>> elements;

        WPEFramework::Core::JSON::Variant val1(10);
        WPEFramework::Core::JSON::Variant val2(20);
        WPEFramework::Core::JSON::Variant val3(30);

        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key1", val1));
        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key2", val2));
        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key3", val3));

        WPEFramework::Core::JSON::VariantContainer container(elements);

        WPEFramework::Core::JSON::VariantContainer::Iterator it = container.Variants();
        EXPECT_TRUE(it.Next());
        EXPECT_TRUE(container.HasLabel("Key1"));
        EXPECT_TRUE(it.IsValid());
    }

    TEST(JSONParser, VariantContainerIterator)
    {
        std::list<std::pair<string, WPEFramework::Core::JSON::Variant>> elements;

        WPEFramework::Core::JSON::Variant val1(10);
        WPEFramework::Core::JSON::Variant val2(20);
        WPEFramework::Core::JSON::Variant val3(30);

        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key1", val1));
        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key2", val2));
        elements.push_back(std::pair<std::string, WPEFramework::Core::JSON::Variant>("Key3", val3));

        WPEFramework::Core::JSON::VariantContainer::Iterator iterator;
        WPEFramework::Core::JSON::VariantContainer::Iterator it(elements);
        WPEFramework::Core::JSON::VariantContainer::Iterator itCopy(iterator);
        WPEFramework::Core::JSON::VariantContainer::Iterator iteratorCopy = itCopy;

        EXPECT_TRUE(it.Next());
        EXPECT_TRUE(it.IsValid());
        EXPECT_STREQ(it.Label(),"Key1");
        EXPECT_STREQ(it.Current().String().c_str(),"10");

        it.Reset();
        EXPECT_FALSE(it.IsValid());
    }

    TEST(JSONParser, simpleSet)
    {
        {
            // UTF8 for the violin music key: (byte array) f0 9d 84 9e => CodePoint 0x1D11E => UTF16 0xD834 0xDD1E 
            string input = R"("Violin key sending: \uD834\uDD1E")";
            Core::JSON::String json;
            string received;

            json.FromString(input);
            json.ToString(received);
            EXPECT_STREQ(input.c_str(), received.c_str());
        }
        {
            string input = R"("Control: [25][\u0019] Character: [28][\u001C]")";
            Core::JSON::String json;
            string received;

            json.FromString(input);
            json.ToString(received);
            EXPECT_STREQ(input.c_str(), received.c_str());
        }
        {
            Core::JSONRPC::Message message;
            string input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.connect","params":{"ssid":"iPhone\\"}})";
            string received;

            message.FromString(input);
            message.ToString(received);
            EXPECT_STREQ(input.c_str(), received.c_str());
        }
        {
            //
            // \u03BB = UTF16 = > UTF8(in string) = > 0xce 0xbb
            // \u1F79 = UTF16 = > UTF8(in string) = > 0xe1 0xbd 0xb9
            // \u03B3 = UTF16 = > UTF8(in string) = > 0xce 0xb3
            // \u03BF = UTF16 = > UTF8(in string) = > 0xce 0xbf
            // \u03C5 = UTF16 = > UTF8(in string) = > 0xcf 0x85
            // \u03C2 = UTF16 = > UTF8(in string) = > 0xcf 0x82
            // It should show it as an UTF8 string show : "aλόγουςb" but the string,
            // should have the hex bytes as shown above.
            string input = R"("a\u03BB\u1F79\u03B3\u03BF\u03C5\u03C2b")";
            Core::JSON::String json;
            string received;

            json.FromString(input);
            json.ToString(received);
            EXPECT_STREQ(input.c_str(), received.c_str());
        }
        {
            string input = R"("Wrong code: \uD4\u1E")";
            Core::JSON::String json;
            string received;

            json.FromString(input);
            json.ToString(received);
            EXPECT_EQ(string{}, received.c_str());
        }
    }

    class SmallTest : public WPEFramework::Core::JSON::Container {
    public:
        SmallTest(const SmallTest&) = delete;
        SmallTest& operator=(const SmallTest&) = delete;

        SmallTest()
            : Core::JSON::Container()
            , A(0)
            , B(0)
            , C()
            , D(false)
            , E(JSONTestEnum::ENUM_4) {
            Add(_T("A"), &A);
            Add(_T("B"), &B);
            Add(_T("C"), &C);
            Add(_T("D"), &D);
            Add(_T("E"), &E);
        }
        ~SmallTest() override = default;

    public:
        void Clear()
        {
            Core::JSON::Container::Clear();
        }

        Core::JSON::DecUInt32  A;
        Core::JSON::Float B;
        Core::JSON::String C;
        Core::JSON::Boolean D;
        Core::JSON::EnumType<JSONTestEnum> E;
    };

    class SmallTest2 : public WPEFramework::Core::JSON::Container {
    public:
        SmallTest2(const SmallTest2&) = delete;
        SmallTest2& operator=(const SmallTest2&) = delete;

        SmallTest2()
            : Core::JSON::Container()
            , A()
            , B(0.0)
            , C(0.0) {
            Add(_T("A"), &A);
            Add(_T("B"), &B);
            Add(_T("C"), &C);
        }
        ~SmallTest2() override = default;

    public:
        void Clear()
        {
            Core::JSON::Container::Clear();
        }

        Core::JSON::ArrayType<Core::JSON::String>  A;
        Core::JSON::Float B;
        Core::JSON::Double C;
    };

    class StringContainer : public Core::JSON::Container {
    public:
        StringContainer(const StringContainer&) = delete;
        StringContainer& operator=(const StringContainer&) = delete;

        StringContainer()
            : Name(_T("")) {
            Add(_T("name"), &Name);
        }
        ~StringContainer() override = default;

    public:
        Core::JSON::String Name;
    };

    class ParamsInfo : public Core::JSON::Container {
    public:
        ParamsInfo()
            : Core::JSON::Container() {
            Add(_T("ssid"), &Ssid);
        }
        ~ParamsInfo() override = default;

        ParamsInfo(const ParamsInfo&) = delete;
        ParamsInfo& operator=(const ParamsInfo&) = delete;

    public:
        Core::JSON::String Ssid; // Identifier of a network
    };

    class CommandParameters : public WPEFramework::Core::JSON::Container {
    public:
        CommandParameters(const CommandParameters&) = delete;
        CommandParameters& operator=(const CommandParameters&) = delete;

        CommandParameters()
            : Core::JSON::Container()
            , G(00)
            , H(0)
            , I()
            , J()
            , K(1)
            , L(0.0) {
            Add(_T("g"), &G);
            Add(_T("h"), &H);
            Add(_T("i"), &I);
            Add(_T("j"), &J);
            Add(_T("k"), &K);
            Add(_T("l"), &L);
        }

        ~CommandParameters() override = default;

    public:
        WPEFramework::Core::JSON::OctSInt16 G;
        WPEFramework::Core::JSON::DecSInt16 H;
        WPEFramework::Core::JSON::EnumType<JSONTestEnum> I;
        WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::DecUInt16> J;
        WPEFramework::Core::JSON::Float K;
        WPEFramework::Core::JSON::Double L;
    };

    class CommandRequest : public WPEFramework::Core::JSON::Container {
    public:
        CommandRequest(const CommandRequest&) = delete;
        CommandRequest& operator=(const CommandRequest&) = delete;

    public:
        CommandRequest()
            : Core::JSON::Container()
            , A(0x0)
            , B()
            , C(0x0)
            , D(false)
            , E(00)
            , F()
            , M()
            , N(0.0)
            , O(0.0)
        {
            Add(_T("a"), &A);
            Add(_T("b"), &B);
            Add(_T("c"), &C);
            Add(_T("d"), &D);
            Add(_T("e"), &E);
            Add(_T("f"), &F);
            Add(_T("m"), &M);
            Add(_T("n"), &N);
            Add(_T("o"), &O);
        }

        ~CommandRequest() override = default;

        void Clear()
        {
            WPEFramework::Core::JSON::Container::Clear();
        }

    public:
        WPEFramework::Core::JSON::HexSInt32 A;
        WPEFramework::Core::JSON::String B;
        WPEFramework::Core::JSON::HexUInt32 C;
        WPEFramework::Core::JSON::Boolean D;
        WPEFramework::Core::JSON::OctUInt16 E;
        CommandParameters F;
        WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::String> M;
        WPEFramework::Core::JSON::Float N;
        WPEFramework::Core::JSON::Double O;
    };

    TEST(JSONParser, smallParser)
    {
        {
            // Boolean test
            string input = R"({"D":true})";
            string translated = R"({"D":true})";

            Core::JSON::Tester<1, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // Boolean test
            string input = R"({"D":false})";
            string translated = R"({"D":false})";

            Core::JSON::Tester<1, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // null test
            string input = R"({"A":null,"B":null,"C":null,"D":null,"E":null})";
            string translated = R"({"A":null,"B":null,"C":null,"D":null,"E":null})";

            Core::JSON::Tester<1, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // SmallTest
            string input = R"({"A":"1","B":"3.2","C":"Text"})";
            string translated = R"({"A":1,"B":3.2,"C":"Text"})";

            Core::JSON::Tester<1, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // SmallTest
            string input = R"({"A":["test"],"B":"3.2","C":"-65.22"})";
            string translated = R"({"A":["test"],"B":3.2,"C":-65.22})";

            Core::JSON::Tester<1, SmallTest2> parser;
            Core::ProxyType<SmallTest2> output = Core::ProxyType<SmallTest2>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"],"k":"1.1","l":"2.11"},"m":["Test"],"n":"3.2","o":"-65.22"})";
            string translated = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22],"k":1.1,"l":2.11},"m":["Test"],"n":3.2,"o":-65.22})";

            Core::JSON::Tester<1, CommandRequest> parser;
            Core::ProxyType<CommandRequest> output = Core::ProxyType<CommandRequest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            //Tester
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"],"k":"1.1","l":"2.11"},"m":["Test"],"n":"3.2","o":"-65.22"})";
            string inputRequired = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22],"k":1.1,"l":2.11},"m":["Test"],"n":3.2,"o":-65.22})";
            string output;
            WPEFramework::Core::ProxyType<CommandRequest> command = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            command->A = -90;
            command->B = _T("TestIdentifier");
            command->C = 90;
            command->D = true;
            command->E = 12;
            command->F.G = -12;
            command->F.H = -44;
            command->F.I = JSONTestEnum::ENUM_4;
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(6, true));
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(14, true));
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(22, true));
            command->F.K = static_cast<float>(1.1);
            command->F.L = 2.11;
            command->N = static_cast<float>(3.2);
            command->O = -65.22;


            WPEFramework::Core::JSON::String str;
            str = string("Test");
            command->M.Add(str);
            WPEFramework::Core::JSON::Tester<1, CommandRequest> parser;
            //ToString
            parser.ToString(command, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());
            //FromString
            WPEFramework::Core::ProxyType<CommandRequest> received = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            parser.FromString(input, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());

            parser.FromString(inputRequired, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());

            //ArrayType Iterator
            WPEFramework::Core::JSON::ArrayType<Core::JSON::DecUInt16>::Iterator settings(command->F.J.Elements());
            for(int i = 0; settings.Next(); i++)
                EXPECT_EQ(settings.Current().Value(), command->F.J[i]);
            //null test
            input = R"({"a":null,"b":null,"c":"0x5A","d":null,"f":{"g":"-014","h":-44}})";
            parser.FromString(input, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(input.c_str(), output.c_str());
        }
        //JsonObject and JsonValue
        {
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22]}})";
            JsonObject command;
            command.FromString(input);
            string output;
            command.ToString(output);
            EXPECT_STREQ(input.c_str(), output.c_str());

            JsonObject object;
            object["g"] = "-014";
            object["h"] = -44;
            object["i"] = "enum_4";
            JsonArray arrayValue;
            arrayValue.Add(6);
            arrayValue.Add(14);
            arrayValue.Add(22);
            object["j"] = arrayValue;
            JsonObject demoObject;
            demoObject["a"] = "-0x5A";
            demoObject["b"] = "TestIdentifier";
            demoObject["c"] = "0x5A";
            demoObject["d"] = true;
            demoObject["e"] = "014";
            demoObject["f"] = object;
            string serialized;
            demoObject.ToString(serialized);
            EXPECT_STREQ(input.c_str(), serialized.c_str());

            JsonObject::Iterator index = demoObject.Variants();
            while (index.Next()) {
                JsonValue value(demoObject.Get(index.Label()));
                EXPECT_EQ(value.Content(), index.Current().Content());
                EXPECT_STREQ(value.Value().c_str(), index.Current().Value().c_str());
            }
        }
        //JsonObject Serialization and Deserialization with escape sequences
        {
            JsonObject command;
            string output, input;
            StringContainer strInput, strOutput;

            input = R"({"method":"WifiControl.1.config@Test\\","params":{"ssid":"Test\\"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 1: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method1":"Te\\st","params":{"ssid":"Te\\st","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 2: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.config@Test","params":{"ssid":"Test\\","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 3: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"hPho\\ne","params":{"ssid":"iPh\one"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 4: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n\n\n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"("hPh\\one")";
            Core::JSON::String str;
            str.FromString(input);
            str.ToString(output);
            printf("\n\n Case 5: \n");
            printf("string --- = %s \n", str.Value().c_str());
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"Iphone\\"})";
            strInput.Name = R"(Iphone\)";
            strOutput.FromString(input);
            printf("\n\n Case 6: \n");
            printf("strInput --- = %s \n", strInput.Name.Value().c_str());
            printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            strInput.Name = R"(Ipho\ne)";
            strOutput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 7: \n");
            printf("strInput --- = %s \n", strInput.Name.Value().c_str());
            printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            strInput.Name = R"(Iphone\)";
            printf("\n\n Case 8: \n");
            printf("name --- = %s \n", strInput.Name.Value().c_str());
            strInput.ToString(output);
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            strOutput.FromString(output);
            printf("name --- = %s \n", strInput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            input = R"({"name":"IPh\\one"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 9: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPhone\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 10: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPho\\\\ne\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 11: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"\\IPh\\one\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 12: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"\\\\\\IPhone\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 13: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPho\ne"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 14: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            char name[] = { 73, 80, 104, 111, 13, 101, 0 };
            input = name;
            strInput.Name = (input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 15: \n");
            printf("     input  %zd --- = %s \n", input.length(), input.c_str());
            printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("     output %zd --- = %s \n", output.length(), output.c_str());
            printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            char escapeSequence = 13;
            input = escapeSequence;
            strInput.Name = (input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 16: \n");
            printf("     input  %zd --- = %s \n", input.length(), input.c_str());
            printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("     output %zd --- = %s \n", output.length(), output.c_str());
            printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            escapeSequence = 10;
            input = escapeSequence;
            printf("\n\n Case 17 \n");
            WPEFramework::Core::ProxyType<CommandRequest> commandInput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            WPEFramework::Core::JSON::Tester<1, CommandRequest> parserInput;
            commandInput->B = input;
            output.clear();
            parserInput.ToString(commandInput, output);
            WPEFramework::Core::ProxyType<CommandRequest> commandOutput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            WPEFramework::Core::JSON::Tester<1, CommandRequest> parserOutput;
            parserOutput.FromString(output, commandOutput);
            EXPECT_STREQ(commandInput->B.Value().c_str(), commandOutput->B.Value().c_str());

            Core::JSONRPC::Message message;
            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.connect","params":{"ssid":"iPhone\\"}})";
            message.FromString(input);
            message.ToString(output);
            EXPECT_STREQ(input.c_str(), output.c_str());
            printf("\n\n Case 18: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            ParamsInfo paramsInfo;
            message.Parameters.ToString(input);
            paramsInfo.FromString(message.Parameters.Value());
            paramsInfo.ToString(output);
            printf("message.params = %s\n", message.Parameters.Value().c_str());
            printf("message.params.ssid = %s\n", paramsInfo.Ssid.Value().c_str());
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());

            EXPECT_STREQ(input.c_str(), output.c_str());
       }
    }

    TEST(JSONParser, BigParser)
    {
        {
            // Boolean test
            string input = R"({"D":true})";
            string translated = R"({"D":true})";

            Core::JSON::Tester<512, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // Boolean test
            string input = R"({"D":false})";
            string translated = R"({"D":false})";

            Core::JSON::Tester<512, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // null test
            string input = R"({"A":null,"B":null,"C":null,"D":null,"E":null})";
            string translated = R"({"A":null,"B":null,"C":null,"D":null,"E":null})";

            Core::JSON::Tester<512, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // SmallTest
            string input = R"({"A":"1","B":"3.2","C":"Text"})";
            string translated = R"({"A":1,"B":3.2,"C":"Text"})";

            Core::JSON::Tester<512, SmallTest> parser;
            Core::ProxyType<SmallTest> output = Core::ProxyType<SmallTest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            // SmallTest
            string input = R"({"A":["test"],"B":"3.2","C":"-65.22"})";
            string translated = R"({"A":["test"],"B":3.2,"C":-65.22})";

            Core::JSON::Tester<512, SmallTest2> parser;
            Core::ProxyType<SmallTest2> output = Core::ProxyType<SmallTest2>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"],"k":"1.1","l":"2.11"},"m":["Test"],"n":"3.2","o":"-65.22"})";
            string translated = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22],"k":1.1,"l":2.11},"m":["Test"],"n":3.2,"o":-65.22})";

            Core::JSON::Tester<512, CommandRequest> parser;
            Core::ProxyType<CommandRequest> output = Core::ProxyType<CommandRequest>::Create();
            string received;

            parser.FromString(input, output);
            parser.ToString(output, received);
            EXPECT_STREQ(translated.c_str(), received.c_str());
        }
        {
            //Tester
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"],"k":"1.1","l":"2.11"},"m":["Test"],"n":"3.2","o":"-65.22"})";
            string inputRequired = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22],"k":1.1,"l":2.11},"m":["Test"],"n":3.2,"o":-65.22})";
            string output;
            WPEFramework::Core::ProxyType<CommandRequest> command = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            command->A = -90;
            command->B = _T("TestIdentifier");
            command->C = 90;
            command->D = true;
            command->E = 12;
            command->F.G = -12;
            command->F.H = -44;
            command->F.I = JSONTestEnum::ENUM_4;
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(6, true));
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(14, true));
            command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(22, true));
            command->F.K = static_cast<float>(1.1);
            command->F.L = 2.11;
            command->N = static_cast<float>(3.2);
            command->O = -65.22;

            WPEFramework::Core::JSON::String str;
            str = string("Test");
            command->M.Add(str);
            WPEFramework::Core::JSON::Tester<512, CommandRequest> parser;
            //ToString
            parser.ToString(command, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());
            //FromString
            WPEFramework::Core::ProxyType<CommandRequest> received = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            parser.FromString(input, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());

            parser.FromString(inputRequired, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(inputRequired.c_str(), output.c_str());

            //ArrayType Iterator
            WPEFramework::Core::JSON::ArrayType<Core::JSON::DecUInt16>::Iterator settings(command->F.J.Elements());
            for (int i = 0; settings.Next(); i++)
                EXPECT_EQ(settings.Current().Value(), command->F.J[i]);
            //null test
            input = R"({"a":null,"b":null,"c":"0x5A","d":null,"f":{"g":"-014","h":-44}})";
            parser.FromString(input, received);
            output.clear();
            parser.ToString(received, output);
            EXPECT_STREQ(input.c_str(), output.c_str());
        }
        //JsonObject and JsonValue
        {
            string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22]}})";
            JsonObject command;
            command.FromString(input);
            string output;
            command.ToString(output);
            EXPECT_STREQ(input.c_str(), output.c_str());

            JsonObject object;
            object["g"] = "-014";
            object["h"] = -44;
            object["i"] = "enum_4";
            JsonArray arrayValue;
            arrayValue.Add(6);
            arrayValue.Add(14);
            arrayValue.Add(22);
            object["j"] = arrayValue;
            JsonObject demoObject;
            demoObject["a"] = "-0x5A";
            demoObject["b"] = "TestIdentifier";
            demoObject["c"] = "0x5A";
            demoObject["d"] = true;
            demoObject["e"] = "014";
            demoObject["f"] = object;
            string serialized;
            demoObject.ToString(serialized);
            EXPECT_STREQ(input.c_str(), serialized.c_str());

            JsonObject::Iterator index = demoObject.Variants();
            while (index.Next()) {
                JsonValue value(demoObject.Get(index.Label()));
                EXPECT_EQ(value.Content(), index.Current().Content());
                EXPECT_STREQ(value.Value().c_str(), index.Current().Value().c_str());
            }
        }
        //JsonObject Serialization and Deserialization with escape sequences
        {
            StringContainer strInput, strOutput;

            JsonObject command;
            string output, input;

            input = R"({"method":"WifiControl.1.config@Test\\","params":{"ssid":"Test\\"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 1: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method1":"Te\\st","params":{"ssid":"Te\\st","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 2: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.config@Test","params":{"ssid":"Test\\","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 3: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"hPho\\ne","params":{"ssid":"iPh\one"}})";
            command.FromString(input);
            command.ToString(output);
            printf("\n\n Case 4: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n\n\n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"("hPh\\one")";
            Core::JSON::String str;
            str.FromString(input);
            str.ToString(output);
            printf("\n\n Case 5: \n");
            printf("string --- = %s \n", str.Value().c_str());
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"Iphone\\"})";
            strInput.Name = R"(Iphone\)";
            strOutput.FromString(input);
            printf("\n\n Case 6: \n");
            printf("strInput --- = %s \n", strInput.Name.Value().c_str());
            printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            strInput.Name = R"(Ipho\ne)";
            strOutput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 7: \n");
            printf("strInput --- = %s \n", strInput.Name.Value().c_str());
            printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            strInput.Name = R"(Iphone\)";
            printf("\n\n Case 8: \n");
            printf("name --- = %s \n", strInput.Name.Value().c_str());
            strInput.ToString(output);
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            strOutput.FromString(output);
            printf("name --- = %s \n", strInput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            input = R"({"name":"IPh\\one"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 9: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPhone\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 10: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPho\\\\ne\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 11: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"\\IPh\\one\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 12: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"\\\\\\IPhone\\\\"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 13: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            input = R"({"name":"IPho\ne"})";
            strInput.FromString(input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 14: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
            EXPECT_STREQ(input.c_str(), output.c_str());

            char name[] = { 73, 80, 104, 111, 13, 101, 0 };
            input = name;
            strInput.Name = (input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 15: \n");
            printf("     input  %zd --- = %s \n", input.length(), input.c_str());
            printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("     output %zd --- = %s \n", output.length(), output.c_str());
            printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            char escapeSequence = 13;
            input = escapeSequence;
            strInput.Name = (input);
            strInput.ToString(output);
            strOutput.FromString(output);
            printf("\n\n Case 16: \n");
            printf("     input  %zd --- = %s \n", input.length(), input.c_str());
            printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
            printf("     output %zd --- = %s \n", output.length(), output.c_str());
            printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
            EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

            escapeSequence = 10;
            input = escapeSequence;
            printf("\n\n Case 17 \n");
            WPEFramework::Core::ProxyType<CommandRequest> commandInput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            WPEFramework::Core::JSON::Tester<512, CommandRequest> parserInput;
            commandInput->B = input;
            output.clear();
            parserInput.ToString(commandInput, output);
            WPEFramework::Core::ProxyType<CommandRequest> commandOutput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
            WPEFramework::Core::JSON::Tester<512, CommandRequest> parserOutput;
            parserOutput.FromString(output, commandOutput);
            EXPECT_STREQ(commandInput->B.Value().c_str(), commandOutput->B.Value().c_str());

            Core::JSONRPC::Message message;
            input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.connect","params":{"ssid":"iPhone\\"}})";
            message.FromString(input);
            message.ToString(output);
            EXPECT_STREQ(input.c_str(), output.c_str());
            printf("\n\n Case 18: \n");
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());
            ParamsInfo paramsInfo;
            message.Parameters.ToString(input);
            paramsInfo.FromString(message.Parameters.Value());
            paramsInfo.ToString(output);
            printf("message.params = %s\n", message.Parameters.Value().c_str());
            printf("message.params.ssid = %s\n", paramsInfo.Ssid.Value().c_str());
            printf("input  %zd --- = %s \n", input.length(), input.c_str());
            printf("output %zd --- = %s \n", output.length(), output.c_str());

            EXPECT_STREQ(input.c_str(), output.c_str());
        }
    }
}
}
