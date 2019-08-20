#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

TEST(Core_DataBuffer, simpleSet)
{
    uint32_t bufferSize = 10;    
    uint32_t size;
    Core::CyclicDataBuffer<Core::ScopedStorage<10>> buffer;
    ASSERT_TRUE(buffer.IsEmpty());
    ASSERT_EQ(buffer.Filled(), 0);
    ASSERT_EQ(buffer.Free(), bufferSize);

    string data = "abcdefghi";
    size = buffer.Write((uint8_t*)data.c_str(), data.size());
    ASSERT_EQ(buffer.Filled(), 9);
    ASSERT_EQ(buffer.Free(), 1);
    ASSERT_EQ(size, data.size());

    uint8_t* received = new uint8_t[bufferSize + 1];
    memset(received, 0, sizeof(received));
    size = buffer.Read(received, 4);
    received[size] = '\0';
    ASSERT_STREQ((char*)received, "abcd");
    ASSERT_EQ(buffer.Filled(), 5);
    ASSERT_EQ(buffer.Free(), 5);
    ASSERT_EQ(size, 4);

    data = "jklmnopq";
    size = buffer.Write((uint8_t*)data.c_str(), data.size());
    ASSERT_EQ(buffer.Filled(), 10);
    ASSERT_EQ(buffer.Free(), 0);
    ASSERT_EQ(size, data.size());

    size = buffer.Read((uint8_t*)received, buffer.Filled());
    received[size] = '\0';
    ASSERT_STREQ((char*)received, "hijklmnopq");
    ASSERT_EQ(buffer.Filled(), 0);
    ASSERT_EQ(buffer.Free(), 10);
    ASSERT_EQ(size, 10);

    delete[] received;
    Core::Singleton::Dispose();
}
