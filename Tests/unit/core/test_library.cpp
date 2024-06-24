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

TEST(Core_Library, simpleSet)
{
#ifdef BUILD_ARM
    const string file = _T("/usr/lib/testdata/libhelloworld.so"); // For QEMU
#else
    const string file = string(BUILD_DIR) + _T("/libhelloworld.so"); // For PC
    //const string file =  _T("/usr/lib/libwpe-0.2.so"); //For box.
#endif
    const TCHAR* function = _T("HelloWorld");
    const string file1 = _T("libThunder.so");

    Library LibObj1(file.c_str());
    EXPECT_TRUE(LibObj1.Error().empty());

    LibObj1.LoadFunction(function);
    EXPECT_TRUE(LibObj1.IsLoaded());
    EXPECT_TRUE(LibObj1.Error().empty());

    Library LibObj2(LibObj1); // Copy constructor
    EXPECT_TRUE(LibObj1.Error().empty());
    EXPECT_TRUE(LibObj2.Error().empty());

    Library LibObj3;
    LibObj3 = LibObj2; // Copy assignment

    EXPECT_TRUE(LibObj2.Error().empty());
    EXPECT_TRUE(LibObj3.Error().empty()); // Same as LibObj2
    EXPECT_EQ(LibObj3.Name(), file);
}
