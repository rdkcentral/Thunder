#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

namespace Tests {

const char g_bufferName[] = "cyclicbuffer01";
uint32_t bufferSize = 10;
Core::CyclicBuffer cyclicBuffer(g_bufferName, bufferSize, true);
bool g_cyclicThreadDone = false;
class ThreadClass : public Core::Thread {
private:
    ThreadClass(const ThreadClass&) = delete;
    ThreadClass& operator=(const ThreadClass&) = delete;

public:
    ThreadClass()
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
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
        return (Core::infinite);
    }
};

TEST(Core_CyclicBuffer, WithoutOverwrite)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer buffer(g_bufferName, bufferSize, false);

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
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer buffer(g_bufferName, bufferSize, false);

        testAdmin.Sync("setup client");

        testAdmin.Sync("server wrote");

        result = buffer.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        EXPECT_STREQ((char*)loadBuffer, "abcd");

        testAdmin.Sync("client read");

        data = "klmnopq";
        result = buffer.Reserve(data.size());
        EXPECT_EQ(result, Core::ERROR_INVALID_INPUT_LENGTH);
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, 0u);

        testAdmin.Sync("client wrote");

        testAdmin.Sync("server peek");

        testAdmin.Sync("server read");
    }
    Core::Singleton::Dispose();
}

TEST(Core_CyclicBuffer, WithOverwrite)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer buffer(g_bufferName, bufferSize, true);

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

        EXPECT_EQ(buffer.ErrorCode(),0u);
        EXPECT_EQ(buffer.LockPid(),0u);
        EXPECT_EQ(buffer.Free(),10u);

        EXPECT_STREQ(buffer.Name().c_str(),g_bufferName);
        EXPECT_STREQ(buffer.Storage().Name().c_str(),g_bufferName);

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
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer buffer(g_bufferName, bufferSize, true);

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
    Core::Singleton::Dispose();
}
TEST(Core_CyclicBuffer, lock)
{
    cyclicBuffer.Lock(false,500);
    cyclicBuffer.Unlock();
    cyclicBuffer.Lock(true,1000); 
}
TEST(Core_CyclicBuffer, lock_unlock)
{
    ThreadClass object;
    object.Run();
    cyclicBuffer.Lock(true,Core::infinite);
    while(!g_cyclicThreadDone);
    object.Stop();
}
} // Tests
