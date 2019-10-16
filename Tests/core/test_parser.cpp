#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_parser, simple_parser)
{
    Core::TextFragment inputLine("/Service/testing/parsertest");
    Core::TextParser textparser(inputLine);
    textparser.Skip(2);
    textparser.Skip('S');
    //textparser.Find('t');
    textparser.SkipWhiteSpace();
    textparser.SkipLine();
}
