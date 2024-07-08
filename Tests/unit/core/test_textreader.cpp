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

    TEST(test_TextReader, simple_TextReader)
    {
        ::Thunder::Core::TextReader();

        uint8_t* val = (uint8_t*) "Checking the fragment";
        ::Thunder::Core::DataElement element(21,val);

        ::Thunder::Core::TextReader Reader(element);
        ::Thunder::Core::TextReader Reader1(Reader);

        Reader.Reset();
        EXPECT_FALSE(Reader.EndOfText());
        ::Thunder::Core::TextFragment fragment = Reader.ReadLine();
        EXPECT_STREQ(fragment.Data(),"Checking the fragment");
    }

} // Core
} // Tests
} // Thunder
