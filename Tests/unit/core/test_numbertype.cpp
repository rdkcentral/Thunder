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

namespace Thunder {
namespace Tests {

    template <typename T, bool NEGATIVE = false>
    T Tester(string& text)
    {
        T num(text.c_str(), text.size());
        EXPECT_EQ((NEGATIVE ? -90 : 90), num.Value());
        EXPECT_TRUE(NEGATIVE == num.Negative());
        return num;
    }
    
    TEST(Core_NumberType, generic)
    {
        Core::NumberType<uint8_t> numbertype;
        string buffer = "/Service/testing/numbertype";
        Core::NumberType<Core::TextFragment> typetextfragment(Core::TextFragment(string(buffer)));

        string valdata = "90";
        Core::NumberType<uint8_t> val1 = Tester<Core::NumberType<uint8_t>>(valdata);
        Core::NumberType<int8_t> val2 = Tester<Core::NumberType<int8_t>>(valdata);
        EXPECT_EQ(Core::NumberType<uint8_t>::ToNetwork(val1),'Z');
        EXPECT_EQ(Core::NumberType<uint8_t>::FromNetwork(val1),'Z');
        EXPECT_EQ(Core::NumberType<int8_t>::ToNetwork(val2),'Z');
        EXPECT_EQ(Core::NumberType<int8_t>::FromNetwork(val2),'Z');

        Core::NumberType<int16_t> val3 = Tester<Core::NumberType<int16_t>>(valdata);
        Core::NumberType<uint16_t> val4 = Tester<Core::NumberType<uint16_t>>(valdata);
        EXPECT_EQ(Core::NumberType<int16_t>::ToNetwork(val3),23040);
        EXPECT_EQ(Core::NumberType<int16_t>::FromNetwork(val3),23040);
        EXPECT_EQ(Core::NumberType<uint16_t>::ToNetwork(val4),23040);
        EXPECT_EQ(Core::NumberType<uint16_t>::FromNetwork(val4),23040);

        Core::NumberType<int32_t> val5 = Tester<Core::NumberType<int32_t>>(valdata);
        Core::NumberType<uint32_t> val6 = (unsigned)Tester<Core::NumberType<int32_t>>(valdata);
        EXPECT_EQ(Core::NumberType<int32_t>::ToNetwork(val5),1509949440);
        EXPECT_EQ(Core::NumberType<int32_t>::FromNetwork(val5),1509949440);
        EXPECT_EQ(Core::NumberType<uint32_t>::ToNetwork(val6),1509949440u);
        EXPECT_EQ(Core::NumberType<uint32_t>::FromNetwork(val6),1509949440u);

        Core::NumberType<int64_t> val7 = Tester<Core::NumberType<int64_t>>(valdata);
        Core::NumberType<uint64_t> val8 = (unsigned)Tester<Core::NumberType<int64_t>>(valdata);
        EXPECT_EQ(Core::NumberType<int64_t>::ToNetwork(val7),90);
        EXPECT_EQ(Core::NumberType<int64_t>::FromNetwork(val7),90);
        EXPECT_EQ(Core::NumberType<uint64_t>::ToNetwork(val8),90u);
        EXPECT_EQ(Core::NumberType<uint64_t>::FromNetwork(val8),90u);

        val8.MaxSize();
        EXPECT_EQ(val8.Serialize(valdata),2);
        EXPECT_EQ(val8.Deserialize(valdata),2);
        
        string valdata1 = "90";
        Core::NumberType<int8_t> num1 = Tester<Core::NumberType<int8_t>, false>(valdata);
        Core::NumberType<int8_t> num2 = Tester<Core::NumberType<int8_t>, false>(valdata1);

        EXPECT_TRUE(num1==num2);
        EXPECT_FALSE(num1!=num2);
        EXPECT_TRUE(num1<=num2);
        EXPECT_TRUE(num1>=num2);
        EXPECT_FALSE(num1<num2);
        EXPECT_FALSE(num1>num2);
     
        EXPECT_STREQ((num1+num1).Text().c_str(),"-76");
        EXPECT_STREQ((num1-num2).Text().c_str(),"0");
        EXPECT_STREQ((num2*num1).Text().c_str(),"-92");
        EXPECT_STREQ((num2/num1).Text().c_str(),"1");
        num2/=num1;
        EXPECT_STREQ(num2.Text().c_str(),"1");
        num2-=num1;
        EXPECT_STREQ(num2.Text().c_str(),"-89");
        num1+=num1;
        EXPECT_STREQ(num1.Text().c_str(),"-76");
        num1*=num2;
        EXPECT_STREQ(num1.Text().c_str(),"108");
    }

    TEST(Core_NumberType, PositiveNumber)
    {
        string data = "90";
        Core::NumberType<uint8_t> num1 = Tester<Core::NumberType<uint8_t>>(data);
        EXPECT_STREQ(num1.Text().c_str(), "90");
        Core::NumberType<int8_t> num2 = Tester<Core::NumberType<int8_t>>(data);
        EXPECT_STREQ(num2.Text().c_str(), "90");
    }

    TEST(Core_NumberType, NegativeNumber)
    {
        string data = "-90";
        Core::NumberType<int8_t> num = Tester<Core::NumberType<int8_t>, true>(data);
        EXPECT_STREQ(num.Text().c_str(), "-90");
    }

    TEST(Core_NumberType, NumberOutOfLimit)
    {
        string data = "256";
        Core::NumberType<uint8_t> num1(data.c_str(), data.size());
        EXPECT_NE(256, num1.Value());

        data = "128";
        Core::NumberType<int8_t> num2(data.c_str(), data.size());
        EXPECT_NE(128, num2.Value());

        data = "-129";
        Core::NumberType<int8_t> num3(data.c_str(), data.size());
        EXPECT_NE(-129, num3.Value());
    }

    TEST(Core_NumberType, PositiveHexNumber)
    {
        string data = "5A";
        Core::NumberType<uint8_t, false, BASE_HEXADECIMAL> num1 = Tester<Core::NumberType<uint8_t, false, BASE_HEXADECIMAL>>(data);
        EXPECT_STREQ(num1.Text().c_str(), "0x5A");
        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num2 = Tester<Core::NumberType<int8_t, true, BASE_HEXADECIMAL>>(data);
        EXPECT_STREQ(num2.Text().c_str(), "0x5A");

        data = "0x5A";
        Core::NumberType<uint8_t, false, BASE_HEXADECIMAL> num3 = Tester<Core::NumberType<uint8_t, false, BASE_HEXADECIMAL>>(data);
        EXPECT_STREQ(num3.Text().c_str(), "0x5A");
        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num4 = Tester<Core::NumberType<int8_t, true, BASE_HEXADECIMAL>>(data);
        EXPECT_STREQ(num4.Text().c_str(), "0x5A");

        Core::NumberType<uint8_t> num5 = Tester<Core::NumberType<uint8_t>>(data);
        EXPECT_STREQ(num5.Text().c_str(), "90");
        Core::NumberType<int8_t> num6 = Tester<Core::NumberType<int8_t>>(data);
        EXPECT_STREQ(num6.Text().c_str(), "90");

        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num7(num4);
        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num8 = num4;
    }

    TEST(Core_NumberType, NegativeHexNumber)
    {
        string data = "-5A";
        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num1 = Tester<Core::NumberType<int8_t, true, BASE_HEXADECIMAL>, true>(data);
        EXPECT_STREQ(num1.Text().c_str(), "-0x5A");

        data = "-0x5A";
        Core::NumberType<int8_t, true, BASE_HEXADECIMAL> num2 = Tester<Core::NumberType<int8_t, true, BASE_HEXADECIMAL>, true>(data);
        EXPECT_STREQ(num2.Text().c_str(), "-0x5A");

        Core::NumberType<int8_t> num3 = Tester<Core::NumberType<int8_t>, true>(data);
        EXPECT_STREQ(num3.Text().c_str(), "-90");
    }

    TEST(Core_NumberType, PositiveOctNumber)
    {
        string data = "132";
        Core::NumberType<uint8_t, false, BASE_OCTAL> num1 = Tester<Core::NumberType<uint8_t, false, BASE_OCTAL>>(data);
        EXPECT_STREQ(num1.Text().c_str(), "0132");
        Core::NumberType<int8_t, true, BASE_OCTAL> num2 = Tester<Core::NumberType<int8_t, true, BASE_OCTAL>>(data);
        EXPECT_STREQ(num2.Text().c_str(), "0132");

        data = "0132";
        Core::NumberType<uint8_t, false, BASE_OCTAL> num3 = Tester<Core::NumberType<uint8_t, false, BASE_OCTAL>>(data);
        EXPECT_STREQ(num3.Text().c_str(), "0132");
        Core::NumberType<int8_t, true, BASE_OCTAL> num4 = Tester<Core::NumberType<int8_t, true, BASE_OCTAL>>(data);
        EXPECT_STREQ(num4.Text().c_str(), "0132");

        Core::NumberType<uint8_t> num5 = Tester<Core::NumberType<uint8_t>>(data);
        EXPECT_STREQ(num5.Text().c_str(), "90");
        Core::NumberType<int8_t> num6 = Tester<Core::NumberType<int8_t>>(data);
        EXPECT_STREQ(num6.Text().c_str(), "90");
    }

    TEST(Core_NumberType, NegativeOctNumber)
    {
        string data = "-132";
        Core::NumberType<int8_t, true, BASE_OCTAL> num1 = Tester<Core::NumberType<int8_t, true, BASE_OCTAL>, true>(data);
        EXPECT_STREQ(num1.Text().c_str(), "-0132");

        data = "-0132";
        Core::NumberType<int8_t, true, BASE_OCTAL> num2 = Tester<Core::NumberType<int8_t, true, BASE_OCTAL>, true>(data);
        EXPECT_STREQ(num2.Text().c_str(), "-0132");

        Core::NumberType<int8_t> num3 = Tester<Core::NumberType<int8_t>, true>(data);
        EXPECT_STREQ(num3.Text().c_str(), "-90");
    }

    TEST(Core_NumberType, ConversionFunctions)
    {
        EXPECT_EQ(Core::FromDigits('a'), 0);
        EXPECT_EQ(Core::FromDigits('1'), 1);

        EXPECT_EQ(Core::FromHexDigits('1'), 1);
        EXPECT_EQ(Core::FromHexDigits('a'), 10);
        EXPECT_EQ(Core::FromHexDigits('A'), 10);
        EXPECT_EQ(Core::FromHexDigits('z'), 0);

        EXPECT_EQ(Core::FromBase64('1'), 53);
        EXPECT_EQ(Core::FromBase64('a'), 26);
        EXPECT_EQ(Core::FromBase64('A'), 0);
        EXPECT_EQ(Core::FromBase64('+'), 62);
        EXPECT_EQ(Core::FromBase64('/'), 63);
        EXPECT_EQ(Core::FromBase64('-'), 0);

        EXPECT_EQ(Core::FromDirect('a'), 97);
        EXPECT_EQ(Core::ToDigits(9), '9');

        EXPECT_EQ(Core::ToHexDigits(1), '1');
        EXPECT_EQ(Core::ToHexDigits(10), 'A');
        EXPECT_EQ(Core::ToHexDigits(30), '0');

        EXPECT_EQ(Core::ToBase64(26), 'a');
        EXPECT_EQ(Core::ToDirect(97), 'a');
    }

    TEST(Core_NumberType, Fractional_test)
    {
        Core::Fractional fractional;
        Core::Fractional fractional1(3,2);
        Core::Fractional fractional2(fractional1);
        Core::Fractional fractional3;
        fractional3 = fractional1;
        EXPECT_EQ(fractional1.Integer(),3);
        EXPECT_EQ(fractional1.Remainder(),2u);

        Core::NumberType<Core::Fractional, true> num1;
        fractional3 = num1.Max();

        EXPECT_EQ(fractional3.Integer(),2147483647);
        EXPECT_EQ(fractional3.Remainder(),4294967295);
        fractional3 = num1.Min();
        EXPECT_EQ(fractional3.Integer(),-2147483648);
        EXPECT_EQ(fractional3.Remainder(),4294967295);
    }
} // Tests
} // Thunder
