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
        Core_MessageDispatcher()
            : _dispatcher(nullptr)
            , _identifier(_T("/tmp/test"))
            , _dataBufferSize(9 * 1024)
        {
        }

        void SetUp() override
        {
            _dispatcher.reset(new Core::MessageDispatcher(Core::MessageDispatcher::Create(_identifier, _instanceId, _dataBufferSize)));
        }
        void TearDown() override
        {
            _dispatcher.reset(nullptr);
            //delete buffers from disk
            string deleteCommand = Core::Format("rm -f %s* ", _identifier.c_str());
            system(deleteCommand.c_str());

            ++_instanceId;

            Core::Singleton::Dispose();
        }

        std::unique_ptr<Core::MessageDispatcher> _dispatcher;
        string _identifier;
        uint32_t _dataBufferSize;

        static int _instanceId;
    };

    int Core_MessageDispatcher::_instanceId = 0;

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInSameProcess)
    {
        //arrange
        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = _dispatcher->GetWriter();
        auto& reader = _dispatcher->GetReader();

        //act
        ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);

        //assert
        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, CreateAndOpenOperatesOnSameValidFile)
    {
        auto writerDispatcher = Core::MessageDispatcher::Create(_T("/tmp/md1"), 0, 2048);

        auto readerDispatcher = Core::MessageDispatcher::Open(_T("/tmp/md1"), 0);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = writerDispatcher.GetWriter();
        auto& reader = writerDispatcher.GetReader();

        ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);

        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, MessageDispatcherCanBeOpenedAndClosed)
    {
        auto writerDispatcher = Core::MessageDispatcher::Create(_T("/tmp/md1"), 0, 2048);
        {
            auto readerDispatcher = Core::MessageDispatcher::Open(_T("/tmp/md1"), 0);
            //destructor is called
        }

        //reopen
        auto readerDispatcher = Core::MessageDispatcher::Open(_T("/tmp/md1"), 0);

        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = writerDispatcher.GetWriter();
        auto& reader = readerDispatcher.GetReader();

        ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);
        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);

        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInSameProcessTwice)
    {
        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = _dispatcher->GetWriter();
        auto& reader = _dispatcher->GetReader();

        //first read, write, assert
        ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);

        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);
        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, 2);
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);

        //second read, write, assert
        testData[0] = 40;
        ASSERT_EQ(writer.Data(1, 1, testData), Core::ERROR_NONE);
        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);
        ASSERT_EQ(readType, 1);
        ASSERT_EQ(readLength, 1);
        ASSERT_EQ(readData[0], 40);
    }

    TEST_F(Core_MessageDispatcher, WriteAndReadDataAreEqualInDiffrentProcesses)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            auto dispatcher = Core::MessageDispatcher::Open(this->_identifier, this->_instanceId);

            auto& reader = dispatcher.GetReader();
            uint8_t readType;
            uint16_t readLength;
            uint8_t readData[2] = { 0, 0 };

            testAdmin.Sync("setup reader");
            testAdmin.Sync("writer wrote");

            ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);

            ASSERT_EQ(readType, 0);
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
            auto& writer = _dispatcher->GetWriter();
            ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);

            testAdmin.Sync("writer wrote");
            testAdmin.Sync("reader read");
        }
        testAdmin.Sync("done");
    }

    TEST_F(Core_MessageDispatcher, PushDataShouldNotFitWhenExcedingDataBufferSize)
    {
        std::vector<uint8_t> fullBufferSimulation;
        fullBufferSimulation.resize(this->_dataBufferSize //raw size
            - sizeof(Core::CyclicBuffer::control) //size of administration
            - sizeof(uint8_t) //size of type (part of message header)
            - sizeof(uint16_t)); //size of length (part of message header)

        auto& writer = _dispatcher->GetWriter();

        ASSERT_EQ(writer.Data(0, fullBufferSimulation.size(), fullBufferSimulation.data()), Core::ERROR_WRITE_ERROR);
    }

    TEST_F(Core_MessageDispatcher, PushDataShouldFlushOldDatIfDoesNotFit)
    {
        std::vector<uint8_t> fullBufferSimulation;
        fullBufferSimulation.resize(this->_dataBufferSize //raw size
            - sizeof(Core::CyclicBuffer::control) //size of administration
            - sizeof(uint8_t) //size of type (part of message header)
            - sizeof(uint16_t) //size of length (part of message header)
            - 1);
        uint8_t testData[] = { 12, 21 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = _dispatcher->GetWriter();
        auto& reader = _dispatcher->GetReader();

        ASSERT_EQ(writer.Data(0, fullBufferSimulation.size(), fullBufferSimulation.data()), Core::ERROR_NONE);
        //buffer is full, but trying to write new data

        ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);
        //new data written, so the oldest data should be replaced
        //this is first entry and should be first popped (FIFO)

        ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);
        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 12);
        ASSERT_EQ(readData[1], 21);
    }

    TEST_F(Core_MessageDispatcher, ReaderShouldWaitUntillRingBells)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            auto dispatcher = Core::MessageDispatcher::Open(this->_identifier, this->_instanceId);

            auto& reader = dispatcher.GetReader();
            uint8_t readType;
            uint16_t readLength;
            uint8_t readData[2] = { 0, 0 };
            bool called = false;
            testAdmin.Sync("init");

            if (reader.Wait(Core::infinite) == Core::ERROR_NONE) {
                ASSERT_EQ(reader.Data(readType, readLength, readData), Core::ERROR_NONE);

                ASSERT_EQ(readType, 0);
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
            auto& writer = _dispatcher->GetWriter();
            testAdmin.Sync("init");

            ASSERT_EQ(writer.Data(0, sizeof(testData), testData), Core::ERROR_NONE);
            ::SleepMs(10); //not a nice way, but now Wait will be called before ringing
            writer.Ring();
        }
        testAdmin.Sync("done");
    }
    

    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcess)
    {
        uint8_t testData[2] = { 13, 37 };
        bool called = false;

        auto& writer = _dispatcher->GetWriter();

        _dispatcher->RegisterDataAvailable([&](const uint8_t type, const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(type, 0);
            ASSERT_EQ(length, sizeof(testData));
            ASSERT_EQ(value[0], 13);
            ASSERT_EQ(value[1], 37);
            called = true;
        });

        ASSERT_EQ(writer.Metadata(0, sizeof(testData), testData), Core::ERROR_NONE);
        ::SleepMs(50);

        ASSERT_EQ(called, true);
        _dispatcher->UnregisterDataAvailable();
    }

    
    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInSameProcessTwice)
    {
        uint8_t testData1[2] = { 13, 37 };
        uint8_t testData2[2] = { 12, 34 };

        auto& writer = _dispatcher->GetWriter();

        //first write and read
        _dispatcher->RegisterDataAvailable([&](const uint8_t type, const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(type, 0);
            ASSERT_EQ(length, sizeof(testData1));
            ASSERT_EQ(value[0], 13);
            ASSERT_EQ(value[1], 37);
        });
        ASSERT_EQ(writer.Metadata(0, sizeof(testData1), testData1), Core::ERROR_NONE);
        ::SleepMs(50); //need to wait before unregistering, not clean solution though
        _dispatcher->UnregisterDataAvailable();

        //second write and read
        _dispatcher->RegisterDataAvailable([&](const uint8_t type, const uint16_t length, const uint8_t* value) {
            ASSERT_EQ(type, 0);
            ASSERT_EQ(length, sizeof(testData2));
            ASSERT_EQ(value[0], 12);
            ASSERT_EQ(value[1], 34);
        });
        ASSERT_EQ(writer.Metadata(0, sizeof(testData2), testData2), Core::ERROR_NONE);
        ::SleepMs(50);
        _dispatcher->UnregisterDataAvailable();
    }
    
    TEST_F(Core_MessageDispatcher, WriteAndReadMetaDataAreEqualInDiffrentProcesses)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            auto dispatcher = Core::MessageDispatcher::Open(this->_identifier, this->_instanceId);
            uint8_t testData[2] = { 13, 37 };
            auto& writer = dispatcher.GetWriter();
            //testAdmin.Sync("setup");

            ASSERT_EQ(writer.Metadata(0, sizeof(testData), testData), Core::ERROR_NONE);
            ::SleepMs(2000);

        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };

        // This side (tested) acts as reader
        IPTestAdministrator testAdmin(otherSide);
        {        
            _dispatcher->RegisterDataAvailable([&](const uint8_t type, const uint16_t length, const uint8_t* value) {
                ASSERT_EQ(type, 0);
                ASSERT_EQ(length, 2);
                ASSERT_EQ(value[0], 13);
                ASSERT_EQ(value[1], 37);
                std::cerr << "CALLED" << std::endl;
            });

            ::SleepMs(2000);

        }
        _dispatcher->UnregisterDataAvailable();
    }

} // Tests
} // WPEFramework
