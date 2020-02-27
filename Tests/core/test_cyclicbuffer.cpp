#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

namespace Tests {

const char g_cyclicBuffer[] = "cyclicbuffer01";
uint32_t g_cyclicBufferSize = 10;
CyclicBuffer cyclicBuffer(g_cyclicBuffer, g_cyclicBufferSize, true);
bool g_cyclicThreadDone = false;

class ThreadClass : public Thread {
private:
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

public:
    ThreadClass()
        : Thread(Thread::DefaultStackSize(), _T("Test"))
    {
    }

    virtual ~ThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!g_cyclicThreadDone)) {
            sleep(2);
            cyclicBuffer.Alert();
            g_cyclicThreadDone = true;
        }
        return (infinite);
    }
};

TEST(Core_CyclicBuffer, WithoutOverwrite)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint8_t loadBuffer[g_cyclicBufferSize + 1];
        const char bufferName[] = "cyclicbuffer02";
        CyclicBuffer buffer(bufferName, g_cyclicBufferSize, false);

        testAdmin.Sync("setup server");

        testAdmin.Sync("setup client");

        EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

        data = "abcdefghi";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());

        testAdmin.Sync("server wrote");

        testAdmin.Sync("client read");

        testAdmin.Sync("client wrote");

        result = buffer.Peek(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 5u);
        EXPECT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("server peek");

        result = buffer.Read(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 5u);
        EXPECT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("server read");
    };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);

    {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        uint8_t loadBuffer[g_cyclicBufferSize + 1];
        const char bufferName[] = "cyclicbuffer02";
        CyclicBuffer buffer(bufferName, g_cyclicBufferSize, false);

        testAdmin.Sync("setup client");

        testAdmin.Sync("server wrote");

        result = buffer.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        EXPECT_STREQ((char*)loadBuffer, "abcd");

        testAdmin.Sync("client read");

        data = "klmnopq";
        result = buffer.Reserve(data.size());
        EXPECT_EQ(result, ERROR_INVALID_INPUT_LENGTH);
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, 0u);

        testAdmin.Sync("client wrote");

        testAdmin.Sync("server peek");

        testAdmin.Sync("server read");
    }
    Singleton::Dispose();
}

TEST(Core_CyclicBuffer, WithOverwrite)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint8_t loadBuffer[g_cyclicBufferSize + 1];
        const char bufferName[] = "cyclicbuffer03";
        CyclicBuffer buffer(bufferName, g_cyclicBufferSize, true);

        testAdmin.Sync("setup server");

        testAdmin.Sync("setup client");

        EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

        data = "abcdefghi";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());

        testAdmin.Sync("server wrote");

        testAdmin.Sync("client read");

        testAdmin.Sync("client wrote");

        result = buffer.Peek(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 9u);
        EXPECT_STREQ((char*)loadBuffer, "ijklmnopq");

        testAdmin.Sync("server peek");

        result = buffer.Read(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 9u);
        EXPECT_STREQ((char*)loadBuffer, "ijklmnopq");

        buffer.Alert();
        buffer.Flush();

        EXPECT_FALSE(buffer.Overwritten());
        EXPECT_FALSE(buffer.IsLocked());

        EXPECT_EQ(buffer.ErrorCode(),2u);
        EXPECT_EQ(buffer.LockPid(),0u);
        EXPECT_EQ(buffer.Free(),10u);

        EXPECT_STREQ(buffer.Name().c_str(),bufferName);
        EXPECT_STREQ(buffer.Storage().Name().c_str(),bufferName);

        EXPECT_TRUE(buffer.IsOverwrite());
        EXPECT_TRUE(buffer.IsValid());

        testAdmin.Sync("server read");
    };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);

    {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        uint8_t loadBuffer[g_cyclicBufferSize + 1];
        const char bufferName[] = "cyclicbuffer03";
        CyclicBuffer buffer(bufferName, g_cyclicBufferSize, true);

        testAdmin.Sync("setup client");

        testAdmin.Sync("server wrote");

        result = buffer.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        EXPECT_STREQ((char*)loadBuffer, "abcd");

        testAdmin.Sync("client read");

        data = "j";
        result = buffer.Reserve(8);
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());
        data = "klmnopq";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());

        testAdmin.Sync("client wrote");

        testAdmin.Sync("server peek");

        testAdmin.Sync("server read");
    }
    Singleton::Dispose();
}
TEST(Core_CyclicBuffer, lock)
{
    const char bufferName[] = "cyclicbuffer04";
    CyclicBuffer cyclicBuffer1(bufferName, g_cyclicBufferSize, true);
    cyclicBuffer1.Lock(false,500);
    cyclicBuffer1.Unlock();
    cyclicBuffer1.Lock(true,1000);
}
TEST(Core_CyclicBuffer, lock_unlock)
{
    ThreadClass object;
    object.Run();
    cyclicBuffer.Lock(true,infinite);
    sleep(2);
    while(!g_cyclicThreadDone);
    object.Stop();
}
} // Tests
