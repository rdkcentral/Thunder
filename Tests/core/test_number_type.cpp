#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

TEST(Core_NumberType, simpleSet)
{
    Core::NumberType<uint8_t> num1(90);
    string text = "90";
    ASSERT_STREQ(((string)num1).c_str(), text.c_str());
    ASSERT_STREQ(num1.Text().c_str(), text.c_str());
    ASSERT_FALSE(num1.Negative());

    Core::NumberType<int8_t> num2(text.c_str(), text.size());
    ASSERT_TRUE(num1 == num2);

    Core::TextFragment textFragment(text);
    Core::NumberType<uint8_t> num3(textFragment);
    ASSERT_TRUE(num1 == num3);

    text = "-90";
    Core::NumberType<int8_t> num4(text.c_str(), text.size());
    ASSERT_TRUE(num4.Negative());
    ASSERT_EQ(num4.Abs(), 90);

    text = "5A";
    Core::NumberType<uint8_t, false, BASE_HEXADECIMAL> numHex1(text.c_str(), text.size());
    text = "0x5A";
    Core::NumberType<uint8_t> numHex2(text.c_str(), text.size());
    ASSERT_TRUE((numHex1.Value() == 90) && (numHex2.Value() == 90));
    ASSERT_STREQ(text.c_str(), numHex1.Text().c_str());

    text = "0x5A";
    Core::NumberType<int8_t> numHex3(text.c_str(), text.size());
    ASSERT_TRUE(numHex3.Value() == 90);
    text = "-0x5A";
    Core::NumberType<int8_t> numHex4(text.c_str(), text.size());
    ASSERT_TRUE(numHex4.Value() == -90);

    text = "-132";
    Core::NumberType<int8_t, true, BASE_OCTAL> numOct1(text.c_str(), text.size());
    text = "-0132";
    Core::NumberType<int8_t> numOct2(text.c_str(), text.size());
    ASSERT_TRUE((numOct1.Value() == -90) && (numOct2.Value() == -90));
    ASSERT_STREQ(text.c_str(), numOct1.Text().c_str());

    ASSERT_EQ(Core::FromDigits('a'), 0);
    ASSERT_EQ(Core::FromDigits('1'), 1);

    ASSERT_EQ(Core::FromHexDigits('1'), 1);
    ASSERT_EQ(Core::FromHexDigits('a'), 10);
    ASSERT_EQ(Core::FromHexDigits('A'), 10);
    ASSERT_EQ(Core::FromHexDigits('z'), 0);

    ASSERT_EQ(Core::FromBase64('1'), 53);
    ASSERT_EQ(Core::FromBase64('a'), 26);
    ASSERT_EQ(Core::FromBase64('A'), 0);
    ASSERT_EQ(Core::FromBase64('+'), 62);
    ASSERT_EQ(Core::FromBase64('/'), 63);
    ASSERT_EQ(Core::FromBase64('-'), 0);

    ASSERT_EQ(Core::FromDirect('a'), 97);
    ASSERT_EQ(Core::ToDigits(9), '9');

    ASSERT_EQ(Core::ToHexDigits(1), '1');
    ASSERT_EQ(Core::ToHexDigits(10), 'A');
    ASSERT_EQ(Core::ToHexDigits(30), '0');

    ASSERT_EQ(Core::ToBase64(26), 'a');
    ASSERT_EQ(Core::ToDirect(97), 'a');
}
