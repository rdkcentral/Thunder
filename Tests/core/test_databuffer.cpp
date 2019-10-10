#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

TEST(Core_DataBuffer, simpleSet)
{
    uint32_t bufferSize = 10;    
    uint32_t size;
    uint8_t* received = new uint8_t[bufferSize + 1];
    memset(received, 0, (bufferSize + 1));

    Core::CyclicDataBuffer<Core::ScopedStorage<10>> buffer;
    EXPECT_TRUE(buffer.IsEmpty());
    EXPECT_EQ(buffer.Filled(), 0u);
    EXPECT_EQ(buffer.Free(), bufferSize);

    string data = "abcdefghi";
    size = buffer.Write((uint8_t*)data.c_str(), data.size());
    EXPECT_EQ(buffer.Filled(), 9u);
    EXPECT_EQ(buffer.Free(), 1u);
    EXPECT_EQ(size, data.size());

    size = buffer.Read(received, 4);
    received[size] = '\0';
    EXPECT_STREQ((char*)received, "abcd");
    EXPECT_EQ(buffer.Filled(), 5u);
    EXPECT_EQ(buffer.Free(), 5u);
    EXPECT_EQ(size, 4u);

    data = "jklmnopq";
    size = buffer.Write((uint8_t*)data.c_str(), data.size());
    EXPECT_EQ(buffer.Filled(), 9u);
    EXPECT_EQ(buffer.Free(), 1u);
    EXPECT_EQ(size, data.size());

    size = buffer.Read((uint8_t*)received, buffer.Filled());
    received[size] = '\0';
    EXPECT_STREQ((char*)received, "ijklmnopq");
    EXPECT_EQ(buffer.Filled(), 0u);
    EXPECT_EQ(buffer.Free(), 10u);
    EXPECT_EQ(size, 9u);

    delete[] received;
    Core::Singleton::Dispose();
}
