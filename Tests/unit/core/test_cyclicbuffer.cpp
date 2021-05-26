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
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace WPEFramework;
using namespace WPEFramework::Core;

namespace {

    class ThreadClass : public Thread {
    public:
        ThreadClass() = delete;
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

        ThreadClass(CyclicBuffer& cyclicBuffer, volatile bool& done, std::mutex& mutex, std::condition_variable& cv)
            : Thread(Thread::DefaultStackSize(), _T("Test"))
            , _cyclicBuffer(cyclicBuffer)
            , _done(done)
            , _mutex(mutex)
            , _cv(cv)
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!_done)) {
                _cyclicBuffer.Alert();
                std::unique_lock<std::mutex> lk(_mutex);
                _done = true;
                _cv.notify_one();
            }
            return (infinite);
        }

    private:
        CyclicBuffer& _cyclicBuffer;
        volatile bool& _done;
        std::mutex& _mutex;
        std::condition_variable& _cv;
    };
}

TEST(Core_CyclicBuffer, WithoutOverwrite)
{
    std::string bufferName {"cyclicbuffer01"};
    auto lambdaFunc = [bufferName](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, false);

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

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);
    {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, false);

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

TEST(Core_CyclicBuffer, WithoutOverwriteReversed)
{
    std::string bufferName {"cyclicbuffer02"};
    auto lambdaFunc = [bufferName](IPTestAdministrator & testAdmin) {
        testAdmin.Sync("setup client");

        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, false);

        testAdmin.Sync("setup server");

        testAdmin.Sync("client wrote");

        result = buffer.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        EXPECT_STREQ((char*)loadBuffer, "abcd");

        testAdmin.Sync("server read");
        data = "klmnopq";
        result = buffer.Reserve(data.size());
        EXPECT_EQ(result, ERROR_INVALID_INPUT_LENGTH);
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, 0u);

        testAdmin.Sync("server wrote");

        testAdmin.Sync("client peek");

        EXPECT_FALSE(buffer.Overwritten());
        EXPECT_FALSE(buffer.IsLocked());

        EXPECT_EQ(buffer.LockPid(), 0u);
        EXPECT_EQ(buffer.Free(), 5u);

        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_STREQ(buffer.Storage().Name().c_str(), bufferName.c_str());

        testAdmin.Sync("client read");
    };

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);
    {
        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, false);

        testAdmin.Sync("setup client");

        testAdmin.Sync("setup server");

        EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

        data = "abcdefghi";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());
        testAdmin.Sync("client wrote");

        testAdmin.Sync("server read");
        testAdmin.Sync("server wrote");

        result = buffer.Peek(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 5u);
        EXPECT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("client peek");
        result = buffer.Read(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';
        EXPECT_EQ(result, 5u);
        EXPECT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("client read");
    }
    Singleton::Dispose();
}

TEST(Core_CyclicBuffer, WithOverwrite)
{
    std::string bufferName {"cyclicbuffer03"};

    auto lambdaFunc = [bufferName](IPTestAdministrator & testAdmin) {
        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, true);

        testAdmin.Sync("setup server");

        testAdmin.Sync("setup client");

        data = "abcdefghi";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        EXPECT_EQ(result, data.size());

        testAdmin.Sync("server wrote");

        testAdmin.Sync("client read");

        testAdmin.Sync("client wrote");

        result = buffer.Peek(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';

        testAdmin.Sync("server peek");

        result = buffer.Read(loadBuffer, buffer.Used());
        loadBuffer[result] = '\0';

        buffer.Alert();
        buffer.Flush();

        testAdmin.Sync("server read");
    };

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);
    {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        uint32_t cyclicBufferSize = 10;
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName.c_str(),
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, true);

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

TEST(Core_CyclicBuffer, WithOverwriteReversed)
{
    constexpr char bufferName[] = "cyclicbuffer04";
    constexpr uint32_t cyclicBufferSize = 20;

    auto lambdaFunc = [bufferName, cyclicBufferSize](IPTestAdministrator & testAdmin) {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        
        uint8_t loadBuffer[cyclicBufferSize + 1];

        CyclicBuffer buffer(bufferName,
            Core::File::USER_READ    |
            Core::File::USER_WRITE   |
            Core::File::USER_EXECUTE |
            Core::File::GROUP_READ   |
            Core::File::GROUP_WRITE  |
            Core::File::OTHERS_READ  |
            Core::File::OTHERS_WRITE |
            Core::File::SHAREABLE,
            cyclicBufferSize, true);

        testAdmin.Sync("setup client");

        testAdmin.Sync("server wrote");

        result = buffer.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';

        testAdmin.Sync("client read");

        data = "j";
        result = buffer.Reserve(8);
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());

        data = "klmnopq";
        result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());

        testAdmin.Sync("client wrote");

        testAdmin.Sync("server peek");

        testAdmin.Sync("server read");
    };

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as server
    IPTestAdministrator testAdmin(otherSide);

    uint32_t result;
    string data;
    uint8_t loadBuffer[cyclicBufferSize + 1];

    CyclicBuffer buffer(bufferName,
        Core::File::USER_READ    |
        Core::File::USER_WRITE   |
        Core::File::USER_EXECUTE |
        Core::File::GROUP_READ   |
        Core::File::GROUP_WRITE  |
        Core::File::OTHERS_READ  |
        Core::File::OTHERS_WRITE |
        Core::File::SHAREABLE,
        cyclicBufferSize, true);

    testAdmin.Sync("setup server");

    testAdmin.Sync("setup client");

    EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

    data = "abcdefghi";
    result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    EXPECT_EQ(result, data.size());

    testAdmin.Sync("server wrote");

    testAdmin.Sync("client read");

    testAdmin.Sync("client wrote");

    uint32_t used = buffer.Used();

    result = buffer.Peek(loadBuffer, used);
    loadBuffer[result] = '\0';
    EXPECT_EQ(result, 13u);
    EXPECT_STREQ((char*)loadBuffer, "efghijklmnopq");

    testAdmin.Sync("server peek");

    result = buffer.Read(loadBuffer, buffer.Used());
    loadBuffer[result] = '\0';
    EXPECT_EQ(result, 13u);
    EXPECT_STREQ((char*)loadBuffer, "efghijklmnopq");

    buffer.Alert();
    buffer.Flush();

    EXPECT_FALSE(buffer.Overwritten());
    EXPECT_FALSE(buffer.IsLocked());

    EXPECT_EQ(buffer.LockPid(), 0u);
    EXPECT_EQ(buffer.Free(), 20u);

    EXPECT_STREQ(buffer.Name().c_str(), bufferName);
    EXPECT_STREQ(buffer.Storage().Name().c_str(), bufferName);

    EXPECT_TRUE(buffer.IsOverwrite());
    EXPECT_TRUE(buffer.IsValid());

    testAdmin.Sync("server read");

    Singleton::Dispose();
}

TEST(Core_CyclicBuffer, lock)
{
    char bufferName[] = "cyclicbuffer05";
    uint32_t cyclicBufferSize = 10;

    CyclicBuffer buffer(bufferName,
        Core::File::USER_READ    |
        Core::File::USER_WRITE   |
        Core::File::USER_EXECUTE |
        Core::File::GROUP_READ   |
        Core::File::GROUP_WRITE  |
        Core::File::OTHERS_READ  |
        Core::File::OTHERS_WRITE |
        Core::File::SHAREABLE,
        cyclicBufferSize, true);

    buffer.Lock(false, 500);
    buffer.Unlock();
    buffer.Lock(true, 1000);
}

TEST(Core_CyclicBuffer, lock_unlock)
{
    char bufferName[] = "cyclicbuffer06";
    uint32_t bufferSize = 10;

    CyclicBuffer buffer(bufferName,
        Core::File::USER_READ    |
        Core::File::USER_WRITE   |
        Core::File::USER_EXECUTE |
        Core::File::GROUP_READ   |
        Core::File::GROUP_WRITE  |
        Core::File::OTHERS_READ  |
        Core::File::OTHERS_WRITE |
        Core::File::SHAREABLE,
        bufferSize, true);

    volatile bool done = false;
    std::mutex mutex;
    std::condition_variable cv;

    ThreadClass object(buffer, done, mutex, cv);
    object.Run();
    buffer.Lock(true, infinite);
    std::unique_lock<std::mutex> lk(mutex);
    while (!done) {
        cv.wait(lk);
    }
    object.Stop();
}
