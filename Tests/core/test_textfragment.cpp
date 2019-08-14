#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/TextFragment.h>
#include <core/SystemInfo.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_textfragment, simple_textfragement)
{
    string buffer = "/Service/testing/test";
    TextFragment result;
    result = TextFragment(string(buffer));
    Core::TextFragment(string(buffer));
    Core::TextSegmentIterator index(Core::TextFragment(string(buffer), 16, 5), false,'/');
    index.Next();
    index.Next();

    ASSERT_STREQ(index.Remainder().Text().c_str(), _T("test")) << "The remainder string is not test";
    ASSERT_STREQ(index.Remainder().Data(), _T("test")) << "The remainder string is not test";
    ASSERT_STREQ(index.Current().Text().c_str(), _T("test")) << "The current string is not test";
    ASSERT_EQ(index.Remainder().Length(), 4) << "The length of the string is not 4.";
    ASSERT_FALSE(index.Remainder().IsEmpty());
    
    Core::TextFragment textFragment();
    const TCHAR buffer_new[] = "/Service/testing/test";
    Core::TextFragment textFragment1(buffer_new);
    Core::TextFragment textFragment2(buffer_new,21);
    Core::TextFragment textFragment3(buffer_new,16,5);
}
