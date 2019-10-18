#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_text_parser, simple_text_parser)
{
    TextFragment inputLine("/Service/testing/parsertest");
    TextParser textparser(inputLine);
    textparser.Skip(2);
    textparser.Skip('S');
    //textparser.Find('t');
    textparser.SkipWhiteSpace();
    textparser.SkipLine();
    OptionalType<TextFragment> token;
    textparser.ReadText(token, _T("/"));
}
TEST(test_path_parser, simple_path_parser)
{
    TextFragment inputFile("C://Service/testing/pathparsertest.txt");
    PathParser pathparser(inputFile);
    pathparser.Drive();
    pathparser.Path();
    pathparser.FileName();
    pathparser.BaseFileName();
    pathparser.Extension();
}
