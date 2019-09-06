#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/DataElement.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_data, simple_data)
{
    const uint8_t arr[] = {10,20,30,40,50,60,70,80,90,100};
    const uint16_t len = 5;
    const uint16_t off = 0;
    DataStore obj1;
    obj1.Copy(arr,len,off);
    uint32_t size = 21474837504;
    ASSERT_EQ(obj1.Size(),size);
    obj1.Size(1025);
    ASSERT_EQ(*obj1.Buffer(),10);

    uint8_t arr1[] = {10,20,30,40,50,60,70,80,90,100};
    const uint64_t s = 10;
    DataElement obj_sample;
    DataElement obj2(s,arr1);
    DataElement obj_operator = obj2;
    uint32_t crc = 4134036294;
    ASSERT_EQ(obj2.CRC32(0,10),crc);
    ASSERT_TRUE(obj2.Copy(obj2,0));

    ASSERT_EQ(obj2.AllocatedSize(),10);
    ASSERT_TRUE(obj2.Size(sizeof(arr)));
    ASSERT_TRUE(obj2.IsValid());
    ASSERT_EQ(*obj2.Buffer(),10);

    uint8_t arr2[] = {10,20,30,40,50};
    const uint8_t infom = 10;
    uint64_t SearchNumber = 10;
    ASSERT_EQ((obj2.SearchNumber<uint8_t, Core::ENDIAN_BIG>(2,infom)),SearchNumber);
    obj2.SetNumber<uint64_t, Core::ENDIAN_BIG>(2,10);
    uint64_t GetNumber = 10;
    ASSERT_EQ((obj2.GetNumber<uint64_t, Core::ENDIAN_BIG>(2)),GetNumber);

    obj2.Search(2,arr2,5);
    obj2.SetBitNumber<uint64_t>(2,5,8,10);
    uint64_t GetBitNumber = 10;
    ASSERT_EQ(obj2.GetBitNumber<uint64_t>(2,5,8),GetBitNumber);

    const uint8_t val= 32;
    const uint64_t offset= 0;
    obj2.Set(val,offset);
    obj2.Align<uint8_t>();

    DataElement obj(obj2);
    DataElement obj3 = obj;
    DataElement obj4(obj2,0,0);

    ASSERT_EQ(obj3.Size(),10);
    ASSERT_FALSE(obj3.Expand(0,0));
    ASSERT_TRUE(obj3.Shrink(0,0));
    ASSERT_FALSE(obj3.Copy(obj2));

}



TEST(test_linkeddata, simple_linkeddata)
{
    uint8_t arr[] = {10,20,30,40,50,60,70,80,90,100};
    uint8_t arr1[] ={10,20,30};
    const uint64_t offset= 0;
    const uint32_t size = 0;
    DataElement objt1(10,arr);
    LinkedDataElement ob1,ob2;
    LinkedDataElement ob3(objt1,0,0);
    LinkedDataElement opertr = ob3;
    ob1.SetBuffer(offset,size,arr);
    ob1.GetBuffer(offset,size,arr1);
    ASSERT_EQ(ob2.Copy(offset,ob3),0);

    ob1.Enclosed();
    ob2.Enclosed(&ob1);
    ASSERT_EQ(ob2.LinkedSize(),0);
    uint32_t LElement = 2;
    ASSERT_EQ(ob2.LinkedElements(),LElement);
}

TEST(test_dataParser, simple_dataParser)
{
    uint8_t arr[] = {10,20,30,40,50,60,70,80,90,100};
    DataElement object1(10,arr);
    DataElementParser parser1(object1,0);
    ASSERT_TRUE(parser1.IsValid());
    uint64_t size = 18446744073709551606;
    ASSERT_EQ(parser1.Size(),size);
    parser1.SkipBytes(2);
}
