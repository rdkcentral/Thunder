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

#include "JSON.h"

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
    TEST(DISABLED_JSONParser, ExponentialNumber)
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

    TEST(DISABLED_JSONParser, DoubleNumber)
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

    TEST(DISABLED_JSONParser, StringWithEscapeSequenceDoublequote)
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

    TEST(DISABLED_JSONParser, StringWithEscapeSequenceBackslash)
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
            value.FromString(data.value);
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
        ExecutePrimitiveJsonTest<Core::JSON::EnumType<JSONTestEnum>>(data, true, nullptr);
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
} // Tests

ENUM_CONVERSION_BEGIN(Tests::JSONTestEnum)
    { WPEFramework::Tests::JSONTestEnum::ONE, _TXT("one") },
    { WPEFramework::Tests::JSONTestEnum::TWO, _TXT("two") },
ENUM_CONVERSION_END(Tests::JSONTestEnum)

} // WPEFramework
