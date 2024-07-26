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

#include <messaging/messaging.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    class Control : public ::Thunder::Core::Messaging::IControl {
    public:
        Control(const ::Thunder::Core::Messaging::Metadata& metaData)
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
        const ::Thunder::Core::Messaging::Metadata& Metadata() const override
        {
            return _metaData;
        }

    private:
        bool _isEnabled;
        ::Thunder::Core::Messaging::Metadata _metaData;
    };

    class Core_Messaging_MessageUnit : public testing::Test {
    protected:
        Core_Messaging_MessageUnit()
        {
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_2"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_3"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_4"), EXPAND_AND_QUOTE(MODULE_NAME) }));
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), _T("Test_Module2") }));
            _controls.emplace_back(new Control({ ::Thunder::Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog") }));

        }
        ~Core_Messaging_MessageUnit() = default;

        static void SetUpTestSuite()
        {
        }

        static void TearDownTestSuite()
        {
            ::Thunder::Core::Singleton::Dispose();
        }

        void SetUp() override
        {
        }

        void TearDown() override
        {
        }

        string DispatcherIdentifier()
        {
            return ::Thunder::Messaging::MessageUnit::Instance().Identifier();
        }

        string DispatcherBasePath()
        {
            string result;
            return ::Thunder::Messaging::MessageUnit::Instance().BasePath();
        }

        void AnnounceAllControls()
        {
            for (const auto& control : _controls) {
                // Only for 'controls' enabled in configuration
                ::Thunder::Core::Messaging::IControl::Announce(control.get());
            }
        }

        void RevokeAllControls()
        {
            for (const auto& control : _controls) {
                ::Thunder::Core::Messaging::IControl::Revoke(control.get());
            }
        }

        void ToggleDefaultConfig(bool activate)
        {
            ASSERT(_activeConfig != activate);

            if (!_activeConfig && activate) {
                ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
                ::Thunder::Messaging::MessageUnit::Instance().Open(Core_Messaging_MessageUnit::_basePath, configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);
            }

            if (_activeConfig && !activate) {
                ::Thunder::Messaging::MessageUnit::Instance().Close();
            }

            _activeConfig = !_activeConfig;
        }

        static bool _background;
        static string _basePath;
        std::list<std::unique_ptr<::Thunder::Core::Messaging::IControl>> _controls;

        bool _activeConfig;
    };

    bool Core_Messaging_MessageUnit::_background = false;
    string Core_Messaging_MessageUnit::_basePath = _T("/tmp/");

    TEST_F(Core_Messaging_MessageUnit, TraceMessageIsEnabledByDefaultWhenConfigFullySpecified)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":true}]}})");

        ::Thunder::Messaging::MessageUnit::Settings settings;
        settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        ::Thunder::Core::Messaging::Metadata metaData(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
        EXPECT_TRUE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessageIsDisabledByDefaultWhenConfigFullySpecified)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":false}]}})");

        ::Thunder::Messaging::MessageUnit::Settings settings;
        settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        ::Thunder::Core::Messaging::Metadata metaData(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreEnabledWhenModuleNotSpecified)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","enabled":true}]}})");

        ::Thunder::Messaging::MessageUnit::Settings settings;
        settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        ::Thunder::Core::Messaging::Metadata metaData(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
        EXPECT_TRUE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
        EXPECT_TRUE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));
    }

    TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreDisabledWhenModuleNotSpecified)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        configuration.FromString(R"({"tracing":{"messages":[{"category":"Information","enabled":false}]}})");

        ::Thunder::Messaging::MessageUnit::Settings settings;
        settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        ::Thunder::Core::Messaging::Metadata metaData(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
        EXPECT_FALSE(settings.IsEnabled(metaData));
    }

    TEST_F(Core_Messaging_MessageUnit, LoggingMessageIsEnabledIfNotConfigured)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;
        configuration.FromString(R"({"logging":{"settings":[{"category":"Startup","module":"SysLog","enabled":false}]}})");

        ::Thunder::Messaging::MessageUnit::Settings settings;
        settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        ::Thunder::Core::Messaging::Metadata metaData(::Thunder::Core::Messaging::Metadata::type::LOGGING, _T("Startup"), _T("SysLog"));
        // Internal Metadata::Default() is true for LOGGING but here overwritten because of element of config
        EXPECT_FALSE(settings.IsEnabled(metaData));

        metaData = ::Thunder::Core::Messaging::Metadata(::Thunder::Core::Messaging::Metadata::type::LOGGING, _T("Notification"), _T("SysLog"));
        // Internal Metadata::Default() is true for LOGGING and not overwritten because of no element of config
        EXPECT_TRUE(settings.IsEnabled(metaData));
    }

    TEST_F(Core_Messaging_MessageUnit, MessageClientWillReturnListOfControls)
    {
        ToggleDefaultConfig(true);

        //this test is using metadata (IPC) passing, so no other proces tests for now
        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath());

        client.AddInstance(0 /*id*/); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);

        int matches = 0;
        int count = 0;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            if (*it == EXPAND_AND_QUOTE(MODULE_NAME)) {
                ++matches;
            }
            ++count;
        }

        client.RemoveInstance(0);

        EXPECT_GE(count, 2);
        EXPECT_EQ(matches, 0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldUpdateExistingDefaultConfig)
    {
        ::Thunder::Messaging::MessageUnit::Settings::Config configuration;

        // If 'enabled' equals false the entry is not added to 'Settings'
        configuration.FromString(R"({"tracing":{"settings":[{"category":"ExampleCategory","module":"ExampleModule","enabled":false}]}})");

        // Populate settings with specified configuration
        ::Thunder::Messaging::MessageUnit::Instance().Open(Core_Messaging_MessageUnit::_basePath, configuration, Core_Messaging_MessageUnit::_background, ::Thunder::Messaging::MessageUnit::OFF);

        const ::Thunder::Core::Messaging::Metadata toBeUpdated(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        Control control(toBeUpdated);
        // Add to the internal list if it is not already
        ::Thunder::Core::Messaging::IControl::Announce(&control);

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        // Creates a MessageUnit::Client internally with the id passed in
        client.AddInstance(0); //we are in framework

        // Get the system 'status'

        bool enabled = false;

        std::vector<std::string> modules;
        client.Modules(modules);

        int matches = 0;
        int count = 0;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   toBeUpdated.Type() == item.Type()
                              && toBeUpdated.Category() == item.Category()
                              && toBeUpdated.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_FALSE(enabled);

        // Enable message via metadata, eg, set enable for the previously added Control, eg, enable category
        client.Enable(toBeUpdated, true);

        modules.clear();
        client.Modules(modules);

        /* bool */ enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;
 
            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   toBeUpdated.Type() == item.Type()
                              && toBeUpdated.Category() == item.Category()
                              && toBeUpdated.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_TRUE(enabled);

        ::Thunder::Core::Messaging::IControl::Revoke(&control);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
    {
        ToggleDefaultConfig(true);

        const ::Thunder::Core::Messaging::Metadata toBeAdded(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);
//missing operational stream
        bool enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;
 
            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   toBeAdded.Type() == item.Type()
                              && toBeAdded.Category() == item.Category()
                              && toBeAdded.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_FALSE(enabled);

        Control control(toBeAdded);

        ::Thunder::Core::Messaging::IControl::Announce(&control);

        modules.clear();
        client.Modules(modules);

        enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;
 
            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   toBeAdded.Type() == item.Type()
                              && toBeAdded.Category() == item.Category()
                              && toBeAdded.Module() == item.Module()
                            )
                         ;
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ::Thunder::Core::Messaging::IControl::Revoke(&control);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessagesByTypeShouldEnableEverything)
    {
        ToggleDefaultConfig(true);

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);
//empty
        bool enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          && item.Enabled()
                         ;
            }
        }

        // Controls from the default are disabled by default, except a few
        EXPECT_FALSE(enabled);

        // Enable message via metadata, eg, set enable for the previously added Control, eg, enable category
        client.Enable({::Thunder::Core::Messaging::Metadata::type::TRACING, _T(""), _T("")}, true);

        modules.clear();
        client.Modules(modules);

        enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          && item.Enabled()
                         ;
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, LogMessagesCanToggledWhenLogModuleSpecified)
    {
        ToggleDefaultConfig(true);

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        ::Thunder::Core::Messaging::Metadata messageToToggle(::Thunder::Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog"));

        std::vector<std::string> modules;
        client.Modules(modules);

        int matches = 0;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   item.Type() == messageToToggle.Type()
                    && item.Category() == messageToToggle.Category()
                    && item.Module() == messageToToggle.Module()
                    && item.Enabled()
                   ) {
                    ++matches;
                }
            }
        }

        EXPECT_EQ(matches, 1);

        client.Enable(messageToToggle, false);

        modules.clear();
        client.Modules(modules);

        matches = 0;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   item.Type() == messageToToggle.Type()
                    && item.Category() == messageToToggle.Category()
                    && item.Module() == messageToToggle.Module()
                    && !item.Enabled()
                   ) {
                    ++matches;
                }
            }
        }

        EXPECT_EQ(matches, 1);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, LogEnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
    {
        ToggleDefaultConfig(true);

        const ::Thunder::Core::Messaging::Metadata toBeAdded(::Thunder::Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog"));

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        // LOGGING is enabled and available by default

        std::vector<std::string> modules;
        client.Modules(modules);

        bool enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   toBeAdded.Type() == item.Type()
                              && toBeAdded.Category() == item.Category()
                              && toBeAdded.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingFullySpecifiedMessageUpdateOnlyThisOne)
    {
        ToggleDefaultConfig(true);

        ::Thunder::Core::Messaging::Metadata message(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME));

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        // TRACING is not enabled but available by default

        std::vector<std::string> modules;
        client.Modules(modules);

        bool enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   message.Type() == item.Type()
                              && message.Category() == item.Category()
                              && message.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_FALSE(enabled);

        client.Enable(message, true);

        client.Enable(message, true);

        modules.clear();
        client.Modules(modules);

        enabled = false;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                enabled =    enabled
                          ||
                             (   message.Type() == item.Type()
                              && message.Category() == item.Category()
                              && message.Module() == item.Module()
                              && item.Enabled()
                            )
                         ;
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByModuleShouldEnableAllCategoriesInsideIt)
    {
        ToggleDefaultConfig(false);

        const ::Thunder::Core::Messaging::Metadata message(::Thunder::Core::Messaging::Metadata::type::TRACING, _T(""), EXPAND_AND_QUOTE(MODULE_NAME));

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);

        bool enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   message.Type() == item.Type()
                    && message.Module() == item.Module()
                ) {
                    enabled =    enabled
                              && item.Enabled()
                             ;
                }
            }
        }

        EXPECT_FALSE(enabled);

        client.Enable(message, true);

        modules.clear();
        client.Modules(modules);

        enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   message.Type() == item.Type()
                    && message.Module() == item.Module()
                ) {
                    enabled =    enabled
                              && item.Enabled()
                             ;
                }
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByCategoryShouldEnableItInAllModules)
    {
        ToggleDefaultConfig(true);

        const ::Thunder::Core::Messaging::Metadata message(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), _T(""));

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);

        bool enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   message.Type() == item.Type()
                    && message.Category() == item.Category()
                ) {
                    enabled =    enabled
                              && item.Enabled()
                             ;
                }
            }
        }

        EXPECT_FALSE(enabled);

        client.Enable(message, true);

        modules.clear();
        client.Modules(modules);

        enabled = true;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                if (   message.Type() == item.Type()
                    && message.Category() == item.Category()
                ) {
                    enabled =    enabled
                              && item.Enabled()
                             ;
                }
            }
        }

        EXPECT_TRUE(enabled);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, TextMessageEventIsProperlySerializedIfBufferBigEnough)
    {
        constexpr string::size_type bufferSize = 1024;

        uint8_t buffer[bufferSize];
        const string testTextMessage = _T("TEST MESSAGE");

        EXPECT_GT(bufferSize, sizeof(testTextMessage.size()));

        ::Thunder::Messaging::TextMessage tm(testTextMessage);
        auto serialized = tm.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0);

        auto deserialized = tm.Deserialize(buffer, sizeof(buffer));
        EXPECT_EQ(serialized, deserialized);

        string result = tm.Data();
        EXPECT_STREQ(result.c_str(), testTextMessage.c_str());
    }

    TEST_F(Core_Messaging_MessageUnit, TextMessageEventIsProperlySerializedAndCutIfBufferNotBigEnough)
    {
        constexpr string::size_type bufferSize = 5;

        uint8_t buffer[bufferSize];
        const string testTextMessage = _T("abcdefghi");

        EXPECT_LT(bufferSize, sizeof(testTextMessage.size()));

        ::Thunder::Messaging::TextMessage tm(testTextMessage);
        auto serialized = tm.Serialize(buffer, sizeof(buffer));
        EXPECT_GT(serialized, 0);

        auto deserialized = tm.Deserialize(buffer, serialized);
        EXPECT_EQ(serialized, deserialized);

        string result = tm.Data();

        EXPECT_STREQ(result.c_str(), testTextMessage.substr(0, bufferSize - 1).c_str());
    }

    TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferBigEnough)
    {
        ToggleDefaultConfig(true);

        constexpr string::size_type bufferSize = 1024;

        uint8_t buffer[bufferSize];

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                ::Thunder::Messaging::MessageUnit::Control control({item.Type(), item.Category(), item.Module()}, item.Enabled());
                auto serialized = control.Serialize(buffer, sizeof(buffer));

                EXPECT_GT(serialized, 0);

                auto deserialized = control.Deserialize(buffer, serialized);

                EXPECT_EQ(serialized, deserialized);
            }
        }

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferNotBigEnough)
    {
        ToggleDefaultConfig(true);

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        std::vector<std::string> modules;
        client.Modules(modules);

        std::vector<uint8_t> buffer;

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                buffer.resize(buffer.size() + sizeof(item.Type()), ::Thunder::Core::Messaging::Metadata::type::INVALID);
                buffer.resize(buffer.size() + item.Category().size() + 1, ::Thunder::Core::Messaging::Metadata::type::INVALID);
                buffer.resize(buffer.size() + item.Module().size() + 1, ::Thunder::Core::Messaging::Metadata::type::INVALID);
                buffer.resize(buffer.size() + sizeof(bool), ::Thunder::Core::Messaging::Metadata::type::INVALID);
            }
        }

        buffer.resize(buffer.size() + 1);

        uint16_t index = 0;

        modules.clear();
        client.Modules(modules);

        for (auto it = modules.begin(), end = modules.end(); it != end; it++) {
            ::Thunder::Messaging::MessageUnit::Iterator item;

            client.Controls(item, *it);

            while (item.Next()) {
                ::Thunder::Messaging::MessageUnit::Control control({item.Type(), item.Category(), item.Module()}, item.Enabled());
                auto serialized = control.Serialize(&(buffer.data()[index]), buffer.size());

                EXPECT_GT(serialized, 0);

                EXPECT_GT(buffer.size(), serialized);

                auto deserialized = control.Deserialize(&buffer.data()[index], serialized);

                index += serialized;

                EXPECT_EQ(serialized, deserialized);
            }
        }

        EXPECT_LT(index, buffer.size());

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

    TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessage)
    {
        ToggleDefaultConfig(true);

        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

        client.AddInstance(0); //we are in framework

        //factory should be added before attempting to pop data
        ::Thunder::Messaging::TraceFactoryType<::Thunder::Core::Messaging::IStore::Tracing, ::Thunder::Messaging::TextMessage> factory;
        client.AddFactory(::Thunder::Core::Messaging::Metadata::type::TRACING, &factory);

        ::Thunder::Core::Messaging::Metadata metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("some_category"), EXPAND_AND_QUOTE(MODULE_NAME));

        client.Enable(metadata, true);

        const string traceMessage = _T("some trace");
        ::Thunder::Messaging::TextMessage tm(traceMessage);

        ::Thunder::Core::Messaging::IStore::Tracing info(::Thunder::Core::Messaging::MessageInfo(metadata, ::Thunder::Core::Time::Now().Ticks()), _T("some_file"), 1337, EXPAND_AND_QUOTE(MODULE_NAME));

        ::Thunder::Messaging::MessageUnit::Instance().Push(info, &tm);

        // Risk of blocking or unknown suitable 'waittime'
        //client.WaitForUpdates(::Thunder::Core::infinite);
        // Instead 'flush' and continue
        client.SkipWaiting();

        bool present = false;

        client.PopMessagesAndCall(
            [&](const ::Thunder::Core::ProxyType<::Thunder::Core::Messaging::MessageInfo>& metadata, const ::Thunder::Core::ProxyType<::Thunder::Core::Messaging::IEvent>& message) {
                //(*metadata).TimeStamp();
                //(*metadata).Module();
                //(*metadata).Category();

                if ((*metadata).Type() == ::Thunder::Core::Messaging::Metadata::type::TRACING) {
                    TRACE_L1(
                        _T("PopMessagesAndCall : Tracing message -> Filename : %s, Linenumber : %d, Classname : %s")
                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).FileName().c_str()
                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).LineNumber()
                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).ClassName().c_str()
                    );

                    present = present || (*message).Data() == traceMessage;
                } else {
                    TRACE_L1(_T("PopMessagesAndCall : Unknown message"));
                }

                // By defining a callback data could be further processed
            }
        );

        EXPECT_TRUE(present);

        client.RemoveInstance(0);

        ToggleDefaultConfig(false);
    }

TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessageInOtherProcess)
{
    const string traceMessage = _T("some trace");

    ::Thunder::Core::Messaging::Metadata metadata(::Thunder::Core::Messaging::Metadata::type::TRACING, _T("some_category"), EXPAND_AND_QUOTE(MODULE_NAME));

    // Make sure the parent does not miss out on the signal if the child completes prematurely
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);

    // Do not continue if it is not guaranteed a child can be killed / ended
    ASSERT_FALSE(sigprocmask(SIG_BLOCK, &sigset, nullptr) == -1);

    pid_t pid = fork();

    switch(pid) {
    case  -1    :   // error
                    {
                        EXPECT_TRUE(pid != -1);
                        break;
                    }
    case 0      :   // child
                    {
                        ASSERT_FALSE(sigprocmask(SIG_UNBLOCK, &sigset, nullptr) == -1);

                        ::Thunder::Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

                        client.AddInstance(0);

                        ::Thunder::Messaging::TraceFactoryType<::Thunder::Core::Messaging::IStore::Tracing, ::Thunder::Messaging::TextMessage> factory;
                        client.AddFactory(::Thunder::Core::Messaging::Metadata::type::TRACING, &factory);

                        client.Enable(metadata, true);

                        client.WaitForUpdates(::Thunder::Core::infinite);
//                        client.SkipWaiting();

                        client.PopMessagesAndCall(
                            [&](const ::Thunder::Core::ProxyType<::Thunder::Core::Messaging::MessageInfo>& metadata, const ::Thunder::Core::ProxyType<::Thunder::Core::Messaging::IEvent>& message) {
                                if ((*metadata).Type() == ::Thunder::Core::Messaging::Metadata::type::TRACING) {
                                    TRACE_L1(
                                        _T("PopMessagesAndCall : Tracing message -> Filename : %s, Linenumber : %d, Classname : %s")
                                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).FileName().c_str()
                                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).LineNumber()
                                        , static_cast<::Thunder::Core::Messaging::IStore::Tracing&>(*metadata).ClassName().c_str()
                                    );

                                    EXPECT_TRUE((*message).Data() == traceMessage);
                                }
                            }
                        );

                        client.RemoveFactory(::Thunder::Core::Messaging::Metadata::TRACING);

                        client.RemoveInstance(0);

                        break;
                    }
        default :   // parent
                    {
                        ASSERT_FALSE(sigprocmask(SIG_UNBLOCK, &sigset, nullptr) == -1);

                        ToggleDefaultConfig(true);

                        ::Thunder::Messaging::TextMessage tm(traceMessage);

                        ::Thunder::Core::Messaging::IStore::Tracing info(::Thunder::Core::Messaging::MessageInfo(metadata, ::Thunder::Core::Time::Now().Ticks()), _T("some_file"), 1337, EXPAND_AND_QUOTE(MODULE_NAME));

                        ::Thunder::Messaging::MessageUnit::Instance().Push(info, &tm);

                        struct timespec timeout;
                        timeout.tv_sec = 60; // Arbitrary value
                        timeout.tv_nsec = 0;

                        do {
                            if (sigtimedwait(&sigset, nullptr, &timeout) == -1) {
                                int err = errno;
                                if (err == EINTR) {
                                    // Signal other than SIGCHLD
                                    continue;
                                } else if (err == EAGAIN) {
                                    // Timeout and no SIGCHLD received
                                    // Kill the child
                                    EXPECT_FALSE(kill(pid, SIGKILL) == -1);
                                } else {
                                    // Error in executing sigtimedwait, 'abort'
                                    EXPECT_FALSE(err == 0);
                                }
                            }

                            break;
                        } while(waitpid(-1, nullptr, WNOHANG) <= 0);

                        ToggleDefaultConfig(false);
                    }
    }
}

} // Core
} // Tests
} // Thunder
