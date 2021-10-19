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

namespace WPEFramework {
namespace Tests {

    class Core_MessageDispatcher : public testing::Test {
    protected:
        void SetUp() override
        {
            _dispatcher.reset(new Core::MessageDispatcher());
            ASSERT_EQ(Core::ERROR_NONE, _dispatcher->Open(_doorBellName + std::to_string(_testCount), _cyclicBufferName + std::to_string(_testCount)));
        }

        void TearDown() override
        {
            _dispatcher->Close();
            _dispatcher.reset(nullptr);
            ++_testCount;
        }

        static int _testCount;
        string _cyclicBufferName = "/tmp/test_cyclic_buffer";
        string _doorBellName = "/tmp/test_doorbell";
        std::unique_ptr<Core::MessageDispatcher> _dispatcher;
    };

    int Core_MessageDispatcher::_testCount = 0;

    TEST_F(Core_MessageDispatcher, NonBlockingWriteAndReadMessageDataAreEqualInSameProcess)
    {
        //arrange
        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = _dispatcher->GetWriter();
        auto& reader = _dispatcher->GetReader();

        //act
        writer.Data(0, sizeof(testData), testData);
        reader.Data(readType, readLength, readData);

        //assert
        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, sizeof(testData));
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
    }

    TEST_F(Core_MessageDispatcher, NonBlockingWriteAndReadMessageDataAreEqualInDiffrentProcesses)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Core::MessageDispatcher dispatcher;
            ASSERT_EQ(dispatcher.Open(this->_doorBellName + std::to_string(this->_testCount), this->_cyclicBufferName + std::to_string(this->_testCount)),
                Core::ERROR_NONE);

            auto& reader = dispatcher.GetReader();
            uint8_t readType;
            uint16_t readLength;
            uint8_t readData[2] = { 0, 0 };

            testAdmin.Sync("setup reader");
            testAdmin.Sync("writer wrote");

            reader.Data(readType, readLength, readData);

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
            writer.Data(0, sizeof(testData), testData);

            testAdmin.Sync("writer wrote");
            testAdmin.Sync("reader read");
        }
        testAdmin.Sync("done");

        Core::Singleton::Dispose();
    }

    
    TEST_F(Core_MessageDispatcher, NonBlockingWriteAndReadMessageDataAreEqualInSameProcessTwice)
    {
        uint8_t testData[2] = { 13, 37 };

        uint8_t readType;
        uint16_t readLength;
        uint8_t readData[2] = { 0, 0 };

        auto& writer = _dispatcher->GetWriter();
        auto& reader = _dispatcher->GetReader();

        //first read, write, assert
        ASSERT_EQ(true, reader.IsEmpty());
        writer.Data(0, sizeof(testData), testData);
        ASSERT_EQ(false, reader.IsEmpty());
        reader.Data(readType, readLength, readData);
        ASSERT_EQ(readType, 0);
        ASSERT_EQ(readLength, 2);
        ASSERT_EQ(readData[0], 13);
        ASSERT_EQ(readData[1], 37);
        ASSERT_EQ(true, reader.IsEmpty());

        //second read, write, assert
        testData[0] = 40;
        writer.Data(1, 1, testData);
        reader.Data(readType, readLength, readData);
        ASSERT_EQ(readType, 1);
        ASSERT_EQ(readLength, 1);
        ASSERT_EQ(readData[0], 40);
    }
    

    TEST_F(Core_MessageDispatcher, RingShouldBellOnDataPush)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Core::MessageDispatcher dispatcher;
            ASSERT_EQ(dispatcher.Open(this->_doorBellName + std::to_string(this->_testCount), this->_cyclicBufferName + std::to_string(this->_testCount)),
                Core::ERROR_NONE);
            auto& reader = dispatcher.GetReader();
            uint8_t readType;
            uint16_t readLength;
            uint8_t readData[2] = { 0, 0 };
            testAdmin.Sync("init");

            if (reader.Wait(Core::infinite) == Core::ERROR_NONE) {
                reader.Data(readType, readLength, readData);

                ASSERT_EQ(readType, 0);
                ASSERT_EQ(readLength, 2);
                ASSERT_EQ(readData[0], 13);
                ASSERT_EQ(readData[1], 37);
            }
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
            ::SleepMs(10); //not a nice way, but now Wait will be called before Data (and thus Ring)
            writer.Data(0, sizeof(testData), testData);
        }
        testAdmin.Sync("done");

        Core::Singleton::Dispose();
    }

    //Assert that ringbell got reset inside
    TEST_F(Core_MessageDispatcher, RingShouldBellOnDataPushTwice)
    {
        auto lambdaFunc = [this](IPTestAdministrator& testAdmin) {
            Core::MessageDispatcher dispatcher;
            ASSERT_EQ(dispatcher.Open(this->_doorBellName + std::to_string(this->_testCount), this->_cyclicBufferName + std::to_string(this->_testCount)),
                Core::ERROR_NONE);
            auto& reader = dispatcher.GetReader();
            uint8_t readType;
            uint16_t readLength;
            uint8_t readData[2] = { 0, 0 };
            bool called[2] = { false, false };

            testAdmin.Sync("init");

            if (reader.Wait(Core::infinite) == Core::ERROR_NONE) {
                reader.Data(readType, readLength, readData);
                //first ring
                called[0] = true;
                testAdmin.Sync("first ring");
            }

            if (reader.Wait(Core::infinite) == Core::ERROR_NONE) {
                reader.Data(readType, readLength, readData);
                //first ring
                called[1] = true;
                testAdmin.Sync("second ring");
            }

            ASSERT_EQ(called[0], true);
            ASSERT_EQ(called[1], true);

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
            ::SleepMs(10); //not a nice way, but now Wait will be called before Data (and thus Ring)

            writer.Data(0, sizeof(testData), testData);
            testAdmin.Sync("first ring");

            writer.Data(0, sizeof(testData), testData);
            testAdmin.Sync("second ring");
        }
        testAdmin.Sync("done");

        Core::Singleton::Dispose();
    }

} // Tests
} // WPEFramework
