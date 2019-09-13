#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_frame, simple_set)
{
    uint8_t arr[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    uint8_t arr1[] = {1,2,3,4,5,6,7,8,9,10,11,12,13};
    const uint16_t BLOCKSIZE = 20;
    FrameType<BLOCKSIZE> obj1;
    FrameType<BLOCKSIZE> obj_copy(obj1);
    FrameType<0> obj2(arr,15);
    uint16_t len = 15;
    uint16_t len1 = 13;
    const uint16_t offset = 0;
    EXPECT_EQ(obj1.SetBuffer<uint64_t>(offset,len,arr),23);
    EXPECT_EQ(obj1.GetBuffer<uint64_t>(offset,len1,arr1),23);

    FrameType<BLOCKSIZE>::Writer obj3(obj1,offset);
    obj3.Buffer<uint64_t>(15,arr);
    obj3.Copy(13, arr1);
    obj3.Number<uint16_t>(4);
    obj3.Boolean(TRUE);
    obj3.Text("Frametype");
    obj3.NullTerminatedText("Frametype");

    obj1.Size(5000);
    FrameType<BLOCKSIZE>::Reader obj4(obj1,offset);
    obj4.Buffer<uint16_t>(15,arr);
    obj4.Copy(13,arr1);
    obj4.Number<uint16_t>();
    obj4.Boolean();
    obj4.Text();
    obj4.NullTerminatedText();
    obj4.UnlockBuffer(15);
    obj4.Dump();
    uint32_t Size = 5000;
    EXPECT_EQ(obj1.Size(),Size);
    obj1.Clear();
}
