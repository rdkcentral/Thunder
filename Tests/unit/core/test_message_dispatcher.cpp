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

#include <core/core.h>
#include <gtest/gtest.h>

#include <core/FileObserver.h>
#include <fstream>

#include <messaging/messaging.h>

namespace WPEFramework {
namespace Tests {

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
            if (Core::File(_basePath).IsDirectory()) {
                Core::Directory(_basePath.c_str()).Destroy();
            }
            //create directory
            if (!Core::Directory(_basePath.c_str()).CreatePath()) {
                std::cerr << "Unable to create MessageDispatcher directory" << std::endl;
            }
        }

        ~Core_MessageDispatcher()
        {
            if (Core::File(_basePath).IsDirectory()) {
                Core::Directory(_basePath.c_str()).Destroy();
            }
        }

        void SetUp() override
        {
            _dispatcher.reset(new Messaging::MessageDataBuffer(_identifier, _instanceId, _basePath, DATA_SIZE, 0, true));
        }
        void TearDown() override
        {
            _dispatcher.reset(nullptr);

            ++_instanceId;

            Core::Singleton::Dispose();
        }

        std::unique_ptr<Messaging::MessageDataBuffer> _dispatcher;
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
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

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
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_GENERAL);

        //assert
        ASSERT_EQ(readLength, 2);
        ASSERT_EQ(readData[0], 13);
    }

    TEST_F(Core_MessageDispatcher, CreateAndOpenOperatesOnSameValidFile)
    {
        Messaging::MessageDataBuffer writerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, true);
        Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(writerDispatcher.PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(readerDispatcher.PopData(readLength, readData), Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, MessageDispatcherCanBeOpenedAndClosed)
    {
        Messaging::MessageDataBuffer writerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, true);

        {
            Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

            //destructor is called
        }

        //reopen
        Messaging::MessageDataBuffer readerDispatcher(_T("test_md"), 0, this->_basePath, DATA_SIZE, 0, false);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(writerDispatcher.PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(readerDispatcher.PopData(readLength, readData), Core::ERROR_NONE);

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
        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);

        //second read, write, assert
        testData[0] = 40;
        readLength = sizeof(readData);
        ASSERT_EQ(_dispatcher->PushData(1, testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);
        ASSERT_EQ(readLength, 1);
        ASSERT_EQ(readData[0], 40);
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInDiffrentProcesses)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Messaging::MessageDataBuffer dispatcher(this->_identifier, this->_instanceId, this->_basePath, DATA_SIZE, 0, false);

            uint8_t readData[4];
            uint16_t readLength = sizeof(readData);

            testAdmin.Sync("setup reader");
            testAdmin.Sync("writer wrote");

            ASSERT_EQ(dispatcher.PopData(readLength, readData), Core::ERROR_NONE);

            ASSERT_EQ(readLength, 2);
            ASSERT_EQ(readData[0], 13);
            ASSERT_EQ(readData[1], 37);

            testAdmin.Sync("reader read");
            testAdmin.Sync("done");
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as writer
        IPTestAdministrator testAdmin(otherSide);
        {
            testAdmin.Sync("setup reader");

            uint8_t testData[2] = { 13, 37 };
            ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);

            testAdmin.Sync("writer wrote");
            testAdmin.Sync("reader read");
        }
        testAdmin.Sync("done");
    }

    TEST_F(Core_MessageDispatcher, PushDataShouldNotFitWhenExcedingDataBufferSize)
    {
        uint8_t fullBufferSimulation[DATA_SIZE + 1
            + sizeof(Core::CyclicBuffer::control)];

        ASSERT_EQ(_dispatcher->PushData(sizeof(fullBufferSimulation), fullBufferSimulation), Core::ERROR_WRITE_ERROR);
    }

    TEST_F(Core_MessageDispatcher, OffsetCutPushDataShouldFlushOldDataIfDoesNotFitOffsetCut)
    {
        // CyclicBuffer Reserve requires 'length' < Size()
        uint8_t fullBufferSimulation[DATA_SIZE - sizeof(uint16_t) - 1]; // sizeof(length) + length - 1, eg. < Size()
        uint8_t testData[] = { 12, 11, 13, 21 };
        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        EXPECT_EQ(_dispatcher->PushData(sizeof(fullBufferSimulation), fullBufferSimulation), Core::ERROR_NONE);
        // One element free space remaining
        // 2+2 bytes, 1 at tail position, 3 starting at position 0

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        // new data written, so the oldest data should be replaced

        // this is first entry and should be first popped (FIFO)
        EXPECT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

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

        EXPECT_EQ(_dispatcher->PushData(sizeof(onlyOffsetFitsSimulation), onlyOffsetFitsSimulation), Core::ERROR_NONE);
        // Two elements free space remaining so offset of testData should fit at the end of the cyclic buffer,
        // and content of testData buffer should be at the beginning of the cyclic buffer

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

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

        EXPECT_EQ(_dispatcher->PushData(sizeof(offsetPlusFitsSimulation), offsetPlusFitsSimulation), Core::ERROR_NONE);
        // Three elements free space remaining, so the offset of testData should still fit at the end of the cyclic buffer,
        // as well as the first part of the testData buffer, but its second part should be at the beginning of the cyclic buffer

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

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

        EXPECT_EQ(_dispatcher->PushData(sizeof(almostFullBufferSimulation), almostFullBufferSimulation), Core::ERROR_NONE);

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData1), testData1), Core::ERROR_NONE);
        // The cyclic buffer is now full

        EXPECT_EQ(_dispatcher->PushData(sizeof(testData2), testData2), Core::ERROR_NONE);
        // The cyclic buffer needs to flush the almostFullBufferSimulation to make space for testData2

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);
        EXPECT_EQ(readLength, sizeof(testData1));
        EXPECT_EQ(readData[0], testData1[0]);
        EXPECT_EQ(readData[3], testData1[3]);

        EXPECT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);
        EXPECT_EQ(readLength, sizeof(testData2));
        EXPECT_EQ(readData[0], testData2[0]);
        EXPECT_EQ(readData[3], testData2[3]);
    }

    //doorbell (socket) is not quite working inside test suite
    TEST_F(Core_MessageDispatcher, DISABLED_ReaderShouldWaitUntillRingBells)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Messaging::MessageDataBuffer dispatcher(this->_identifier, this->_instanceId, this->_basePath, 0, false);

            uint8_t readData[4];
            uint16_t readLength = sizeof(readData);

            bool called = false;
            dispatcher.Wait(0); //initialize socket
            testAdmin.Sync("init");

            if (dispatcher.Wait(Core::infinite) == Core::ERROR_NONE) {
                ASSERT_EQ(dispatcher.PopData(readLength, readData), Core::ERROR_NONE);

                ASSERT_EQ(readLength, 2);
                ASSERT_EQ(readData[0], 13);
                ASSERT_EQ(readData[1], 37);
                called = true;
            }
            ASSERT_EQ(called, true);

            testAdmin.Sync("done");
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as writer
        IPTestAdministrator testAdmin(otherSide);
        {
            uint8_t testData[2] = { 13, 37 };
            testAdmin.Sync("init");

            ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        }
        testAdmin.Sync("done");
    }
#ifdef _0
    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcess)
    {
        uint8_t testData[2] = { 13, 37 };

        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value, uint16_t& outLength, uint8_t* outValue) {
            EXPECT_EQ(length, sizeof(testData));
            EXPECT_EQ(value[0], 13);
            EXPECT_EQ(value[1], 37);

            outValue[0] = 60;
            outValue[1] = 61;
            outLength = 2;
        });

        auto result = _dispatcher->PushMetadata(sizeof(testData), testData, sizeof(testData));
        ASSERT_EQ(result, 2);
        ASSERT_EQ(testData[0], 60);
        ASSERT_EQ(testData[1], 61);
        ::SleepMs(50); //wait for callback complete before closing

        _dispatcher->UnregisterDataAvailable();
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcessTwice)
    {
        uint8_t testData1[2] = { 13, 37 };
        uint8_t testData2[2] = { 12, 34 };

        //first write and read
        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value, uint16_t& outLength, uint8_t* outValue) {
            EXPECT_EQ(length, sizeof(testData1));
            EXPECT_EQ(value[0], 13);
            EXPECT_EQ(value[1], 37);

            outValue[0] = 60;
            outValue[1] = 61;
            outLength = 2;
        });
        auto result = _dispatcher->PushMetadata(sizeof(testData1), testData1, sizeof(testData1));
        ASSERT_EQ(result, 2);
        ASSERT_EQ(testData1[0], 60);
        ASSERT_EQ(testData1[1], 61);
        ::SleepMs(50); //need to wait before unregistering, not clean solution though
        _dispatcher->UnregisterDataAvailable();

        //second write and read
        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value, uint16_t& outLength, uint8_t* outValue) {
            EXPECT_EQ(length, sizeof(testData2));
            EXPECT_EQ(value[0], 12);
            EXPECT_EQ(value[1], 34);

            outValue[0] = 60;
            outValue[1] = 61;
            outLength = 2;
        });
        result = _dispatcher->PushMetadata(sizeof(testData2), testData2, sizeof(testData2));
        ASSERT_EQ(result, 2);
        ASSERT_EQ(testData2[0], 60);
        ASSERT_EQ(testData2[1], 61);
        ::SleepMs(50);
        _dispatcher->UnregisterDataAvailable();
    }

    //socket problems inside test suite
    TEST_F(Core_MessageDispatcher, DISABLED_WriteAndReadMetaDataAreEqualInDiffrentProcesses)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> dispatcher(this->_identifier, this->_instanceId, false, this->_basePath);
            uint8_t testData[2] = { 13, 37 };
            //testAdmin.Sync("setup");

            auto result = _dispatcher->PushMetadata(sizeof(testData), testData, sizeof(testData));
            ASSERT_EQ(result, 2);
            ASSERT_EQ(testData[0], 60);
            ASSERT_EQ(testData[1], 61);
            ::SleepMs(2000);
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as reader
        IPTestAdministrator testAdmin(otherSide);
        {
            _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value, uint16_t& outLength, uint8_t* outValue) {
                std::vector<uint8_t> result{ 60, 61 };
                EXPECT_EQ(length, 2);
                EXPECT_EQ(value[0], 13);
                EXPECT_EQ(value[1], 37);
                return result;
            });

            ::SleepMs(2000);
        }
        _dispatcher->UnregisterDataAvailable();
    }

    TEST_F(Core_MessageDispatcher, WriteMetaDataShouldFailIfReaderNotRegistered)
    {
        uint8_t testData[2] = { 13, 37 };

        auto result = _dispatcher->PushMetadata(sizeof(testData), testData, sizeof(testData));
        ASSERT_EQ(result, 0);
    }
#endif
} // Tests
} // WPEFramework
