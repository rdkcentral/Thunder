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

    enum class TestEnum {
        ONE,
        TWO,
        THREE
    };    

    TEST(Core_Enumerate, CheckEntries)
    {
        Core::EnumerateType<TestEnum> testEnum;
        EXPECT_FALSE(testEnum.IsSet());
        for (int i = 0; i < 3; i++) {
            EXPECT_EQ(testEnum.Entry(i)->value, (i == 0 ? TestEnum::ONE : (i == 1 ? TestEnum::TWO : TestEnum::THREE)));
            EXPECT_STREQ(testEnum.Entry(i)->name, (i == 0 ? "ONE" : (i == 1 ? "TWO" : "THREE")));
        }
    }

    TEST(Core_Enumerate, Assignment)
    {
        Core::EnumerateType<TestEnum> testEnum;
        testEnum.Assignment(true, "three");
        EXPECT_FALSE(testEnum.IsSet());

        testEnum.Assignment(true, "THREE");
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");

        testEnum.Assignment(false, "three");
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");

        testEnum.Assignment(false, "four");
        EXPECT_FALSE(testEnum.IsSet());

        testEnum.Clear();
        EXPECT_FALSE(testEnum.IsSet());

        Core::EnumerateType<TestEnum> testEnum1;
        testEnum.Assignment(true, "three");

        EXPECT_FALSE(testEnum != testEnum1);
        EXPECT_TRUE(testEnum != testEnum1.Value());

        Core::EnumerateType<TestEnum> testEnum2(testEnum1);
        Core::EnumerateType<TestEnum> testEnum3;
        testEnum3 = testEnum1;
        EXPECT_TRUE(testEnum3 == testEnum1);
        testEnum3 = testEnum1.Value(); 
    }

    TEST(Core_Enumerate, FromEnumType)
    {
        TestEnum e = TestEnum::TWO;
        Core::EnumerateType<TestEnum> testEnum(e);
        EXPECT_EQ(testEnum, e);
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::TWO);
        EXPECT_STREQ(testEnum.Data(), "TWO");
    }

    TEST(Core_Enumerate, FromValue)
    {
        Core::EnumerateType<TestEnum> testEnum(1);
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::TWO);
        EXPECT_STREQ(testEnum.Data(), "TWO");

        testEnum = 3;
        EXPECT_FALSE(testEnum.IsSet());
    }

    TEST(Core_Enumerate, FromStringCaseSensitiveTrue)
    {
        Core::EnumerateType<TestEnum> testEnum("THREE");
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");
    }

    TEST(Core_Enumerate, FromStringCaseSensitiveFalse)
    {
        Core::EnumerateType<TestEnum> testEnum("three");
        EXPECT_FALSE(testEnum.IsSet());
    }

    TEST(Core_Enumerate, FromStringCaseInsensitive)
    {
        Core::EnumerateType<TestEnum> testEnum("three", false);
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");
    }

    TEST(Core_Enumerate, FromTextFragmentCaseSensitiveTrue)
    {
        Core::TextFragment testFragment("THREE");
        Core::EnumerateType<TestEnum> testEnum(testFragment);
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");
    }

    TEST(Core_Enumerate, FromTextFragmentCaseSensitiveFalse)
    {
        Core::TextFragment testFragment("three");
        Core::EnumerateType<TestEnum> testEnum(testFragment);
        EXPECT_FALSE(testEnum.IsSet());
    }

    TEST(Core_Enumerate, FromTextFragmentCaseInsensitive)
    {
        Core::TextFragment testFragment("three");
        Core::EnumerateType<TestEnum> testEnum(testFragment, false);
        EXPECT_TRUE(testEnum.IsSet());
        EXPECT_EQ(testEnum.Value(), TestEnum::THREE);
        EXPECT_STREQ(testEnum.Data(), "THREE");
    }
} // Tests
ENUM_CONVERSION_BEGIN(Tests::TestEnum)
    { Tests::TestEnum::ONE, _TXT("ONE") },
    { Tests::TestEnum::TWO, _TXT("TWO") },
    { Tests::TestEnum::THREE, _TXT("THREE") },
ENUM_CONVERSION_END(Tests::TestEnum)
} // Thunder
