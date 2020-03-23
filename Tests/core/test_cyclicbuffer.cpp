#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace WPEFramework;
using namespace WPEFramework::Core;

namespace Tests {

    class ThreadClass : public Thread {
    public:
        ThreadClass() = delete;
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

    public:
        ThreadClass(CyclicBuffer* cyclicBuffer,bool* cyclicThreadDone, std::mutex* cyclicMutex,std::condition_variable* cyclicCV)
            : Thread(Thread::DefaultStackSize(), _T("Test"))
            , _cyclicBuffer(cyclicBuffer)
            , _cyclicThreadDone(cyclicThreadDone)
            , _cyclicMutex(cyclicMutex)
            , _cyclicCV(cyclicCV)
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!*_cyclicThreadDone)) {
                _cyclicBuffer->Alert();
                std::unique_lock<std::mutex> lk(*_cyclicMutex);
                *_cyclicThreadDone = true;
                _cyclicCV->notify_one();
            }
            return (infinite);
        }

    private:
        CyclicBuffer* _cyclicBuffer;
        bool* _cyclicThreadDone;
        std::mutex* _cyclicMutex;
        std::condition_variable* _cyclicCV;
    };

    TEST(Core_CyclicBuffer, WithoutOverwrite)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            uint32_t result;
            string data;
            uint32_t cyclicBufferSize = 10;
            uint8_t loadBuffer[cyclicBufferSize + 1];
            char bufferName[] = "cyclicbuffer02";

            CyclicBuffer buffer(bufferName, cyclicBufferSize, false);

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
            uint32_t cyclicBufferSize = 10;
            uint8_t loadBuffer[cyclicBufferSize + 1];
            char bufferName[] = "cyclicbuffer02";

            CyclicBuffer buffer(bufferName, cyclicBufferSize, false);

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
            uint32_t cyclicBufferSize = 10;
            uint8_t loadBuffer[cyclicBufferSize + 1];
            char bufferName[] = "cyclicbuffer03";

            CyclicBuffer buffer(bufferName, cyclicBufferSize, true);

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
            uint32_t cyclicBufferSize = 10;
            uint8_t loadBuffer[cyclicBufferSize + 1];
            char bufferName[] = "cyclicbuffer03";

            CyclicBuffer buffer(bufferName, cyclicBufferSize, true);

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
        char bufferName[] = "cyclicbuffer04";
        uint32_t cyclicBufferSize = 10;

        CyclicBuffer buffer(bufferName, cyclicBufferSize, true);
        buffer.Lock(false,500);
        buffer.Unlock();
        buffer.Lock(true,1000);
    }

    TEST(Core_CyclicBuffer, lock_unlock)
    {
        char bufferName[] = "cyclicbuffer01";
        uint32_t cyclicBufferSize = 10;

        CyclicBuffer buffer(bufferName, cyclicBufferSize, true);

        bool cyclicThreadDone = false;
        std::mutex cyclicMutex;
        std::condition_variable cyclicCV;

        ThreadClass object(&buffer,&cyclicThreadDone,&cyclicMutex,&cyclicCV);
        object.Run();
        buffer.Lock(true,infinite);
        std::unique_lock<std::mutex> lk(cyclicMutex);
        while(!cyclicThreadDone)
        {
            cyclicCV.wait(lk);
        }
        object.Stop();
    }
} // Tests
