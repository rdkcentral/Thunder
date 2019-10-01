#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_TextReader, simple_TextReader)
{
    TextReader();

    uint8_t* val = (uint8_t*) "Checking the fragment";
    DataElement element(21,val);

    TextReader Reader(element);
    TextReader Reader1(Reader);

    Reader.Reset();
    EXPECT_FALSE(Reader.EndOfText());
    TextFragment fragment = Reader.ReadLine();
    EXPECT_STREQ(fragment.Data(),"Checking the fragment");
}
