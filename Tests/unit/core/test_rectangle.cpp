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

    TEST(test_rectangle, simple_rectangle)
    {
        ::Thunder::Core::Rectangle();
        ::Thunder::Core::Rectangle r1(2,3,2,3); 
        ::Thunder::Core::Rectangle r2(2,4,2,4);
        EXPECT_FALSE(r1 == r2);
        EXPECT_TRUE(r1 != r2);
     
        ::Thunder::Core::Rectangle r3(2,3,2,3); 
        EXPECT_TRUE(r1 == r3);
        EXPECT_FALSE(r1 != r3);
         
        ::Thunder::Core::Rectangle r4 = r1 & r2;
        ::Thunder::Core::Rectangle r5(2,4,2,2);
        EXPECT_TRUE(r4 == r5);

        ::Thunder::Core::Rectangle r6 = r1 | r2;
        ::Thunder::Core::Rectangle r7(2,3,2,5);
        EXPECT_TRUE(r6 == r7);
        
        ::Thunder::Core::Rectangle r8 = r1.combine(r2);
        EXPECT_TRUE(r8 == r7);

        EXPECT_TRUE(r1.Contains(2,3));
        EXPECT_TRUE(r1.Overlaps(r2));
    }

} // Core
} // Tests
} // Thunder
