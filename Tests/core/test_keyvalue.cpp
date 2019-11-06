#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


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

   TextKeyValueType<CASESENSITIVE, TextFragment> textkeyvaluetype();
   TextKeyValueType<CASESENSITIVE, TextFragment> textkeyvaluetype1(key, value);
}
