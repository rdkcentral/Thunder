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
