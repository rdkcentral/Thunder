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

using namespace Thunder;
using namespace Thunder::Core;

TEST(test_ISO639, simple_ISO639)
{
    Language();
    Language lang("eng");
    Language(12);
    Language lang1(" ");
    Language lang2("en");
    Language lang3("aaaaaa");
     
    EXPECT_TRUE(lang.IsValid());
    EXPECT_STREQ(lang.LetterCode3(),"eng");
    EXPECT_STREQ(lang.LetterCode2(),"en");
    EXPECT_STREQ(lang.Description(),"Engels");
    EXPECT_EQ(lang.Id(),127);
}

