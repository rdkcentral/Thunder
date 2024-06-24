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

TEST(test_keyvaluetype, simple_keyvaluetype)
{
    string key = "Name";
    string value = "Adam";
    string key1 = "Age";
   
    KeyValueType<string,string> keyvalue(key);
    KeyValueType<string,string> keyvalue1(key,value);
    KeyValueType<string,string> keyvalue2(keyvalue1);
    KeyValueType<string,string> keyvalue3(key);
    keyvalue3 = keyvalue1;
    KeyValueType<string,string> keyvalue4(key1);

    EXPECT_TRUE(keyvalue3 == keyvalue1);
    EXPECT_TRUE(keyvalue3 != keyvalue4);
    
    EXPECT_STREQ(keyvalue3.Key().c_str(),key.c_str());
    EXPECT_STREQ(keyvalue3.Value().c_str(),value.c_str());
    EXPECT_TRUE(keyvalue3.IsKey(key));
}

TEST(test_textkeyvaluetype, simple_textkeyvaluetype)
{
   string buffer = "/Service/testing/test";
   TextFragment key;
   key = TextFragment(string(buffer));
   OptionalType<TextFragment> value = TextFragment(string(buffer));
   const bool CASESENSITIVE = true;

   TextKeyValueType<CASESENSITIVE, TextFragment> textkeyvaluetype;
   TextKeyValueType<CASESENSITIVE, TextFragment> textkeyvaluetype1(key, value);
}
