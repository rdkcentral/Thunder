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

#include <core/DataElement.h>

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(test_data, simple_data)
    {
        const uint8_t arr[] = {10,20,30,40,50,60,70,80,90,100};
        const uint16_t len = 5;
        const uint16_t off = 0;
        ::Thunder::Core::DataStore obj1;
        obj1.Copy(arr,len,off);
        uint32_t size = 1024;
        EXPECT_EQ(obj1.Size(),size);
        ASSERT_EQ(obj1.Size(), 1024);
        EXPECT_EQ(*obj1.Buffer(),10);

        uint8_t arr1[] = {10,20,30,40,50,60,70,80,90,100};
        const uint64_t s = 10;
        ::Thunder::Core::DataElement obj_sample;
        ::Thunder::Core::DataElement obj2(s,arr1);
        ::Thunder::Core::DataElement obj_operator = obj2;
        uint32_t crc = 4134036294;
        EXPECT_EQ(obj2.CRC32(0,10),crc);
        EXPECT_TRUE(obj2.Copy(obj2,0));

        uint64_t alloc_size = 10;
        EXPECT_EQ(obj2.AllocatedSize(), alloc_size);
        EXPECT_TRUE(obj2.Size(sizeof(arr)));
        EXPECT_TRUE(obj2.IsValid());
        EXPECT_EQ(*obj2.Buffer(),10);

        uint8_t arr2[] = {10,20,30,40,50};
        const uint8_t infom = 10;
        uint64_t SearchNumber = 10;
        EXPECT_EQ((obj2.SearchNumber<uint8_t, ::Thunder::Core::ENDIAN_BIG>(2,infom)),SearchNumber);
        obj2.SetNumber<uint64_t, ::Thunder::Core::ENDIAN_BIG>(2,10);
        uint64_t GetNumber = 10;
        EXPECT_EQ((obj2.GetNumber<uint64_t, ::Thunder::Core::ENDIAN_BIG>(2)),GetNumber);

        EXPECT_EQ(obj2.Search(2,arr2,5), obj2.Size()); // Unable to find pattern
        obj2.SetBitNumber<uint64_t>(2,5,8,10);
        uint64_t GetBitNumber = 10;
        EXPECT_EQ(obj2.GetBitNumber<uint64_t>(2,5,8),GetBitNumber);

        const uint8_t val= 32;
        const uint64_t offset= 0;
        obj2.Set(val,offset);
        obj2.Align<uint8_t>();

        ::Thunder::Core::DataElement obj(obj2);
        ::Thunder::Core::DataElement obj3 = obj;
        ::Thunder::Core::DataElement obj4(obj2,0,0);

        uint32_t ob_size = 10;
        EXPECT_EQ(obj3.Size(),ob_size);
        ASSERT_TRUE(obj3.Shrink(obj3.AllocatedSize()-obj3.Size()+1,obj3.Size()-1));
        ASSERT_FALSE(obj3.Expand(obj3.AllocatedSize()-obj3.Size()-1,obj3.Size()+1)); // No internal storage, not the owner of the buffer
        ASSERT_TRUE(obj3.Expand(obj3.AllocatedSize()-obj3.Size()-1,obj3.Size())); // No internal storage, but also no actual change in size
        ASSERT_TRUE(obj3.Expand(obj3.AllocatedSize()-obj3.Size()-1,obj3.AllocatedSize()));
        EXPECT_TRUE(obj3.Copy(obj2));
    }

    TEST(test_linkeddata, simple_linkeddata)
    {
        uint8_t arr[] = {10,20,30,40,50,60,70,80,90,100};
        uint8_t arr1[sizeof(arr)] = {};
        const uint64_t offset= 0;
        ::Thunder::Core::DataElement objt1(10,arr);
        ::Thunder::Core::LinkedDataElement ob1;
        ::Thunder::Core::LinkedDataElement ob2(objt1);
        ::Thunder::Core::LinkedDataElement ob3(ob2);
        ob3.Enclosed(&ob2);
        EXPECT_EQ(ob3.Enclosed(), &ob2);
        ob3.SetBuffer(2,9,arr);
        ob3.GetBuffer(2,9,arr1);
        ::Thunder::Core::LinkedDataElement ob4;
        ob4 = ob2;
        EXPECT_EQ(ob4.Copy(offset,ob2), unsigned(10));
        EXPECT_EQ(ob2.Copy(offset,ob3), unsigned(10));

        EXPECT_EQ(ob1.Enclosed(), nullptr);
        EXPECT_EQ(ob2.LinkedSize(), unsigned(10));
        EXPECT_EQ(ob2.LinkedElements(),unsigned(1));
    }

    TEST(test_dataParser, simple_dataParser)
    {
        uint8_t arr[] = {10,20,30,40,50};
        ::Thunder::Core::DataElement object1(10,arr);
        ::Thunder::Core::DataElementParser parser1(object1,0);
        uint64_t size = -10;
        EXPECT_TRUE(parser1.IsValid());
        EXPECT_EQ(parser1.Size(),size);
        parser1.SkipBytes(2);
    }

} // Core
} // Tests
} // Thunder
