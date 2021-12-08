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
                Core::Directory(_basePath.c_str()).Destroy(false);
            }
            //create directory
            if (!Core::Directory(_basePath.c_str()).CreatePath()) {
                std::cerr << "Unable to create MessageDispatcher directory" << std::endl;
            }
        }

        ~Core_MessageDispatcher()
        {
            if (Core::File(_basePath).IsDirectory()) {
                Core::Directory(_basePath.c_str()).Destroy(false);
            }
        }

        void SetUp() override
        {
            _dispatcher.reset(new Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE>(_identifier, _instanceId, true, _basePath));
        }
        void TearDown() override
        {
            _dispatcher.reset(nullptr);

            ++_instanceId;

            Core::Singleton::Dispose();
        }

        std::unique_ptr<Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE>> _dispatcher;
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
        Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> writerDispatcher(_T("test_md"), 0, true, this->_basePath);
        Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> readerDispatcher(_T("test_md"), 0, false, this->_basePath);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, MessageDispatcherCanBeOpenedAndClosed)
    {
        Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> writerDispatcher(_T("test_md"), 0, true, this->_basePath);

        {
            Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> readerDispatcher(_T("test_md"), 0, false, this->_basePath);

            //destructor is called
        }

        //reopen
        Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> readerDispatcher(_T("test_md"), 0, true, this->_basePath);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);

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
            Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> dispatcher(this->_identifier, this->_instanceId, false, this->_basePath);

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

    TEST_F(Core_MessageDispatcher, PushDataShouldFlushOldDataIfDoesNotFit)
    {
        uint8_t fullBufferSimulation[DATA_SIZE - 1 + sizeof(Core::CyclicBuffer::control) //almost full buffer
            - sizeof(uint8_t) //size of type (part of message header)
            - sizeof(uint16_t)]; //size of length (part of message header)

        uint8_t testData[] = { 12, 21 };

        uint8_t readData[4];
        uint16_t readLength = sizeof(readData);

        ASSERT_EQ(_dispatcher->PushData(sizeof(fullBufferSimulation), fullBufferSimulation), Core::ERROR_NONE);
        //buffer is full, but trying to write new data

        ASSERT_EQ(_dispatcher->PushData(sizeof(testData), testData), Core::ERROR_NONE);
        //new data written, so the oldest data should be replaced
        //this is first entry and should be first popped (FIFO)

        ASSERT_EQ(_dispatcher->PopData(readLength, readData), Core::ERROR_NONE);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 12);
        ASSERT_EQ(readData[1], 21);
    }

    //doorbell (socket) is not quite working inside test suite
    TEST_F(Core_MessageDispatcher, DISABLED_ReaderShouldWaitUntillRingBells)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Core::MessageDispatcherType<METADATA_SIZE, DATA_SIZE> dispatcher(this->_identifier, this->_instanceId, false, this->_basePath);

            uint8_t readData[4];
            uint16_t readLength = sizeof(readData);

            bool called = false;
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
            ::SleepMs(10); //not a nice way, but now Wait will be called before ringing
            _dispatcher->Ring();
        }
        testAdmin.Sync("done");
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcess)
    {
        uint8_t testData[2] = { 13, 37 };
        bool called = false;

        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(length, sizeof(testData));
            ASSERT_EQ(value[0], 13);
            ASSERT_EQ(value[1], 37);
            called = true;
        });

        ASSERT_EQ(_dispatcher->PushMetadata(sizeof(testData), testData), Core::ERROR_NONE);
        ::SleepMs(50); //wait for callback complete before closing

        ASSERT_EQ(called, true);
        _dispatcher->UnregisterDataAvailable();
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcessTwice)
    {
        uint8_t testData1[2] = { 13, 37 };
        uint8_t testData2[2] = { 12, 34 };

        //first write and read
        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(length, sizeof(testData1));
            ASSERT_EQ(value[0], 13);
            ASSERT_EQ(value[1], 37);
        });
        ASSERT_EQ(_dispatcher->PushMetadata(sizeof(testData1), testData1), Core::ERROR_NONE);
        ::SleepMs(50); //need to wait before unregistering, not clean solution though
        _dispatcher->UnregisterDataAvailable();

        //second write and read
        _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(length, sizeof(testData2));
            ASSERT_EQ(value[0], 12);
            ASSERT_EQ(value[1], 34);
        });
        ASSERT_EQ(_dispatcher->PushMetadata(sizeof(testData2), testData2), Core::ERROR_NONE);
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

            ASSERT_EQ(dispatcher.PushMetadata(sizeof(testData), testData), Core::ERROR_NONE);
            ::SleepMs(2000);
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as reader
        IPTestAdministrator testAdmin(otherSide);
        {
            _dispatcher->RegisterDataAvailable([&](const uint16_t length, const uint8_t* value) {
                ASSERT_EQ(length, 2);
                ASSERT_EQ(value[0], 13);
                ASSERT_EQ(value[1], 37);
                std::cerr << "CALLED" << std::endl;
            });

            ::SleepMs(2000);
        }
        _dispatcher->UnregisterDataAvailable();
    }

    TEST_F(Core_MessageDispatcher, WriteMetaDataShouldFailIfReaderNotRegistered)
    {
        uint8_t testData[2] = { 13, 37 };

        ASSERT_EQ(_dispatcher->PushMetadata(sizeof(testData), testData), Core::ERROR_UNAVAILABLE);
    }

} // Tests
} // WPEFramework
