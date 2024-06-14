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

TEST(test_textfragment, simple_textfragement)
{
    string buffer = "/Service/testing/test";
    TextFragment result;
    result = TextFragment(string(buffer));
    TextFragment(string(buffer));
    TextSegmentIterator index(TextFragment(string(buffer), 16, 5), false,'/');
    index.Next();
    index.Next();

    EXPECT_STREQ(index.Remainder().Text().c_str(), _T("test")) << "The remainder string is not test";
    EXPECT_STREQ(index.Remainder().Data(), _T("test")) << "The remainder string is not test";
    EXPECT_STREQ(index.Current().Text().c_str(), _T("test")) << "The current string is not test";
    EXPECT_EQ(index.Remainder().Length(), 4u) << "The length of the string is not 4.";
    EXPECT_FALSE(index.Remainder().IsEmpty());
    
    TextFragment textFragment;
    const TCHAR buffer_new[] = "/Service/testing/test";
    TextFragment textFragment1(buffer_new);
    TextFragment textFragment2(buffer_new,21);
    TextFragment textFragment3(buffer_new,16,5);

    char delimiter[] = {'/'};
    EXPECT_EQ(textFragment1.ForwardFind(delimiter),0u);
    textFragment1.Clear();
    EXPECT_EQ(textFragment2[1],'S');

    EXPECT_FALSE(textFragment1 == buffer_new);
    EXPECT_FALSE(textFragment1 == buffer);
    EXPECT_TRUE(textFragment1 != buffer_new);
    EXPECT_TRUE(textFragment1 != buffer);

    EXPECT_TRUE(textFragment2.OnMarker(buffer_new));
    EXPECT_TRUE(textFragment2.EqualText(buffer_new));

    const TCHAR begin[] = "Service";
    const TCHAR end[] = "test";
    const TCHAR middle[] = "testing";
    textFragment2.TrimBegin(begin);
    textFragment2.TrimEnd(end);
    EXPECT_EQ(textFragment2.ReverseFind(middle),15u);
    EXPECT_EQ(textFragment2.ReverseSkip(middle),16u);

    const TCHAR splitters[] ={'/',','};
    TextSegmentIterator iterator1(TextFragment(string(buffer), 16, 5), false,splitters);
    TextSegmentIterator iterator2(iterator1);
    index = iterator2;
    iterator2.Reset();
    TextSegmentIterator iterator3;
}
