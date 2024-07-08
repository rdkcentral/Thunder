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
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            ::Thunder::Messaging::MessageDataBuffer dispatcher(this->_identifier, this->_instanceId, this->_basePath, DATA_SIZE, 0, false);

            uint8_t readData[4];
            uint16_t readLength = sizeof(readData);

            // Arbitrary timeout value, 1 second
            ASSERT_EQ(dispatcher.Wait(1000), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(dispatcher.PopData(readLength, readData), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(readLength, 2);
            ASSERT_EQ(readData[0], 13);
            ASSERT_EQ(readData[1], 37);
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as writer
        IPTestAdministrator testAdmin(otherSide);
        {
            uint8_t testData[2] = { 13, 37 };
            ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), ::Thunder::Core::ERROR_NONE);
            // The 'ring' is implicit
//            _dispatcher->Ring();
        }
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
        uint8_t fullBufferSimulation[DATA_SIZE - sizeof(uint16_t) - 1]; // sizeof(length) + length - 1, eg. < Size()
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
        uint8_t onlyOffsetFitsSimulation[DATA_SIZE - sizeof(uint16_t) - 2];
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
        uint8_t offsetPlusFitsSimulation[DATA_SIZE - sizeof(uint16_t) - 3];
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
        uint8_t almostFullBufferSimulation[DATA_SIZE - sizeof(uint16_t) - 6];
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
} // Core
} // Tests
} // Thunder
