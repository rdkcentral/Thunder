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

TEST(test_optional, simple_test)
{
    OptionalType<int> obj;
    int type = obj;
    EXPECT_EQ(type, 0);
    obj.Clear();
    EXPECT_FALSE(obj.IsSet());
    OptionalType<string> obj1("request");
    OptionalType<string> obj2(obj1);
    EXPECT_TRUE(obj2 == obj1);
    EXPECT_TRUE(obj2.Value() == obj1.Value());
    EXPECT_TRUE(obj1.IsSet());
    const OptionalType<string> obj3;
    OptionalType<string> obj4;
    obj4 = obj3;
    const OptionalType<int> obj5;
    const OptionalType<int> objSample = 40;
    OptionalType<int> object;
    object = 20;
    EXPECT_TRUE(objSample != object.Value());
    EXPECT_TRUE(objSample != obj5);
    int type1 = objSample;
    EXPECT_EQ(type1, 40);
}
