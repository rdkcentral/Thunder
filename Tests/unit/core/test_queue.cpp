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

TEST(test_queue, simple_queue)
{
    QueueType<int> obj1(20);
    EXPECT_TRUE(obj1.Insert(20,300));
    EXPECT_TRUE(obj1.Insert(30,300));
    EXPECT_TRUE(obj1.Post(20));
    EXPECT_TRUE(obj1.Remove(20));
    int a_Result = 30;
    EXPECT_TRUE(obj1.Extract(a_Result,300));
    EXPECT_EQ(obj1.Length(),1u);
    obj1.Enable();
    obj1.Disable();
    obj1.Flush();
}
