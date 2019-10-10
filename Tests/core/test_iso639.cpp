#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_ISO639, simple_ISO639)
{
    Language();
    Language lang("eng");
    Language(12);
     
    EXPECT_TRUE(lang.IsValid());
    EXPECT_STREQ(lang.LetterCode3(),"eng");
    EXPECT_STREQ(lang.LetterCode2(),"en");
    EXPECT_STREQ(lang.Description(),"Engels");
    EXPECT_EQ(lang.Id(),127);
}

