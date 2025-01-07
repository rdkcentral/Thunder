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

#ifdef __APPLE__
#include <time.h>
#endif

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    struct Data {
        bool shareable;
        bool usingDataElementFile;
        uint32_t mode;
        uint32_t offset;
        string bufferName;
    };

    inline uint32_t ResetFileMask()
    {
        return (static_cast<uint32_t>(umask(0)));
    }
    inline void RestoreFileMask(const uint32_t& mask)
    {
        umask(static_cast<mode_t>(mask));
    }
    inline bool IsFileExist(const char fileName[])
    {
        return (access(fileName, F_OK ) == 0);
    }
    inline bool IsValidFilePermissions(const char fileName[], const uint32_t mode)
    {
        bool valid = true;
        struct stat st;
        if (lstat(fileName, &st) == 0) {

            if ((mode & ::Thunder::Core::File::Mode::USER_READ) != (st.st_mode & S_IRUSR)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::Mode::USER_WRITE) != (st.st_mode & S_IWUSR)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::Mode::USER_EXECUTE) != (st.st_mode & S_IXUSR)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::Mode::GROUP_READ) != (st.st_mode & S_IRGRP)) {
                valid = false;
            }
            else if((mode & ::Thunder::Core::File::Mode::GROUP_WRITE) != (st.st_mode & S_IWGRP)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::GROUP_EXECUTE) != (st.st_mode & S_IXGRP)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::OTHERS_READ) != (st.st_mode & S_IROTH)) {
                valid = false;
            }
            else if((mode & ::Thunder::Core::File::OTHERS_WRITE) != (st.st_mode & S_IWOTH)) {
                valid = false;
            }
            else if ((mode & ::Thunder::Core::File::OTHERS_EXECUTE) != (st.st_mode & S_IXOTH)) {
                valid = false;
            } else {
                valid = true;
            }

        } while(0);
        return valid;
    }
    uint32_t FileSize(const char fileName[])
    {
        struct stat st;
        uint32_t size = 0;
        if (lstat(fileName, &st) == 0) {
            size = st.st_size;
        }
        return size;
    }
    struct timespec GetCurrentTime()
    {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        return now;
    }
    void CreateTimeOut(struct timespec& timeOutTime, const uint32_t waitTime)
    {
        timeOutTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000);
        timeOutTime.tv_sec += (waitTime / 1000) + (timeOutTime.tv_nsec / 1000000000);
        timeOutTime.tv_nsec = timeOutTime.tv_nsec % 1000000000;
    }
    bool CheckTimeOutIsExpired(struct timespec timeOutTime, const uint32_t waitTime)
    {
        struct timespec now;

        uint32_t remainingTime = 0;
        CreateTimeOut(timeOutTime, waitTime);
        clock_gettime(CLOCK_REALTIME, &now);

        if (now.tv_nsec > timeOutTime.tv_nsec) {

            remainingTime = (now.tv_sec - timeOutTime.tv_sec) * 1000 +
                   ((now.tv_nsec - timeOutTime.tv_nsec) / 1000000);
        } else {
            remainingTime = (timeOutTime.tv_sec - now.tv_sec - 1) * 1000 +
                    ((1000000000 - (timeOutTime.tv_nsec - now.tv_nsec)) / 1000000);
        }
        return (remainingTime < waitTime);
    }

    const char SampleData[] = "Best";
    const uint32_t MaxSignalWaitTime = 1000; // In milliseconds
    static void* ThreadToCheckBufferIsSharable(void* data)
    {
        ::Thunder::Core::CyclicBuffer* buffer = static_cast<::Thunder::Core::CyclicBuffer*>(data);

        uint32_t written = buffer->Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        static bool shareable = (written == sizeof(SampleData));
        pthread_exit(&shareable);
    }
    bool CheckBufferIsSharable(::Thunder::Core::CyclicBuffer* buffer)
    {
        void *status = nullptr;
        pthread_t threadID;
        pthread_create(&threadID, NULL, &ThreadToCheckBufferIsSharable, static_cast<void*>(buffer));
        pthread_join(threadID, &status);

        return ((status != nullptr) ? *(static_cast<bool *>(status)) : false);
    }
    class ThreadLock : public ::Thunder::Core::Thread {
    public:
        ThreadLock() = delete;
        ThreadLock(const ThreadLock&) = delete;
        ThreadLock& operator=(const ThreadLock&) = delete;

        ThreadLock(::Thunder::Core::CyclicBuffer& cyclicBuffer, uint32_t waitTime, ::Thunder::Core::Event& event)
            : ::Thunder::Core::Thread(::Thunder::Core::Thread::DefaultStackSize(), _T("Test2"))
            , _event(event)
            , _waitTime(waitTime)
            , _cyclicBuffer(cyclicBuffer)
        {
        }

        ~ThreadLock() = default;

        virtual uint32_t Worker() override
        {
            if (IsRunning()) {

                _event.SetEvent();
                _cyclicBuffer.Lock(true, _waitTime);
                _event.SetEvent();
                Block();
            }
            return (::Thunder::Core::infinite);
        }
    private:
        ::Thunder::Core::Event& _event;
        uint32_t _waitTime;
        ::Thunder::Core::CyclicBuffer& _cyclicBuffer;
    };

    class CyclicBufferTest : public ::Thunder::Core::CyclicBuffer {
    public:
       CyclicBufferTest() = delete;
       CyclicBufferTest(const CyclicBufferTest&) = delete;
        CyclicBufferTest& operator=(const CyclicBufferTest&) = delete;

       CyclicBufferTest(const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite)
            : ::Thunder::Core::CyclicBuffer(fileName, mode, bufferSize, overwrite)
        {
        }
       CyclicBufferTest(::Thunder::Core::DataElementFile& dataElementFile, const bool initiator, const uint32_t offset, const uint32_t bufferSize, const bool overwrite)
            : ::Thunder::Core::CyclicBuffer(dataElementFile, initiator, offset, bufferSize, overwrite)
        {
        }

        ~CyclicBufferTest() = default;

        /* virtual */ uint32_t GetOverwriteSize(::Thunder::Core::CyclicBuffer::Cursor& cursor) override
        {
            while (cursor.Offset() < cursor.Size()) {
                uint16_t chunkSize = 0;
                cursor.Peek(chunkSize);

                cursor.Forward(chunkSize);
            }

            return cursor.Offset();
        }
    };
    TEST(Core_CyclicBuffer, Create)
    {
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            ::Thunder::Core::File::Mode::CREATE | ::Thunder::Core::File::Mode::SHAREABLE;

        {
            std::string bufferName {"cyclicbuffer01"};
            uint8_t cyclicBufferSize = 50;
            // Create::Thunder::Core::CyclicBuffer with Size 50
            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);

            // Check File Size
            EXPECT_EQ(FileSize(bufferName.c_str()), (cyclicBufferSize + sizeof(::Thunder::Core::CyclicBuffer::control)));

            // Remove after usage before destruction
            buffer.Close();

            // Check File Exist
            EXPECT_EQ(IsFileExist(bufferName.c_str()), false);
        }
        {
            // Overwrite Buffer with Create
            uint8_t cyclicBufferSize = 100;
            std::string bufferName {"cyclicbuffer02"};
            ::Thunder::Core::CyclicBuffer buffer1(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer1.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer1.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer1.IsValid(), true);
            EXPECT_EQ(buffer1.Open(), true);

            buffer1.~CyclicBuffer();
            EXPECT_EQ(IsFileExist(bufferName.c_str()), true);

            // Create cyclic buffer with same name to check overwritting or not
            cyclicBufferSize = 20;
            ::Thunder::Core::CyclicBuffer buffer2(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer2.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer2.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer2.IsValid(), true);
            EXPECT_EQ(buffer2.Open(), true);

            // Remove after usage before destruction
            buffer2.Close();
            EXPECT_EQ(IsFileExist(bufferName.c_str()), false);
        }
    }
    TEST(Core_CyclicBuffer, CheckAccessOnStorageFile)
    {
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            ::Thunder::Core::File::Mode::CREATE | ::Thunder::Core::File::Mode::SHAREABLE;

        std::string bufferName {"cyclicbuffer01"};
        uint8_t cyclicBufferSize = 50;
        // Create::Thunder::Core::CyclicBuffer with Size 50
        ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_STREQ(buffer.Storage().Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);
        EXPECT_EQ(buffer.Open(), true);

        EXPECT_EQ(buffer.Storage().IsOpen(), true);
        buffer.Close();
        EXPECT_EQ(buffer.Storage().IsOpen(), false);
    }
    TEST(Core_CyclicBuffer, Create_WithDifferentPermissions)
    {
        {
            const uint32_t mask = ResetFileMask();
            std::string bufferName {"cyclicbuffer01"};
            const uint8_t cyclicBufferSize = 50;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::OTHERS_READ | ::Thunder::Core::File::Mode::OTHERS_WRITE;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(IsValidFilePermissions(bufferName.c_str(), mode), true);

            // Remove after usage before destruction
            buffer.Close();
            RestoreFileMask(mask);
        }
        {
            const uint32_t mask = ResetFileMask();
            std::string bufferName {"cyclicbuffer01"};
            const uint8_t cyclicBufferSize = 50;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::SHAREABLE;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);
            EXPECT_EQ(IsValidFilePermissions(bufferName.c_str(), mode), true);
            EXPECT_EQ(CheckBufferIsSharable(&buffer), true);

            // Remove after usage before destruction
            buffer.Close();
            RestoreFileMask(mask);
        }
        {
            const uint32_t mask = ResetFileMask();
            std::string bufferName {"cyclicbuffer01"};
            const uint8_t cyclicBufferSize = 50;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_WRITE |
                ::Thunder::Core::File::Mode::OTHERS_READ | ::Thunder::Core::File::Mode::OTHERS_WRITE;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);
            EXPECT_EQ(IsValidFilePermissions(bufferName.c_str(), mode), true);

            // Remove after usage before destruction
            buffer.Close();
            RestoreFileMask(mask);
        }
        {
            const uint32_t mask = ResetFileMask();
            std::string bufferName {"cyclicbuffer01"};
            const uint8_t cyclicBufferSize = 50;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);
            EXPECT_EQ(IsValidFilePermissions(bufferName.c_str(), mode), true);
            EXPECT_EQ(CheckBufferIsSharable(&buffer), true);

            EXPECT_EQ(buffer.Used(), sizeof(SampleData));
            // Remove after usage before destruction
            buffer.Close();
            RestoreFileMask(mask);
        }
    }
    TEST(Core_CyclicBuffer, Write)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 50;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;
        ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData));
        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData) * 2);
        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData) * 3);
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(SampleData) * 3));

        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Write_BasedOnFreeSpaceAvailable)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 20;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
            ::Thunder::Core::File::Mode::SHAREABLE;

        CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);
        EXPECT_EQ(buffer.IsOverwrite(), true);

        uint8_t data1[10];
        memset(&data1, 'A', sizeof(data1));
        uint16_t size = sizeof(data1) + 2;
        uint8_t reserved = buffer.Reserve(size);
        if (reserved == size) {
            buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            buffer.Write(reinterpret_cast<uint8_t*>(&data1), sizeof(data1));
        }
        EXPECT_EQ(buffer.Overwritten(), false);
        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        uint8_t data2[20];
        memset(&data2, 'B', sizeof(data2));
        size = sizeof(data2) + 2;
        uint8_t previousFreeSpace = buffer.Free();
        buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
        // Try to write to only with free space available
        buffer.Write(reinterpret_cast<uint8_t*>(&data2), buffer.Free());

        EXPECT_EQ(buffer.Used(), previousFreeSpace);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - previousFreeSpace));

        // Remove after usage before destruction
        buffer.Close();
    }

    TEST(Core_CyclicBuffer, Read)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 50;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;
        ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData));
        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData) * 2);
        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData) * 3);
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(SampleData) * 3));

        uint8_t readData[sizeof(SampleData)];
        EXPECT_EQ(buffer.Read(readData, sizeof(readData)), sizeof(readData));
        for (uint8_t i = 0 ; i < sizeof(readData); i++) {
            EXPECT_EQ(readData[i], SampleData[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(SampleData) * 2));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData) * 2);
        // Read next data
        EXPECT_EQ(buffer.Read(readData, sizeof(readData)), sizeof(readData));
        for (uint8_t i = 0 ; i < sizeof(readData); i++) {
            EXPECT_EQ(readData[i], SampleData[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(SampleData)));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData));
        // Read remaining
        EXPECT_EQ(buffer.Read(readData, sizeof(readData)), sizeof(readData));
        for (uint8_t i = 0 ; i < sizeof(readData); i++) {
            EXPECT_EQ(readData[i], SampleData[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize);
        EXPECT_EQ(buffer.Used(), 0u);

        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Peek)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 50;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;
       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        char test1[] = "Hello";
        buffer.Write(reinterpret_cast<const uint8_t*>(test1), sizeof(test1));
        EXPECT_EQ(buffer.Used(), sizeof(test1));
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1)));

        char test2[] = "Haihi";
        buffer.Write(reinterpret_cast<const uint8_t*>(test2), sizeof(test2));
        EXPECT_EQ(buffer.Used(), sizeof(test1) + sizeof(test2));
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1) + sizeof(test2)));

        char test3[] = "testing";
        buffer.Write(reinterpret_cast<const uint8_t*>(test2), sizeof(test3));
        EXPECT_EQ(buffer.Used(), sizeof(test1) + sizeof(test2) + sizeof(test3));
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1) + sizeof(test2) + sizeof(test3)));

        uint8_t peekData[sizeof(test1)];
        EXPECT_EQ(buffer.Peek(peekData, sizeof(peekData)), sizeof(peekData));
        for (uint8_t i = 0 ; i < sizeof(peekData); i++) {
            EXPECT_EQ(peekData[i], test1[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1) + sizeof(test2) + sizeof(test3)));
        EXPECT_EQ(buffer.Used(), sizeof(test1) + sizeof(test2) + sizeof(test3));
        // Try next peek
        EXPECT_EQ(buffer.Peek(peekData, sizeof(peekData)), sizeof(peekData));
        for (uint8_t i = 0 ; i < sizeof(peekData); i++) {
            EXPECT_EQ(peekData[i], test1[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1) + sizeof(test2) + sizeof(test3)));
        EXPECT_EQ(buffer.Used(), sizeof(test1) + sizeof(test2) + sizeof(test3));
        // Peek again
        EXPECT_EQ(buffer.Peek(peekData, sizeof(peekData)), sizeof(peekData));
        for (uint8_t i = 0 ; i < sizeof(peekData); i++) {
            EXPECT_EQ(peekData[i], test1[i]);
        }
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1) + sizeof(test2) + sizeof(test3)));
        EXPECT_EQ(buffer.Used(), sizeof(test1) + sizeof(test2) + sizeof(test3));

        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Flush)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 10;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;
       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        char test1[] = "HelloHai";
        // Try Flush full data
        buffer.Write(reinterpret_cast<const uint8_t*>(test1), sizeof(test1));
        EXPECT_EQ(buffer.Used(), sizeof(test1));
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(test1)));
        buffer.Flush();
        EXPECT_EQ(buffer.Used(), 0u);
        EXPECT_EQ(buffer.Free(), cyclicBufferSize);

        // Try Flush after single data
        buffer.Write(reinterpret_cast<const uint8_t*>(test1), 1u);
        EXPECT_EQ(buffer.Used(), 1u);
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - 1u);
        buffer.Flush();
        EXPECT_EQ(buffer.Used(), 0u);
        EXPECT_EQ(buffer.Free(), cyclicBufferSize);

        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Reserve)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 50;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;
       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        EXPECT_EQ(buffer.Reserve(sizeof(SampleData)), sizeof(SampleData));
        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        EXPECT_EQ(buffer.Used(), sizeof(SampleData));
        EXPECT_EQ(buffer.Free(), cyclicBufferSize - (sizeof(SampleData)));

        EXPECT_EQ(buffer.Reserve(0), 0u);
        EXPECT_EQ(buffer.Reserve(51), ::Thunder::Core::ERROR_INVALID_INPUT_LENGTH);
        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Using_DataElementFile)
    {
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            ::Thunder::Core::File::CREATE | ::Thunder::Core::File::Mode::SHAREABLE;

        {
            std::string fileName {"cyclicbuffer01"};
            uint8_t cyclicBufferSize = 50;
            uint8_t cyclicBufferWithControlDataSize = cyclicBufferSize + sizeof(::Thunder::Core::CyclicBuffer::control);
            ::Thunder::Core::DataElementFile dataElementFile(fileName, mode, cyclicBufferWithControlDataSize);
            // Create::Thunder::Core::CyclicBuffer with Size 50
           ::Thunder::Core::CyclicBuffer buffer(dataElementFile, true, 0, cyclicBufferWithControlDataSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), fileName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);

            // Check File Size
            EXPECT_EQ(FileSize(fileName.c_str()), cyclicBufferWithControlDataSize);

            // Remove after usage before destruction
            buffer.Close();

            // Check File Exist
            EXPECT_EQ(IsFileExist(fileName.c_str()), false);
        }
    }
    TEST(Core_CyclicBuffer, Using_DataElementFile_WithOffset)
    {
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            ::Thunder::Core::File::CREATE | ::Thunder::Core::File::Mode::SHAREABLE;

        {
            std::string fileName {"cyclicbuffer01"};
            uint8_t cyclicBufferSize = 50;
            uint32_t offset = 56;
            uint8_t cyclicBufferWithControlDataSize = cyclicBufferSize + sizeof(::Thunder::Core::CyclicBuffer::control);
            ::Thunder::Core::DataElementFile dataElementFile(fileName, mode, cyclicBufferWithControlDataSize + offset);
            // Create::Thunder::Core::CyclicBuffer with Size 50
           ::Thunder::Core::CyclicBuffer buffer(dataElementFile, true, offset, cyclicBufferWithControlDataSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), fileName.c_str());
            EXPECT_EQ(buffer.Size(), cyclicBufferSize);
            EXPECT_EQ(buffer.IsValid(), true);
            EXPECT_EQ(buffer.Open(), true);

            // Check File Size
            EXPECT_EQ(FileSize(fileName.c_str()), cyclicBufferWithControlDataSize + offset);

            // Remove after usage before destruction
            buffer.Close();

            // Check File Exist
            EXPECT_EQ(IsFileExist(fileName.c_str()), false);
        }
    }
    TEST(Core_CyclicBuffer, Using_DataElementFile_WithInvalidOffset)
    {
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            ::Thunder::Core::File::CREATE | ::Thunder::Core::File::Mode::SHAREABLE;

        {
            std::string fileName {"cyclicbuffer01"};
            uint8_t cyclicBufferSize = 50;
            uint32_t offset = 50;
            uint8_t cyclicBufferWithControlDataSize = cyclicBufferSize + sizeof(::Thunder::Core::CyclicBuffer::control);
            ::Thunder::Core::DataElementFile dataElementFile(fileName, mode, cyclicBufferWithControlDataSize + offset);
            // Create::Thunder::Core::CyclicBuffer with Size 50
           ::Thunder::Core::CyclicBuffer buffer(dataElementFile, true, offset, cyclicBufferWithControlDataSize, false);

            EXPECT_STREQ(buffer.Name().c_str(), fileName.c_str());
            EXPECT_EQ(buffer.IsValid(), false);

            // Check File Size
            EXPECT_EQ(FileSize(fileName.c_str()), cyclicBufferWithControlDataSize + offset);

            // Remove after usage before destruction
            buffer.Close();

            // Check File Exist
            EXPECT_EQ(IsFileExist(fileName.c_str()), false);
        }
    }

    TEST(Core_CyclicBuffer, Write_Overflow_WithoutOverWrite)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 10;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  ;

       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

        EXPECT_EQ(buffer.IsOverwrite(), false);
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);

        uint8_t data = 'X';
        for (uint8_t i = 1; i <= cyclicBufferSize - 1 ; ++i) {
             buffer.Write(&data, 1);
        }
        EXPECT_EQ(buffer.Used(), static_cast<uint32_t>(cyclicBufferSize - 1));
        EXPECT_EQ(buffer.Free(), 1u);

        buffer.Write(&data, 1);
        EXPECT_EQ(buffer.Used(), static_cast<uint32_t>(cyclicBufferSize - 1));

        char testData1[] = "H";
        EXPECT_EQ(buffer.Write(reinterpret_cast<uint8_t*>(&testData1), sizeof(testData1)), 0u);
        EXPECT_EQ(buffer.Overwritten(), false);

        char testData[] = "Hello";
        EXPECT_EQ(buffer.Write(reinterpret_cast<uint8_t*>(&testData), sizeof(testData)), 0u);
        EXPECT_EQ(buffer.Overwritten(), false);

        uint8_t read = 0;
        // Verify data is overwritted
        for (uint8_t i = 0; i < cyclicBufferSize - 1; ++i) {
             EXPECT_EQ(buffer.Read(&read, 1), 1u);
             EXPECT_EQ(read, data);
        }

        // Remove after usage before destruction
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, Write_Overflow_WithOverWrite)
    {
        std::string bufferName {"cyclicbuffer01"};
        const uint8_t cyclicBufferSize = 100;
        const uint32_t mode =
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
            ::Thunder::Core::File::Mode::SHAREABLE;

       CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
        EXPECT_EQ(buffer.Size(), cyclicBufferSize);
        EXPECT_EQ(buffer.IsValid(), true);
        EXPECT_EQ(buffer.IsOverwrite(), true);

        uint8_t data1[90];
        memset(&data1, 'A', sizeof(data1));
        uint16_t size = sizeof(data1) + 2;
        uint8_t reserved = buffer.Reserve(size);
        if (reserved == size) {
            buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            buffer.Write(reinterpret_cast<uint8_t*>(&data1), sizeof(data1));
        }
        EXPECT_EQ(buffer.Overwritten(), false);
        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        uint8_t data2[80];
        memset(&data2, 'B', sizeof(data2));
        size = sizeof(data2) + 2;
        reserved = buffer.Reserve(size);
        if (reserved == size) {
            buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            buffer.Write(reinterpret_cast<uint8_t*>(&data2), sizeof(data2));
        }

        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        char testData[] = "123456789012345678901234567890";
        size = sizeof(testData) + 2;
        EXPECT_EQ(buffer.Reserve(size), size);
        buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
        EXPECT_EQ(buffer.Write(reinterpret_cast<uint8_t*>(&testData), sizeof(testData)), sizeof(testData));

        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        // Try overwrite with size of Free()
        uint32_t count = buffer.Free();
        uint8_t* data3 = new uint8_t[count];
        memset(data3, 'C', count);
        size = count + 2;
        reserved = buffer.Reserve(size);
        if (reserved == size) {
            buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            buffer.Write(data3, count);
        }
        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));
        delete[] data3;

        // Flush to start from beginning
        buffer.Flush();
        size = sizeof(testData) + 2;
        EXPECT_EQ(buffer.Reserve(size), size);
        EXPECT_EQ(buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2), 2u);
        EXPECT_EQ(buffer.Write(reinterpret_cast<uint8_t*>(&testData), sizeof(testData)), sizeof(testData));
        EXPECT_EQ(buffer.Overwritten(), false);

        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        uint16_t read = 0;
        // Verify data is overwritten
        EXPECT_EQ(buffer.Read(reinterpret_cast<uint8_t*>(&read), 2), 2u);
        EXPECT_EQ(read, size);
        for (uint8_t i = 0; i < sizeof(testData); ++i) {
             EXPECT_EQ(buffer.Read(reinterpret_cast<uint8_t*>(&read), 1), 1u);
             EXPECT_EQ(read, testData[i]);
        }

        EXPECT_EQ(buffer.Free(), cyclicBufferSize);
        size = sizeof(data1) + 2;
        reserved = buffer.Reserve(size);
        if (reserved == size) {
            buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            buffer.Write(reinterpret_cast<uint8_t*>(&data1), sizeof(data1));
        }
        EXPECT_EQ(buffer.Overwritten(), false);
        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        // Verify Overwrite without reservation
        size = sizeof(data2) + 2;
        buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
        buffer.Write(reinterpret_cast<uint8_t*>(&data2), sizeof(data2));

        EXPECT_EQ(buffer.Used(), size);
        EXPECT_EQ(buffer.Free(), static_cast<uint32_t>(cyclicBufferSize - size));

        // Remove after usage before destruction
        buffer.Close();
    }
    static int ClonedProcessFunc(void* arg) {
        Data* data = static_cast<Data*>(arg);
        uint32_t cyclicBufferSize = 10;
        uint32_t shareableFlag = (data->shareable == true) ? static_cast<std::underlying_type<::Thunder::Core::File::Mode>::type>(::Thunder::Core::File::Mode::SHAREABLE) : 0;

       ::Thunder::Core::CyclicBuffer buffer(data->bufferName.c_str(),
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  | shareableFlag,
            cyclicBufferSize, false);

        buffer.Write(reinterpret_cast<const uint8_t*>(SampleData), sizeof(SampleData));
        sleep(2);
        return 0;
    }
    void SetSharePermissionsFromClonedProcess(const string& bufferName,  bool shareable)
    {
        const int STACK_SIZE = 65536;
        char* stack = static_cast<char*>(malloc(STACK_SIZE));
        if (!stack) {
            perror("malloc");
            exit(1);
        }

        unsigned long flags = CLONE_PARENT;
        static Data data;
        data.bufferName = bufferName.c_str();
        data.shareable = shareable;

        int pid = 0;
        if ((pid = clone(ClonedProcessFunc, stack + STACK_SIZE, flags, reinterpret_cast<void*>(&data))) == -1) {
            perror("clone");
            exit(1);
        }

        sleep(1);
        int status;
        waitpid(pid, &status, 0);
        free(stack);
    }
    void TestSharePermissionsFromDifferentProcess(bool shareable)
    {
        std::string bufferName {"cyclicbuffer01"};
        SetSharePermissionsFromClonedProcess(bufferName, shareable);

        uint32_t cyclicBufferSize = 0;
        uint32_t shareableFlag = (shareable == true) ? static_cast<std::underlying_type<::Thunder::Core::File::Mode>::type>(::Thunder::Core::File::Mode::SHAREABLE) : 0;

       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(),
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE   |
            shareableFlag, cyclicBufferSize, false);

        EXPECT_EQ(buffer.Used(), ((shareable == true) ? sizeof(SampleData) : 0));
        EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());

        if (shareable == true) {
            EXPECT_NE(buffer.Size(), 0u);
            uint8_t writeData = 'T';
            uint8_t readData = 0;
            EXPECT_EQ(buffer.Read(&readData, 1, true), 1u);
            EXPECT_EQ(readData, 'B');
            EXPECT_EQ(buffer.Write(&writeData, 1), 1u);
        } else {
            EXPECT_EQ(buffer.Size(), 0u);
        }

        buffer.Close();
    }
    TEST(Core_CyclicBuffer, CheckSharePermissionsFromClonedProcess)
    {
        // With Buffer Permission as SHAREABLE
        TestSharePermissionsFromDifferentProcess(true);

        // With Buffer Permission as PRIVATE
        TestSharePermissionsFromDifferentProcess(false);
    }
    void SetSharePermissionsFromForkedProcessAndVerify(bool shareable, bool usingDataElementFile = false, uint32_t offset = 0)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string bufferName {"cyclicbuffer01"};

        constexpr uint32_t CyclicBufferSize = 10;
        constexpr uint32_t cyclicBufferWithControlDataSize = CyclicBufferSize + sizeof(::Thunder::Core::CyclicBuffer::control);

        const uint32_t mode = (  ::Thunder::Core::File::Mode::USER_READ
                               | ::Thunder::Core::File::Mode::USER_WRITE
                               | ::Thunder::Core::File::Mode::USER_EXECUTE
                               | ::Thunder::Core::File::Mode::GROUP_READ
                               | ::Thunder::Core::File::Mode::GROUP_WRITE
                               | (shareable ? static_cast<std::underlying_type<::Thunder::Core::File::Mode>::type>(::Thunder::Core::File::Mode::SHAREABLE) : 0)
                              );

        const struct Data data{shareable, usingDataElementFile, mode, offset, bufferName.c_str()};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the parent can be set up
            SleepMs(maxInitTime);

            uint32_t result;
            uint8_t loadBuffer[CyclicBufferSize + 1];

            CyclicBufferTest* buffer = nullptr;
            ::Thunder::Core::DataElementFile* dataElementFile = nullptr;

            if (data.usingDataElementFile == true) {
                dataElementFile = new ::Thunder::Core::DataElementFile(bufferName, data.mode | ::Thunder::Core::File::CREATE, cyclicBufferWithControlDataSize + data.offset);

                ASSERT_TRUE(dataElementFile != nullptr);

                buffer = new CyclicBufferTest(*dataElementFile, true, data.offset, cyclicBufferWithControlDataSize, false);
            } else {
                buffer = new CyclicBufferTest(bufferName, data.mode, CyclicBufferSize, false);
            }

            ASSERT_TRUE(buffer != nullptr);

            EXPECT_EQ(buffer->Size(), CyclicBufferSize);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            if (data.shareable == true) {
                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                string dataStr = "abcd";
                result = buffer->Write(reinterpret_cast<const uint8_t*>(dataStr.c_str()), dataStr.size());
                EXPECT_EQ(result, dataStr.size());

                dataStr = "efgh";
                result = buffer->Write(reinterpret_cast<const uint8_t*>(dataStr.c_str()), dataStr.size());
                EXPECT_EQ(result, dataStr.size());

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                result = buffer->Peek(loadBuffer, buffer->Used());
                loadBuffer[result] = '\0';
                EXPECT_EQ(result, 9u);

                EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "bcdefghkl");
                EXPECT_EQ(buffer->Used(), 9u);

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                result = buffer->Read(loadBuffer, buffer->Used());
                loadBuffer[result] = '\0';

                EXPECT_EQ(result, 9u);
                EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "bcdefghkl");

                EXPECT_EQ(buffer->Used(), 0u);

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                delete buffer;

                if (dataElementFile) {
                    delete dataElementFile;
                }
            }
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint32_t result;
            uint8_t loadBuffer[CyclicBufferSize];

            CyclicBufferTest* buffer = nullptr;
            ::Thunder::Core::DataElementFile* dataElementFile = nullptr;

            if (usingDataElementFile == true) {
                dataElementFile = new ::Thunder::Core::DataElementFile(bufferName, mode, 0);

                ASSERT_TRUE(dataElementFile != nullptr);

                buffer = new CyclicBufferTest(*dataElementFile, false, offset, 0, false);
            } else {
                buffer = new CyclicBufferTest(bufferName, mode, 0, false);
            }

            ASSERT_TRUE(buffer != nullptr);

            EXPECT_EQ(buffer->Size(), static_cast<uint32_t>((shareable ? CyclicBufferSize : 0)));

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            if (shareable == true) {
                memset(loadBuffer, 0, CyclicBufferSize);

                result = buffer->Read(loadBuffer, 1, false);
                EXPECT_EQ(result, 0);

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                result = buffer->Read(loadBuffer, 1, false);
                EXPECT_EQ(result, 1);

                loadBuffer[result] = '\0';
                EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "a");

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                string data = "kl";
                result = buffer->Reserve(data.size());
                EXPECT_EQ(result, 2u);

                result = buffer->Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
                EXPECT_EQ(result, 2u);

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                EXPECT_EQ(buffer->Used(), 9u);

                ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

                ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

                EXPECT_EQ(buffer->Used(), 0u);
            }

            buffer->Close();

            delete buffer;

            if (dataElementFile) {
                delete dataElementFile;
            }

        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
    TEST(Core_CyclicBuffer, CheckSharePermissionsFromForkedProcessWithoutOverwrite)
    {
        SetSharePermissionsFromForkedProcessAndVerify(true);
        SetSharePermissionsFromForkedProcessAndVerify(false);
    }
    TEST(Core_CyclicBuffer, CheckSharePermissionsFromForkedProcessWithoutOverwrite_UsingDataElementFile)
    {
        SetSharePermissionsFromForkedProcessAndVerify(true, true);
        SetSharePermissionsFromForkedProcessAndVerify(false, true);
    }
    TEST(Core_CyclicBuffer, CheckSharePermissionsFromForkedProcessWithoutOverwrite_UsingDataElementFile_WithOffset)
    {
        SetSharePermissionsFromForkedProcessAndVerify(true, true, 56);
        SetSharePermissionsFromForkedProcessAndVerify(false, true, 56);
    }
    TEST(Core_CyclicBuffer, WithoutOverwriteUsingForksReversed)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string bufferName {"cyclicbuffer02"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            constexpr uint32_t cyclicBufferSize = 0;

            constexpr uint32_t mode =   ::Thunder::Core::File::Mode::USER_READ
                                      | ::Thunder::Core::File::Mode::USER_WRITE
                                      | ::Thunder::Core::File::Mode::USER_EXECUTE
                                      | ::Thunder::Core::File::Mode::GROUP_READ
                                      | ::Thunder::Core::File::Mode::GROUP_WRITE
                                      | ::Thunder::Core::File::Mode::SHAREABLE
                                      ;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            ASSERT_TRUE(buffer.IsValid());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint8_t loadBuffer[10 + 1];

            uint32_t result = buffer.Read(loadBuffer, 4);
            loadBuffer[result] = '\0';

            EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "abcd");

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
       
            const string data = "klmnopq";

            result = buffer.Reserve(data.size());
            EXPECT_EQ(result, ::Thunder::Core::ERROR_INVALID_INPUT_LENGTH);
        
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, 0u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_FALSE(buffer.Overwritten());
            EXPECT_FALSE(buffer.IsLocked());

            EXPECT_EQ(buffer.LockPid(), 0u);
            EXPECT_EQ(buffer.Free(), 5u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_STREQ(buffer.Name().c_str(), bufferName.c_str());
            EXPECT_STREQ(buffer.Storage().Name().c_str(), bufferName.c_str());

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buffer.Used(), 0u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            constexpr uint32_t cyclicBufferSize = 10;

            constexpr uint32_t mode =   ::Thunder::Core::File::Mode::USER_READ
                                      | ::Thunder::Core::File::Mode::USER_WRITE
                                      | ::Thunder::Core::File::Mode::USER_EXECUTE 
                                      | ::Thunder::Core::File::Mode::GROUP_READ
                                      | ::Thunder::Core::File::Mode::GROUP_WRITE
                                      | ::Thunder::Core::File::Mode::SHAREABLE
                                      ;

            ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

            ASSERT_TRUE(buffer.IsValid());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint8_t loadBuffer[cyclicBufferSize + 1];

            const string data = "abcdefghi";

            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            result = buffer.Peek(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';

            EXPECT_EQ(result, 5u);
            EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "efghi");

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            result = buffer.Read(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';

            EXPECT_EQ(result, 5u);
            EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "efghi");

            EXPECT_EQ(buffer.Used(), 0u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
    TEST(Core_CyclicBuffer, WithOverWriteUsingFork)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string bufferName {"cyclicbuffer03"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the parent can be set up
            SleepMs(maxInitTime);

            constexpr uint32_t cyclicBufferSize = 10;

            constexpr uint32_t mode =   ::Thunder::Core::File::Mode::USER_READ
                                      | ::Thunder::Core::File::Mode::USER_WRITE
                                      | ::Thunder::Core::File::Mode::USER_EXECUTE
                                      | ::Thunder::Core::File::Mode::GROUP_READ
                                      | ::Thunder::Core::File::Mode::GROUP_WRITE
                                      | ::Thunder::Core::File::Mode::SHAREABLE
                                      ;

            CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);
 
            ASSERT_TRUE(buffer.IsValid());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            const string data = "abcdef";
            const uint16_t size = data.size() + 2;

            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2u);
            EXPECT_EQ(result, 2u);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint8_t loadBuffer[cyclicBufferSize + 1];

            result = buffer.Peek(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            result = buffer.Read(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            buffer.Alert();
            buffer.Flush();

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            constexpr uint32_t cyclicBufferSize = 0;

            constexpr uint32_t mode =  ::Thunder::Core::File::Mode::USER_READ
                                     | ::Thunder::Core::File::Mode::USER_WRITE
                                     | ::Thunder::Core::File::Mode::USER_EXECUTE
                                     | ::Thunder::Core::File::Mode::GROUP_READ
                                     | ::Thunder::Core::File::Mode::GROUP_WRITE
                                     | ::Thunder::Core::File::Mode::SHAREABLE
                                     ;

            CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);

            ASSERT_TRUE(buffer.IsValid());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            string data = "j";
            const uint16_t size = 9;

            uint32_t result = buffer.Reserve(9);
            EXPECT_EQ(result, 9u);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2u);
            EXPECT_EQ(result, 2u);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), 1u);
            EXPECT_EQ(result, data.size());

            data = "klmnop";

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), 6u);
            EXPECT_EQ(result, data.size());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
    TEST(Core_CyclicBuffer, WithOverwriteUsingForksReversed)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string bufferName {"cyclicbuffer03"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            constexpr uint32_t cyclicBufferSize = 0;

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            constexpr uint32_t mode =   ::Thunder::Core::File::Mode::USER_READ
                                      | ::Thunder::Core::File::Mode::USER_WRITE
                                      | ::Thunder::Core::File::Mode::USER_EXECUTE
                                      | ::Thunder::Core::File::Mode::GROUP_READ
                                      | ::Thunder::Core::File::Mode::GROUP_WRITE
                                      | ::Thunder::Core::File::Mode::SHAREABLE
                                      ;

            CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);
            EXPECT_TRUE(buffer.IsOverwrite());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint16_t size;
            EXPECT_EQ(buffer.Read(reinterpret_cast<uint8_t*>(&size), 2), 2);

            uint8_t* loadBuffer = new uint8_t[size + 1];

            ASSERT_GE(size, 2);

            uint32_t result = buffer.Read(loadBuffer, size - 2);
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, size - 2);

            delete[] loadBuffer;

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            string data = "j";
            size = 9;

            result = buffer.Reserve(size);
            EXPECT_EQ(result, size);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            EXPECT_EQ(result, 2);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), 1);
            EXPECT_EQ(result, 1);

            data = "lmnopq";
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), 6);
            EXPECT_EQ(result, 6);

            EXPECT_EQ(buffer.Used(), 9u);
            EXPECT_EQ(buffer.Overwritten(), false);

            size = 7;
            result = buffer.Reserve(size);
            EXPECT_EQ(result, size);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);
            EXPECT_EQ(result, 2);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), size - 2);
            EXPECT_EQ(result, size - 2);

            EXPECT_EQ(buffer.Free(), 3u);
            EXPECT_EQ(buffer.Used(), 7u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            buffer.Flush();
            EXPECT_EQ(buffer.Used(), 0u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            constexpr uint32_t cyclicBufferSize = 10;

            constexpr uint32_t mode =   ::Thunder::Core::File::Mode::USER_READ
                                      | ::Thunder::Core::File::Mode::USER_WRITE
                                      | ::Thunder::Core::File::Mode::USER_EXECUTE
                                      | ::Thunder::Core::File::Mode::GROUP_READ
                                      | ::Thunder::Core::File::Mode::GROUP_WRITE
                                      | ::Thunder::Core::File::Mode::SHAREABLE
                                      ;

            CyclicBufferTest buffer(bufferName.c_str(), mode, cyclicBufferSize, true);

            EXPECT_TRUE(buffer.IsOverwrite());
            EXPECT_TRUE(buffer.IsValid());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint8_t loadBuffer[cyclicBufferSize + 1];

            uint16_t size = 9;
            string data = "abcdefi";
            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(&size), 2);

            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), size - 2);
            EXPECT_EQ(result, data.size());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            buffer.Peek(reinterpret_cast<uint8_t*>(&size), 2);
            EXPECT_EQ(size, buffer.Used());

            result = buffer.Peek(loadBuffer, size);
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 7u);
            EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0] + 2), "lmnop");

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buffer.Free(), 3u);

            buffer.Read(reinterpret_cast<uint8_t*>(&size), 2);
            EXPECT_EQ(buffer.Used(), static_cast<uint32_t>(size - 2));

            result = buffer.Read(loadBuffer, size - 2);
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 5u);
            EXPECT_STREQ(reinterpret_cast<const char*>(&loadBuffer[0]), "lmnop");

            EXPECT_EQ(buffer.Free(), 10u);
            EXPECT_EQ(buffer.Used(), 0u);

            buffer.Alert();
            EXPECT_FALSE(buffer.IsLocked());
            EXPECT_EQ(buffer.LockPid(), 0u);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(buffer.Free(), cyclicBufferSize);

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
    TEST(Core_CyclicBuffer, LockUnlock_WithoutDataPresent)
    {
        string bufferName = "cyclicbuffer01";
        uint32_t cyclicBufferSize = 10;

       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(),
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
            ::Thunder::Core::File::Mode::SHAREABLE, cyclicBufferSize, false);

        buffer.Lock();
        EXPECT_EQ(buffer.IsLocked(), true);
        buffer.Unlock();
        EXPECT_EQ(buffer.IsLocked(), false);
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, LockAlert_WithoutDataPresent)
    {
        string bufferName = "cyclicbuffer01";
        uint32_t cyclicBufferSize = 10;

       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(),
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
            ::Thunder::Core::File::Mode::SHAREABLE, cyclicBufferSize, false);

        buffer.Lock();
        EXPECT_EQ(buffer.IsLocked(), true);
        buffer.Alert(); // Alert no functioning on DataPeresent == false case
        EXPECT_EQ(buffer.IsLocked(), true);
        buffer.Unlock();
        EXPECT_EQ(buffer.IsLocked(), false);
        buffer.Close();
    }
    
    TEST(Core_CyclicBuffer, DISABLED_LockUnLock_FromParentAndForks)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        VARIABLE_IS_NOT_USED constexpr uint8_t maxRetries = 1;
 
        std::string bufferName {"cyclicbuffer04"};

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            uint32_t cyclicBufferSize = 20;

            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

//            testAdmin.Sync("setup server");
//            testAdmin.Sync("setup client");

            EXPECT_EQ(buffer.LockPid(), 0u);
            buffer.Lock(false);
            EXPECT_EQ(buffer.LockPid(), static_cast<uint32_t>(getpid()));
//            testAdmin.Sync("server locked");

//            testAdmin.Sync("client wrote");
            buffer.Unlock();
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("server unlocked");

//            testAdmin.Sync("client locked");
            EXPECT_NE(buffer.LockPid(), 0u);
            EXPECT_NE(buffer.LockPid(), static_cast<uint32_t>(getpid()));
//            testAdmin.Sync("server verified");

            // TODO: What is the purpose of lock ?? since we are able to write from server process
            // even it is locked from client process
            string data = "abcdefghi";
            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());
            EXPECT_EQ(buffer.Used(), data.size() * 2);
            uint8_t* loadBuffer = new uint8_t[cyclicBufferSize + 1];
            result = buffer.Peek(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';
            delete[] loadBuffer;
//            testAdmin.Sync("server wrote and peeked");

//            testAdmin.Sync("client unlocked");
            EXPECT_EQ(buffer.LockPid(), 0u);

//            testAdmin.Sync("client read");
        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

//            testAdmin.Sync("setup server");

            uint32_t cyclicBufferSize = 0;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, true);

            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("setup client");
//            testAdmin.Sync("server locked");
            EXPECT_NE(buffer.LockPid(), 0u);
            EXPECT_NE(buffer.LockPid(), static_cast<uint32_t>(getpid()));

            // TODO: What is the purpose of lock ?? since we are able to write from client process
            // even it is locked from server process
            string data = "jklmnopqr";
            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            EXPECT_NE(buffer.LockPid(), 0u);
            EXPECT_NE(buffer.LockPid(), static_cast<uint32_t>(getpid()));
//            testAdmin.Sync("client wrote");
//            testAdmin.Sync("server unlocked");
            EXPECT_EQ(buffer.LockPid(), 0u);
            buffer.Lock(false);
            EXPECT_EQ(buffer.LockPid(), static_cast<uint32_t>(getpid()));

//            testAdmin.Sync("client locked");
//            testAdmin.Sync("server verified");

//            testAdmin.Sync("server wrote and peeked");
            buffer.Unlock();
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("client unlocked");

            uint8_t* loadBuffer = new uint8_t[cyclicBufferSize + 1];
            result = buffer.Read(loadBuffer, 4);
            loadBuffer[result] = '\0';
            EXPECT_STREQ((char*)loadBuffer, "jklm");
            delete[] loadBuffer;

//            testAdmin.Sync("client read");

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
//TODO: revisit these test cases after fixing the issues with cyclicbuffer lock/unlock sequence
    TEST(Core_CyclicBuffer, DISABLED_LockUnlock_FromParentAndForks_WithDataPresent)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, VARIABLE_IS_NOT_USED maxInitTime = 2000;
        VARIABLE_IS_NOT_USED constexpr uint8_t maxRetries = 1;

        std::string bufferName {"cyclicbuffer05"};

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            uint32_t cyclicBufferSize = 20;

            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

//            testAdmin.Sync("setup server");
//            testAdmin.Sync("setup client");

            EXPECT_EQ(buffer.LockPid(), 0u);
            buffer.Lock(true, 100);
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("server timedLock");
            EXPECT_EQ(buffer.IsLocked(), false);

            {
                uint8_t data = 1;
                uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(&data), 1);
                EXPECT_EQ(result, 1u);
                EXPECT_EQ(buffer.IsLocked(), false);
                EXPECT_NE(buffer.Used(), 0u);

                ::Thunder::Core::Event event(false, false);
                ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
                threadLock.Run();

                // Lock before requesting cyclic buffer lock
                if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
                    // Seems thread is not active so re-request lock
                    event.ResetEvent();
                    event.Lock(MaxSignalWaitTime);
                }

                // Lock after requesting cyclic buffer lock
                event.Lock(MaxSignalWaitTime);
                event.ResetEvent();
                threadLock.Stop();
                EXPECT_EQ(buffer.IsLocked(), true);
                buffer.Unlock();
            }
//            testAdmin.Sync("server locked & unlocked");

            buffer.Flush();
            EXPECT_EQ(buffer.Used(), 0u);

            EXPECT_EQ(buffer.IsLocked(), false);
            ::Thunder::Core::Event event(false, false);
            ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
            threadLock.Run();
            EXPECT_EQ(buffer.IsLocked(), false);

            // Lock before requesting cyclic buffer lock
            if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
                // Seems thread is not active so re-request lock
                event.ResetEvent();
                event.Lock(MaxSignalWaitTime);
            }

            sleep(1);
//            testAdmin.Sync("server locked");
            EXPECT_EQ(buffer.LockPid(), 0u);
            EXPECT_EQ(buffer.IsLocked(), false);

            string data = "abcdefghi";
            EXPECT_EQ(buffer.IsLocked(), false);
            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(buffer.IsLocked(), true);
            EXPECT_EQ(result, data.size());
            EXPECT_EQ(buffer.LockPid(), static_cast<uint32_t>(getpid()));

            // Lock after requesting cyclic buffer lock
            event.Lock(MaxSignalWaitTime);
            event.ResetEvent();
            threadLock.Stop();

//            testAdmin.Sync("server locked & wrote");

            buffer.Unlock();
//            testAdmin.Sync("server unlocked");
//            testAdmin.Sync("client wrote & locked");

//            testAdmin.Sync("client exit");
            EXPECT_EQ(buffer.LockPid(), 0u);
        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
//            testAdmin.Sync("setup server");

            uint32_t cyclicBufferSize = 0;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, true);
//           testAdmin.Sync("setup client");

//            testAdmin.Sync("server timedLock");

//            testAdmin.Sync("server locked & unlocked");
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("server locked");
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("server locked & wrote");
            EXPECT_NE(buffer.LockPid(), 0u);
//            testAdmin.Sync("server unlocked");
            EXPECT_EQ(buffer.LockPid(), 0u);

            // Check Lock Timed Out after wait Time
            buffer.Flush();
            const uint32_t waitTime = 1500;
            struct timespec timeOutTime = GetCurrentTime();
            buffer.Lock(true, waitTime);
            EXPECT_EQ(CheckTimeOutIsExpired(timeOutTime, waitTime), true);
            EXPECT_EQ(buffer.IsLocked(), false);

            ::Thunder::Core::Event event(false, false);
            ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
            threadLock.Run();

            // Lock before requesting cyclic buffer lock
            if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
                // Seems thread is not active so re-request lock
                event.ResetEvent();
                event.Lock(MaxSignalWaitTime);
            }
            EXPECT_EQ(buffer.LockPid(), 0u);

            string data = "jklmnopqr";
            uint32_t result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());
            EXPECT_EQ(buffer.IsLocked(), true);
            // Lock after requesting cyclic buffer lock
            event.Lock(MaxSignalWaitTime * 2);
            event.ResetEvent();
            threadLock.Stop();

            EXPECT_EQ(buffer.LockPid(), static_cast<uint32_t>(getpid()));
//            testAdmin.Sync("client wrote & locked");
            buffer.Unlock();
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("client exit");
            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }
    TEST(Core_CyclicBuffer, DISABLED_LockUnlock_UsingAlert)
    {
        string bufferName = "cyclicbuffer01";
        uint32_t cyclicBufferSize = 10;

       ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(),
            ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
            ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
            ::Thunder::Core::File::Mode::SHAREABLE, cyclicBufferSize, false);

        ::Thunder::Core::Event event(false, false);
        ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
        threadLock.Run();

        // Lock before requesting cyclic buffer lock
        if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
            // Seems thread is not active so re-request lock
            event.ResetEvent();
            event.Lock(MaxSignalWaitTime);
        }
        event.ResetEvent();

        EXPECT_EQ(buffer.IsLocked(), false);

        buffer.Alert();

        event.Lock(MaxSignalWaitTime);
        event.ResetEvent();
        EXPECT_EQ(buffer.IsLocked(), false);
        threadLock.Stop();
        buffer.Close();
    }
    TEST(Core_CyclicBuffer, DISABLED_LockUnlock_FromParentAndForks_UsingAlert)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, VARIABLE_IS_NOT_USED maxWaitTimeMs = 4000, maxInitTime = 2000;
        VARIABLE_IS_NOT_USED constexpr uint8_t maxRetries = 1;

        std::string bufferName {"cyclicbuffer05"};

        IPTestAdministrator::Callback callback_child = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            uint32_t cyclicBufferSize = 20;

            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, false);

//            testAdmin.Sync("setup server");
//            testAdmin.Sync("setup client");

            EXPECT_EQ(buffer.LockPid(), 0u);
            ::Thunder::Core::Event event(false, false);
            ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
            threadLock.Run();
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("server locked");

            // Lock before requesting cyclic buffer lock
            if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
                // Seems thread is not active so re-request lock
                event.ResetEvent();
                event.Lock(MaxSignalWaitTime);
            }
            event.ResetEvent();

            buffer.Alert();

            event.Lock(MaxSignalWaitTime * 2);
            event.ResetEvent();
            threadLock.Stop();

//            testAdmin.Sync("server alerted");
//            testAdmin.Sync("client locked");
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("client alerted");
            EXPECT_EQ(buffer.LockPid(), 0u);
        };

        IPTestAdministrator::Callback callback_parent = [&](VARIABLE_IS_NOT_USED IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

//            testAdmin.Sync("setup server");

            uint32_t cyclicBufferSize = 0;
            const uint32_t mode =
                ::Thunder::Core::File::Mode::USER_READ | ::Thunder::Core::File::Mode::USER_WRITE | ::Thunder::Core::File::Mode::USER_EXECUTE |
                ::Thunder::Core::File::Mode::GROUP_READ | ::Thunder::Core::File::Mode::GROUP_WRITE  |
                ::Thunder::Core::File::Mode::SHAREABLE;

           ::Thunder::Core::CyclicBuffer buffer(bufferName.c_str(), mode, cyclicBufferSize, true);

//            testAdmin.Sync("setup client");
//            testAdmin.Sync("server locked");

            EXPECT_EQ(buffer.LockPid(), 0u);

//            testAdmin.Sync("server alerted");
            EXPECT_EQ(buffer.LockPid(), 0u);

            ::Thunder::Core::Event event(false, false);
            ThreadLock threadLock(buffer, ::Thunder::Core::infinite, event);
            threadLock.Run();
            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("client locked");

            // Lock before requesting cyclic buffer lock
            if (event.Lock(MaxSignalWaitTime * 2) == ::Thunder::Core::ERROR_NONE) {
                // Seems thread is not active so re-request lock
                event.ResetEvent();
                event.Lock(MaxSignalWaitTime);
            }
            event.ResetEvent();

            buffer.Alert();

            event.Lock(MaxSignalWaitTime * 2);
            event.ResetEvent();
            threadLock.Stop();

            EXPECT_EQ(buffer.LockPid(), 0u);
//            testAdmin.Sync("client alerted");

            buffer.Close();
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
