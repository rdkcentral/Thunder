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

TEST(test_frame, simple_set)
{
    uint8_t arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    uint8_t arr1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    const uint16_t BLOCKSIZE = 20;
    FrameType<BLOCKSIZE> obj1;
    FrameType<BLOCKSIZE> obj_copy(obj1);
    FrameType<0> obj2(arr, 15);
    uint16_t len = 15;
    uint16_t len1 = 13;
    uint16_t offset = 0;
    EXPECT_EQ(obj1.SetBuffer<uint16_t>(offset, len, arr), 17u);
    EXPECT_EQ(obj1.GetBuffer<uint16_t>(offset, len1, arr1), 17u);

    FrameType<BLOCKSIZE>::Writer obj3(obj1, offset);
    obj3.Buffer<uint16_t>(15, arr);
    obj3.Copy(13, arr1);
    obj3.Number<uint16_t>(4);
    obj3.Boolean(TRUE);
    obj3.Text("Frametype");
    obj3.NullTerminatedText("Frametype");

    obj1.Size(5000);
    FrameType<BLOCKSIZE>::Reader obj4(obj1, offset);
    obj4.Buffer<uint16_t>(15, arr);
    obj4.Copy(13, arr1);
    obj4.Number<uint16_t>();
    obj4.Boolean();
    obj4.Text();
    obj4.NullTerminatedText();
    obj4.UnlockBuffer(15);
    // TODO: why doesn't this work when inlining is disabled?
    //obj4.Dump();
    uint32_t Size = 5000;
    EXPECT_EQ(obj1.Size(), Size);
    obj1.Clear();
}
