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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    class Deserializer {
    private:
        typedef Thunder::Core::ParserType<Thunder::Core::TerminatorCarriageReturnLineFeed, Deserializer> Parser;

    public :
        Deserializer()
           : _parser(*this)
        {
        }

        void operations()
        {
            _parser.Reset();
            _parser.CollectWord();
            _parser.CollectWord('/');
            _parser.CollectWord(Thunder::Core::ParserType<Thunder::Core::TerminatorCarriageReturnLineFeed, Deserializer>::ParseState::UPPERCASE);
            _parser.CollectWord('/',Thunder::Core::ParserType<Thunder::Core::TerminatorCarriageReturnLineFeed, Deserializer>::ParseState::UPPERCASE);
            _parser.CollectLine();
            _parser.CollectLine(Thunder::Core::ParserType<Thunder::Core::TerminatorCarriageReturnLineFeed, Deserializer>::ParseState::UPPERCASE);
            _parser.FlushLine();
            _parser.PassThrough(5);
        }

        ~Deserializer()
        {
        }

    private:
        Parser _parser;
    };

    TEST(test_parser_type, simple_parser_type)
    {
        Deserializer deserializer;
        deserializer.operations();
    }

    TEST(test_text_parser, simple_text_parser)
    {
        Thunder::Core::TextFragment inputLine("/Service/testing/parsertest");
        Thunder::Core::TextParser textparser(inputLine);
        textparser.Skip(2);
        textparser.Skip('S');
        textparser.Find(_T("e"));
        textparser.SkipWhiteSpace();
        textparser.SkipLine();
        Thunder::Core::OptionalType<Thunder::Core::TextFragment> token;
        textparser.ReadText(token, _T("/"));
    }

    TEST(test_path_parser, simple_path_parser)
    {
        Thunder::Core::TextFragment inputFile("C://Service/testing/pathparsertest.txt");
        Thunder::Core::PathParser pathparser(inputFile);
        
        EXPECT_EQ(pathparser.Drive().Value(),'C');
        EXPECT_STREQ(pathparser.Path().Value().Text().c_str(),_T("//Service/testing"));
        EXPECT_STREQ(pathparser.FileName().Value().Text().c_str(),_T("pathparsertest.txt"));
        EXPECT_STREQ(pathparser.BaseFileName().Value().Text().c_str(),_T("pathparsertest"));
        EXPECT_STREQ(pathparser.Extension().Value().Text().c_str(),_T("txt"));;
    }

} // Core
} // Tests
} // Thunder
