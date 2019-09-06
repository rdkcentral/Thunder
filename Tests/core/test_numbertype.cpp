#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    template <typename T, bool NEGATIVE = false>
    T Tester(string& text)
    {
        T num(text.c_str(), text.size());
        EXPECT_EQ((NEGATIVE ? -90 : 90), num.Value());
        EXPECT_TRUE(NEGATIVE == num.Negative());
        return num;
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
}
}
