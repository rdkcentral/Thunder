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

#include <fstream>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <messaging/messaging.h>

#include "../IPTestAdministrator.h"

namespace Thunder {

namespace Tests {
namespace Core {

    class Core_MessageDispatcher : public testing::Test {
    protected:
        static constexpr uint16_t METADATA_SIZE = 1 * 1024;
        static constexpr uint16_t DATA_SIZE = 9 * 1024;

        Core_MessageDispatcher()
            : _dispatcher(nullptr)
            , _identifier(_T("md"))
            , _basePath(_T("/tmp/TestMessageDispatcher"))
        {
            //if directory exists remove it to clear data (eg. sockets) that can remain after previous run
            if (::Thunder::Core::File(_basePath).IsDirectory()) {
                ::Thunder::Core::Directory(_basePath.c_str()).Destroy();
            }
            //create directory
            if (!::Thunder::Core::Directory(_basePath.c_str()).CreatePath()) {
                std::cerr << "Unable to create MessageDispatcher directory" << std::endl;
            }
        }

        ~Core_MessageDispatcher()
        {
            if (::Thunder::Core::File(_basePath).IsDirectory()) {
                ::Thunder::Core::Directory(_basePath.c_str()).Destroy();
            }
        }

        void SetUp() override
        {
            _dispatcher.reset(new ::Thunder::Messaging::MessageDataBuffer(_identifier, _instanceId, _basePath, DATA_SIZE, 0, true));
        }
        void TearDown() override
        {
            _dispatcher.reset(nullptr);

            ++_instanceId;

            ::Thunder::Core::Singleton::Dispose();
        }

        std::unique_ptr<::Thunder::Messaging::MessageDataBuffer> _dispatcher;
        string _identifier;
        string _basePath;

        static int _instanceId;
    };

    int Core_MessageDispatcher::_instanceId = 0;

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInSameProcess)
    {
        //arrange
        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        //act
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        //assert
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, ReadDataIsCutIfCannotFitIntoBuffer)
    {
        //arrange
        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[1];
        uint16_t readLength = sizeof(readData);

        //act
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_GENERAL);

        //assert
        ASSERT_EQ(readLength, 2);
        ASSERT_EQ(readData[0], 13);
    }

    TEST_F(Core_MessageDispatcher, CreateAndOpenOperatesOnSameValidFile)
    {
        ::Thunder::Messaging::MessageDataBuffer writerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, true);
        ::Thunder::Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(writerDispatcher.PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(readerDispatcher.PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, MessageDispatcherCanBeOpenedAndClosed)
    {
        ::Thunder::Messaging::MessageDataBuffer writerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, true);

        {
            ::Thunder::Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

            //destructor is called
        }

        //reopen
        ::Thunder::Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(writerDispatcher.PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(readerDispatcher.PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInSameProcessTwice)
    {
        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        //first read, write, assert
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);

        //second read, write, assert
        testData[0] = 40;
        readLength = sizeof(readData);
        ASSERT_EQ(_dispatcher->PushData(1, testData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(readLength, 1);
        ASSERT_EQ(readData[0], 40);
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInDifferentProcesses)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Messaging::MessageDataBuffer dispatcher(this->_identifier, this->_instanceId, this->_basePath, DATA_SIZE, 0, false);

            uint8_t readData[4];
            uint16_t readLength = sizeof(readData);

            ASSERT_EQ(dispatcher.Wait(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(dispatcher.PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(readLength, 2);
            ASSERT_EQ(readData[0], 13);
            ASSERT_EQ(readData[1], 37);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        // This side (tested) acts as writer
        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            uint8_t testData[2] = { 13, 37 };
            ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST_F(Core_MessageDispatcher, PushDataShouldNotFitWhenExcedingDataBufferSize)
    {
        uint8_t fullBufferSimulation[DATA_SIZE + 1
            + sizeof(::Thunder::Core::CyclicBuffer::control)];

        ASSERT_EQ(_dispatcher->PushData(sizeof(fullBufferSimulation), fullBufferSimulation), ::Thunder::Core::ERROR_WRITE_ERROR);
    }

    TEST_F(Core_MessageDispatcher, OffsetCutPushDataShouldFlushOldDataIfDoesNotFitOffsetCut)
    {
        // CyclicBuffer Reserve requires 'length' < Size()
        uint8_t fullBufferSimulation[DATA_SIZE - sizeof(uint16_t) - 1] = {}; // sizeof(length) + length - 1, eg. < Size()
        uint8_t testData[] = { 12, 11, 13, 21 };
        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        EXPECT_EQ(_dispatcher->PushData(sizeof(fullBufferSimulation), fullBufferSimulation), ::Thunder::Core::ERROR_NONE);
        // One element free space remaining
        // 2+2 bytes, 1 at tail position, 3 starting at position 0

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        // new data written, so the oldest data should be replaced

        // this is first entry and should be first popped (FIFO)
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(readLength, sizeof(testData));
        EXPECT_EQ(readData[0], testData[0]);
        EXPECT_EQ(readData[3], testData[3]);
    }

    TEST_F(Core_MessageDispatcher, OnlyOffsetFitsPushDataShouldFlushOldDataIfDoesNotFit)
    {
        uint8_t onlyOffsetFitsSimulation[DATA_SIZE - sizeof(uint16_t) - 2] = {};
        uint8_t testData[] = { 12, 11, 13, 21 };
        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        EXPECT_EQ(_dispatcher->PushData(sizeof(onlyOffsetFitsSimulation), onlyOffsetFitsSimulation), ::Thunder::Core::ERROR_NONE);
        // Two elements free space remaining so offset of testData should fit at the end of the cyclic buffer,
        // and content of testData buffer should be at the beginning of the cyclic buffer

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(readLength, sizeof(testData));
        EXPECT_EQ(readData[0], testData[0]);
        EXPECT_EQ(readData[3], testData[3]);
    }

    TEST_F(Core_MessageDispatcher, OffsetFitsBufferCutPushDataShouldFlushOldDataIfDoesNotFit)
    {
        uint8_t offsetPlusFitsSimulation[DATA_SIZE - sizeof(uint16_t) - 3] = {};
        uint8_t testData[] = { 12, 11, 13, 21 };
        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        EXPECT_EQ(_dispatcher->PushData(sizeof(offsetPlusFitsSimulation), offsetPlusFitsSimulation), ::Thunder::Core::ERROR_NONE);
        // Three elements free space remaining, so the offset of testData should still fit at the end of the cyclic buffer,
        // as well as the first part of the testData buffer, but its second part should be at the beginning of the cyclic buffer

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(readLength, sizeof(testData));
        EXPECT_EQ(readData[0], testData[0]);
        EXPECT_EQ(readData[3], testData[3]);
    }

    TEST_F(Core_MessageDispatcher, BufferGetsFilledToItsVeryMaximum)
    {
        uint8_t almostFullBufferSimulation[DATA_SIZE - sizeof(uint16_t) - 6] = {};
        uint8_t testData1[] = { 12, 11, 13, 21 };
        uint8_t testData2[] = { 54, 62, 78, 91 };
        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        EXPECT_EQ(_dispatcher->PushData(sizeof(almostFullBufferSimulation), almostFullBufferSimulation), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData1), testData1), ::Thunder::Core::ERROR_NONE);
        // The cyclic buffer is now full

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData2), testData2), ::Thunder::Core::ERROR_NONE);
        // The cyclic buffer needs to flush the almostFullBufferSimulation to make space for testData2

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readLength, sizeof(testData1));
        EXPECT_EQ(readData[0], testData1[0]);
        EXPECT_EQ(readData[3], testData1[3]);

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readLength, sizeof(testData2));
        EXPECT_EQ(readData[0], testData2[0]);
        EXPECT_EQ(readData[3], testData2[3]);
    }

    // =========================================================================
    // Gap 10: Cross-Process Messaging E2E — Buffer-level coverage
    // =========================================================================

    // High volume: rapid sequential push/pop — no data loss
    TEST_F(Core_MessageDispatcher, HighVolume_RapidPushPop)
    {
        constexpr int kIterations = 100;

        for (int i = 0; i < kIterations; i++) {
            uint8_t writeData[4] = {
                static_cast<uint8_t>(i & 0xFF),
                static_cast<uint8_t>((i >> 8) & 0xFF),
                static_cast<uint8_t>(42),
                static_cast<uint8_t>(i % 7)
            };
            uint8_t readData[4] = {};
            uint16_t readLength = sizeof(readData);

            ASSERT_EQ(_dispatcher->PushData(sizeof(writeData), writeData), ::Thunder::Core::ERROR_NONE)
                << "PushData failed at iteration " << i;
            ASSERT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE)
                << "PopData failed at iteration " << i;

            EXPECT_EQ(readLength, sizeof(writeData));
            EXPECT_EQ(readData[0], writeData[0]);
            EXPECT_EQ(readData[1], writeData[1]);
            EXPECT_EQ(readData[2], writeData[2]);
            EXPECT_EQ(readData[3], writeData[3]);
        }
    }

    // Multiple messages queued before reading
    TEST_F(Core_MessageDispatcher, MultipleMessages_QueuedThenRead)
    {
        uint8_t msg1[] = { 0xAA, 0xBB };
        uint8_t msg2[] = { 0xCC, 0xDD };
        uint8_t msg3[] = { 0xEE, 0xFF };

        ASSERT_EQ(_dispatcher->PushData(sizeof(msg1), msg1), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PushData(sizeof(msg2), msg2), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PushData(sizeof(msg3), msg3), ::Thunder::Core::ERROR_NONE);

        uint8_t readData[2] = {};
        uint16_t readLength = sizeof(readData);

        // Read in order
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readData[0], msg1[0]);
        EXPECT_EQ(readData[1], msg1[1]);

        readLength = sizeof(readData);
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readData[0], msg2[0]);
        EXPECT_EQ(readData[1], msg2[1]);

        readLength = sizeof(readData);
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readData[0], msg3[0]);
        EXPECT_EQ(readData[1], msg3[1]);
    }

    // Variable-length messages
    TEST_F(Core_MessageDispatcher, VariableLengthMessages)
    {
        uint8_t short_msg[] = { 0x01 };
        uint8_t medium_msg[] = { 0x02, 0x03, 0x04, 0x05 };
        uint8_t long_msg[64];
        for (size_t i = 0; i < sizeof(long_msg); i++) {
            long_msg[i] = static_cast<uint8_t>(i);
        }

        ASSERT_EQ(_dispatcher->PushData(sizeof(short_msg), short_msg), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PushData(sizeof(medium_msg), medium_msg), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PushData(sizeof(long_msg), long_msg), ::Thunder::Core::ERROR_NONE);

        uint8_t readBuf[64] = {};
        uint16_t readLen;

        readLen = sizeof(readBuf);
        EXPECT_EQ(_dispatcher->PopData(readLen, readBuf), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readLen, sizeof(short_msg));
        EXPECT_EQ(readBuf[0], 0x01);

        readLen = sizeof(readBuf);
        EXPECT_EQ(_dispatcher->PopData(readLen, readBuf), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readLen, sizeof(medium_msg));
        EXPECT_EQ(readBuf[0], 0x02);

        readLen = sizeof(readBuf);
        EXPECT_EQ(_dispatcher->PopData(readLen, readBuf), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(readLen, sizeof(long_msg));
        EXPECT_EQ(readBuf[0], 0x00);
        EXPECT_EQ(readBuf[63], 63);
    }

    // Pop from empty buffer returns appropriate error
    TEST_F(Core_MessageDispatcher, PopFromEmptyBuffer)
    {
        uint8_t readData[4] = {};
        uint16_t readLength = sizeof(readData);

        // Nothing was pushed — pop should indicate no data
        uint32_t result = _dispatcher->PopData(readLength, readData);
        // Empty pop should return ERROR_READ_ERROR or similar
        EXPECT_NE(result, ::Thunder::Core::ERROR_NONE);
    }

} // Core
} // Tests
} // Thunder
