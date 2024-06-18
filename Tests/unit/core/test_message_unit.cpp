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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    class Control : public Thunder::Core::Messaging::IControl {
    public:
        Control(const Thunder::Core::Messaging::MetaData& metaData)
            : _metaData(metaData)
        {
        }
        ~Control() override
        {
            Destroy();
        }
        void Enable(bool enable) override
        {
            _isEnabled = enable;
        }
        bool Enable() const override
        {
            return _isEnabled;
        }
        void Destroy() override
        {
            _isEnabled = false;
        }
        const Thunder::Core::Messaging::MetaData& MessageMetaData() const override
        {
            return _metaData;
        }

    private:
        bool _isEnabled;
        Thunder::Core::Messaging::MetaData _metaData;
    };

    class Core_Messaging_MessageUnit : public testing::Test {
    protected:
        Core_Messaging_MessageUnit()
        {
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_2"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_3"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_4"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_1"), _T("Test_Module2") }));
            _controls.emplace_back(new Control({ Thunder::Core::Messaging::MetaData::MessageType::LOGGING, _T("Test_Category_5"), _T("SysLog") }));
        }
        ~Core_Messaging_MessageUnit() = default;

        static void SetUpTestSuite()
        {
            Thunder::Core::Messaging::MessageUnit::Instance().IsBackground(_background);
            Thunder::Core::Messaging::MessageUnit::Instance().Open(_basePath);
        }

        static void TearDownTestSuite()
        {
            Thunder::Core::Messaging::MessageUnit::Instance().Close();
            Thunder::Core::Singleton::Dispose();
        }
        void SetUp() override
        {
            for (const auto& control : _controls) {
                Thunder::Core::Messaging::MessageUnit::Instance().Announce(control.get());
            }
        }

        void TearDown() override
        {
            Thunder::Core::Messaging::MessageUnit::Instance().Defaults(_T(""));
            for (const auto& control : _controls) {
                Thunder::Core::Messaging::MessageUnit::Instance().Revoke(control.get());
            }
        }

        string DispatcherIdentifier()
        {
            string result;
            Thunder::Core::SystemInfo::GetEnvironment(Thunder::Core::Messaging::MessageUnit::MESSAGE_DISPACTHER_IDENTIFIER_ENV, result);
            return result;
        }

        string DispatcherBasePath()
        {
            string result;
            Thunder::Core::SystemInfo::GetEnvironment(Thunder::Core::Messaging::MessageUnit::MESSAGE_DISPATCHER_PATH_ENV, result);
            return result;
        }

        static bool _background;
        static string _basePath;
        std::list<std::unique_ptr<Thunder::Core::Messaging::IControl>> _controls;
    };

    bool Core_Messaging_MessageUnit::_background = false;
    string Core_Messaging_MessageUnit::_basePath = _T("/tmp/");

    TEST_F(Core_Messaging_MessageUnit, TraceMessageIsEnabledByDefaultWhenConfigFullySpecified)
    {
        const string config = R"({"tracing":{"messages":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":true}]}})";

        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Plugin_DeviceInfo") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Some_Module") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo") }));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessageIsDisabledByDefaultWhenConfigFullySpecified)
    {
        const string config = R"({"tracing":{"messages":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":false}]}})";

        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Plugin_DeviceInfo") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Some_Module") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo") }));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreEnabledWhenModuleNotSpecified)
    {
        const string config = R"({"tracing":{"messages":[{"category":"Information","enabled":true}]}})";

        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Plugin_DeviceInfo") }));
        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Some_Module") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo") }));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreDisabledWhenModuleNotSpecified)
    {
        const string config = R"({"tracing":{"messages":[{"category":"Information","enabled":false}]}})";

        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Plugin_DeviceInfo") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Information"), _T("Some_Module") }));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo") }));
    }

    TEST_F(Core_Messaging_MessageUnit, LoggingMessageIsEnabledIfNotConfigured)
    {
        //logging messages are enabled by default (if not specified otherwise in the config)
        const string config = R"({"logging":{"messages":[{"category":"Startup","module":"SysLog","enabled":false}]}})";
        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::LOGGING, _T("Startup"), _T("SysLog") }));
        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault({ Thunder::Core::Messaging::MetaData::MessageType::LOGGING, _T("Notification"), _T("SysLog") }));
    }

    TEST_F(Core_Messaging_MessageUnit, MessageClientWillReturnListOfControls)
    {
        //this test is using metadata (IPC) passing, so no other proces tests for now
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto it = client.Enabled();

        int matches = 0;
        int count = 0;
        while (it.Next()) {
            auto info = it.Current();
            if (info.first.Module() == EXPAND_AND_QUOTE(MODULE_NAME)) {
                ++matches;
            }
            ++count;
        }

        ASSERT_GE(count, 4);
        ASSERT_EQ(matches, 4);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldUpdateExistingDefaultConfig)
    {
        const string config = R"({"tracing":{"messages":[{"category":"ExampleCategory","module":"ExampleModule","enabled":false}]}})";
        Thunder::Core::Messaging::MessageUnit::Instance().Defaults(config);
        const Thunder::Core::Messaging::MetaData toBeUpdated(Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("ExampleCategory"), _T("ExampleModule"));
        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(toBeUpdated));

        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        client.Enable(toBeUpdated, true);

        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(toBeUpdated));
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
    {
        const Thunder::Core::Messaging::MetaData toBeAdded(Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(toBeAdded));

        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        client.Enable(toBeAdded, true);

        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(toBeAdded));
        auto defaultsString = Thunder::Core::Messaging::MessageUnit::Instance().Defaults();
        Thunder::Core::Messaging::Settings settings;
        settings.FromString(defaultsString);

        ASSERT_EQ(settings.Tracing.Entries.Length(), 1);
        auto entriesIt = settings.Tracing.Entries.Elements();
        while (entriesIt.Next()) {
            ASSERT_STREQ(entriesIt.Current().Category.Value().c_str(), toBeAdded.Category().c_str());
            ASSERT_STREQ(entriesIt.Current().Module.Value().c_str(), toBeAdded.Module().c_str());
        }
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesByTypeShouldEnableEverything)
    {
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto itBeforeUpdate = client.Enabled();

        int matches = 0;
        while (itBeforeUpdate.Next()) {
            auto info = itBeforeUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.second == true) {
                ++matches;
            }
        }
        ASSERT_EQ(matches, 0);

        matches = 0;
        client.Enable({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T(""), _T("") }, true);
        auto itAfterUpdate = client.Enabled();
        while (itAfterUpdate.Next()) {
            auto info = itAfterUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.second == true) {
                ++matches;
            }
        }
        ASSERT_GE(matches, 5);
    }

    TEST_F(Core_Messaging_MessageUnit, LogMessagesCanToggledWhenLogModuleSpecified)
    {
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto itBeforeUpdate = client.Enabled();
        Thunder::Core::Messaging::MetaData messageToToggle(Thunder::Core::Messaging::MetaData::MessageType::LOGGING, _T("Test_Category_5"), _T("SysLog"));

        int matches = 0;
        while (itBeforeUpdate.Next()) {
            auto info = itBeforeUpdate.Current();
            if (info.first == messageToToggle && info.second == true) {
                ++matches;
            }
        }
        ASSERT_EQ(matches, 1);

        matches = 0;
        client.Enable(messageToToggle, false);
        auto itAfterUpdate = client.Enabled();
        while (itAfterUpdate.Next()) {
            auto info = itAfterUpdate.Current();
            if (info.first == messageToToggle && info.second == false) {
                ++matches;
            }
        }
        ASSERT_EQ(matches, 1);
    }

    TEST_F(Core_Messaging_MessageUnit, LogEnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
    {
        const Thunder::Core::Messaging::MetaData tobeAdded(Thunder::Core::Messaging::MetaData::MessageType::LOGGING, _T("Test_Category_5"), _T("SysLog"));
        ASSERT_TRUE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(tobeAdded));

        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        client.Enable(tobeAdded, false);

        ASSERT_FALSE(Thunder::Core::Messaging::MessageUnit::Instance().IsEnabledByDefault(tobeAdded));
        auto defaultsString = Thunder::Core::Messaging::MessageUnit::Instance().Defaults();
        Thunder::Core::Messaging::Settings settings;
        settings.FromString(defaultsString);

        ASSERT_EQ(settings.Logging.Entries.Length(), 1);
        auto entriesIt = settings.Logging.Entries.Elements();
        while (entriesIt.Next()) {
            ASSERT_STREQ(entriesIt.Current().Category.Value().c_str(), tobeAdded.Category().c_str());
            ASSERT_STREQ(entriesIt.Current().Module.Value().c_str(), tobeAdded.Module().c_str());
        }
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingFullySpecifiedMessageUpdateOnlyThisOne)
    {
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto itBeforeUpdate = client.Enabled();
        Thunder::Core::Messaging::MetaData message(Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME));

        int matches = 0;
        while (itBeforeUpdate.Next()) {
            auto info = itBeforeUpdate.Current();
            if (info.first == message && info.second == false) {
                ++matches;
            }
        }
        ASSERT_EQ(matches, 1);

        matches = 0;
        client.Enable(message, true);
        auto itAfterUpdate = client.Enabled();
        while (itAfterUpdate.Next()) {
            auto info = itAfterUpdate.Current();
            if (info.first == message && info.second == true) {
                ++matches;
            }
        }
        ASSERT_EQ(matches, 1);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByModuleShouldEnableAllCategoriesInsideIt)
    {
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto itBeforeUpdate = client.Enabled();

        int enabled = 0;
        while (itBeforeUpdate.Next()) {
            auto info = itBeforeUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.first.Module() == EXPAND_AND_QUOTE(MODULE_NAME)) {
                if (info.second == true) {
                    ++enabled;
                }
            }
        }
        ASSERT_EQ(enabled, 0);

        enabled = 0;
        client.Enable({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T(""), EXPAND_AND_QUOTE(MODULE_NAME) }, true);
        auto itAfterUpdate = client.Enabled();
        while (itAfterUpdate.Next()) {
            auto info = itAfterUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.first.Module() == EXPAND_AND_QUOTE(MODULE_NAME)) {
                if (info.second == true) {
                    ++enabled;
                }
            }
        }

        ASSERT_EQ(enabled, 4);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByCategoryShouldEnableItInAllModules)
    {
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework
        auto itBeforeUpdate = client.Enabled();

        int enabled = 0;
        while (itBeforeUpdate.Next()) {
            auto info = itBeforeUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.first.Category() == _T("Test_Category_1")) {
                if (info.second == true) {
                    ++enabled;
                }
            }
        }
        ASSERT_EQ(enabled, 0);

        enabled = 0;
        client.Enable({ Thunder::Core::Messaging::MetaData::MessageType::TRACING, _T("Test_Category_1"), _T("") }, true);
        auto itAfterUpdate = client.Enabled();
        while (itAfterUpdate.Next()) {
            auto info = itAfterUpdate.Current();
            if (info.first.Type() == Thunder::Core::Messaging::MetaData::MessageType::TRACING && info.first.Category() == _T("Test_Category_1")) {
                if (info.second == true) {
                    ++enabled;
                }
            }
        }

        ASSERT_EQ(enabled, 2);
    }

    TEST_F(Core_Messaging_MessageUnit, TextMessageEventIsProperlySerializedIfBufferBigEnough)
    {
        uint8_t buffer[1 * 1024];
        const string testTextMessage = _T("TEST MESSAGE");

        Messaging::TextMessage tm(testTextMessage);
        auto serialized = tm.Serialize(buffer, sizeof(buffer));
        ASSERT_GT(serialized, 0);

        auto deserialized = tm.Deserialize(buffer, sizeof(buffer));
        ASSERT_EQ(serialized, deserialized);

        string result;
        tm.ToString(result);
        ASSERT_STREQ(result.c_str(), testTextMessage.c_str());
    }

    TEST_F(Core_Messaging_MessageUnit, TextMessageEventIsProperlySerializedAndCutIfBufferNotBigEnough)
    {
        uint8_t buffer[5];
        const string testTextMessage = _T("abcdefghi");

        Messaging::TextMessage tm(testTextMessage);
        auto serialized = tm.Serialize(buffer, sizeof(buffer));
        ASSERT_GT(serialized, 0);

        auto deserialized = tm.Deserialize(buffer, serialized);
        ASSERT_EQ(serialized, deserialized);

        string result;
        tm.ToString(result);
        //last byte reserved for null termination
        ASSERT_STREQ(result.c_str(), _T("abcd"));
    }

    TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferBigEnough)
    {
        uint8_t buffer[1 * 1024];

        Thunder::Core::Messaging::ControlList cl;
        for (const auto& control : _controls) {
            cl.Announce(control.get());
        }

        auto serialized = cl.Serialize(buffer, sizeof(buffer));
        ASSERT_GT(serialized, 0);
        ASSERT_EQ(buffer[0], _controls.size());

        auto deserialized = cl.Deserialize(buffer, serialized);
        ASSERT_EQ(serialized, deserialized);

        auto informationIt = cl.Information();
        auto controlsIt = _controls.cbegin();
        while (informationIt.Next()) {
            ASSERT_EQ(informationIt.Current().first, controlsIt->get()->MessageMetaData());
            ++controlsIt;
        }
    }

    TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferNotBigEnough)
    {
        const int controlsThatShouldFit = 2;
        uint16_t maxBufferSize = 0;
        auto it = _controls.cbegin();
        for (int i = 0; i < controlsThatShouldFit; ++i, ++it) {
            maxBufferSize += sizeof(it->get()->MessageMetaData().Type());
            maxBufferSize += it->get()->MessageMetaData().Category().size() + 1;
            maxBufferSize += it->get()->MessageMetaData().Module().size() + 1;
            maxBufferSize += sizeof(bool);
        }

        std::vector<uint8_t> buffer;
        buffer.resize(maxBufferSize + 1);

        Thunder::Core::Messaging::ControlList cl;
        for (const auto& control : _controls) {
            cl.Announce(control.get());
        }

        auto serialized = cl.Serialize(buffer.data(), buffer.size());
        ASSERT_GT(serialized, 0);
        ASSERT_EQ(buffer[0], controlsThatShouldFit);

        auto deserialized = cl.Deserialize(buffer.data(), serialized);
        ASSERT_EQ(serialized, deserialized);

        auto informationIt = cl.Information();
        auto controlsIt = _controls.cbegin();
        while (informationIt.Next()) {
            ASSERT_EQ(informationIt.Current().first, controlsIt->get()->MessageMetaData());
            ++controlsIt;
        }
    }

    TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessage)
    {
        const string traceMessage = _T("some trace");
        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
        client.AddInstance(0); //we are in framework

        //factory should be added before attempting to pop data
        Messaging::TraceFactory factory;
        client.AddFactory(Thunder::Core::Messaging::MetaData::MessageType::TRACING, &factory);

        Messaging::TextMessage tm(traceMessage);
        Thunder::Core::Messaging::Information info(Thunder::Core::Messaging::MetaData::MessageType::TRACING,
            _T("some_category"),
            EXPAND_AND_QUOTE(MODULE_NAME),
            _T("some_file.cpp"),
            1337,
            Thunder::Core::Time::Now().Ticks());

        Thunder::Core::Messaging::MessageUnit::Instance().Push(info, &tm);

        auto messages = client.PopMessagesAsList();
        ASSERT_EQ(messages.size(), 1);
        auto message = messages.front();

        ASSERT_NE(message.first.MessageMetaData().Type(), Thunder::Core::Messaging::MetaData::MessageType::INVALID);
        ASSERT_EQ(message.first.MessageMetaData(), info.MessageMetaData());

        string result;
        message.second->ToString(result);
        ASSERT_STREQ(message.first.FileName().c_str(), info.FileName().c_str());
        ASSERT_EQ(message.first.LineNumber(), info.LineNumber());
        ASSERT_EQ(message.first.TimeStamp(), info.TimeStamp());
        ASSERT_STREQ(traceMessage.c_str(), result.c_str());
    }

    TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessageInOtherProcess)
    {
        const string traceMessage = _T("some trace");
        Messaging::TextMessage tm(traceMessage);
        Thunder::Core::Messaging::Information info(Thunder::Core::Messaging::MetaData::MessageType::TRACING,
            _T("some_category"),
            EXPAND_AND_QUOTE(MODULE_NAME),
            _T("some_file.cpp"),
            1337,
            Thunder::Core::Time::Now().Ticks());

        auto lambdaFunc = [&](IPTestAdministrator& testAdmin) {
            Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());
            client.AddInstance(0);
            Messaging::TraceFactory factory;
            client.AddFactory(Thunder::Core::Messaging::MetaData::MessageType::TRACING, &factory);
            testAdmin.Sync("setup");
            testAdmin.Sync("writer wrote");
            auto messages = client.PopMessagesAsList();

            ASSERT_EQ(messages.size(), 1);
            auto message = messages.front();

            ASSERT_NE(message.first.MessageMetaData().Type(), Thunder::Core::Messaging::MetaData::MessageType::INVALID);
            ASSERT_EQ(message.first.MessageMetaData(), info.MessageMetaData());

            string result;
            message.second->ToString(result);
            ASSERT_STREQ(message.first.FileName().c_str(), info.FileName().c_str());
            ASSERT_EQ(message.first.LineNumber(), info.LineNumber());
            ASSERT_EQ(message.first.TimeStamp(), info.TimeStamp());
            ASSERT_STREQ(traceMessage.c_str(), result.c_str());

            testAdmin.Sync("reader read");
            testAdmin.Sync("done");
        };

        static std::function<void(IPTestAdministrator&)> lambdaVar = lambdaFunc;
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) { lambdaVar(testAdmin); };
        IPTestAdministrator testAdmin(otherSide);

        {
            testAdmin.Sync("setup");
            testAdmin.Sync("writer wrote");
            Thunder::Core::Messaging::MessageUnit::Instance().Push(info, &tm);
            testAdmin.Sync("reader read");
        }
        testAdmin.Sync("done");
    }

} // Core
} // Tests
} // Thunder
