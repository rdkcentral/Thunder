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
#include <messaging/messaging.h>

using namespace WPEFramework;

class Control : public Core::Messaging::IControl {
public:
    Control(const Core::Messaging::Metadata& metaData)
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
    const Core::Messaging::Metadata& Metadata() const override
    {
        return _metaData;
    }

private:
    bool _isEnabled;
    Core::Messaging::Metadata _metaData;
};

class Core_Messaging_MessageUnit : public testing::Test {
protected:
    Core_Messaging_MessageUnit()
    {
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME) }));
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::TRACING, _T("Test_Category_2"), EXPAND_AND_QUOTE(MODULE_NAME) }));
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::TRACING, _T("Test_Category_3"), EXPAND_AND_QUOTE(MODULE_NAME) }));
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::TRACING, _T("Test_Category_4"), EXPAND_AND_QUOTE(MODULE_NAME) }));
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), _T("Test_Module2") }));
        _controls.emplace_back(new Control({ Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog") }));

        _activeConfig = false;
    }
    ~Core_Messaging_MessageUnit() = default;

    static void SetUpTestSuite()
    {
    }

    static void TearDownTestSuite()
    {
        Core::Singleton::Dispose();
    }

    void SetUp() override
    {
        AnnounceAllControls();

        ToggleDefaultConfig(true);

        _activeConfig = true;
    }

    void TearDown() override
    {
        RevokeAllControls();

        ToggleDefaultConfig(false);

        _activeConfig = false;
    }

    string DispatcherIdentifier()
    {
        return Messaging::MessageUnit::Instance().Identifier();
    }

    string DispatcherBasePath()
    {
        string result;
        return Messaging::MessageUnit::Instance().BasePath();
    }

    void AnnounceAllControls()
    {
        for (const auto& control : _controls) {
            // Only for 'controls' enabled in configuration
            Core::Messaging::IControl::Announce(control.get());
        }
    }

    void RevokeAllControls()
    {
        for (const auto& control : _controls) {
            Core::Messaging::IControl::Revoke(control.get());
        }
    }

    void ToggleDefaultConfig(bool activate)
    {
        ASSERT(_activeConfig != activate);

        if (!_activeConfig && activate) {
            Messaging::MessageUnit::Settings::Config configuration;
            Messaging::MessageUnit::Instance().Open(Core_Messaging_MessageUnit::_basePath, configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);
        }

        if (_activeConfig && !activate) {
            Messaging::MessageUnit::Instance().Close();
        }

        _activeConfig = !_activeConfig;
    }

    static bool _background;
    static string _basePath;
    std::list<std::unique_ptr<Core::Messaging::IControl>> _controls;

    bool _activeConfig;
};

bool Core_Messaging_MessageUnit::_background = false;
string Core_Messaging_MessageUnit::_basePath = _T("/tmp/");

TEST_F(Core_Messaging_MessageUnit, TraceMessageIsEnabledByDefaultWhenConfigFullySpecified)
{
    Messaging::MessageUnit::Settings::Config configuration;
    configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":true}]}})");

    Messaging::MessageUnit::Settings settings;
    settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
    EXPECT_TRUE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));
}

TEST_F(Core_Messaging_MessageUnit, TraceMessageIsDisabledByDefaultWhenConfigFullySpecified)
{
    Messaging::MessageUnit::Settings::Config configuration;
    configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","module":"Plugin_DeviceInfo","enabled":false}]}})");

    Messaging::MessageUnit::Settings settings;
    settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));
}

TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreEnabledWhenModuleNotSpecified)
{
    Messaging::MessageUnit::Settings::Config configuration;
    configuration.FromString(R"({"tracing":{"settings":[{"category":"Information","enabled":true}]}})");

    Messaging::MessageUnit::Settings settings;
    settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
    EXPECT_TRUE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
    EXPECT_TRUE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));
}

TEST_F(Core_Messaging_MessageUnit, TraceMessagesAreDisabledWhenModuleNotSpecified)
{
    Messaging::MessageUnit::Settings::Config configuration;
    configuration.FromString(R"({"tracing":{"messages":[{"category":"Information","enabled":false}]}})");

    Messaging::MessageUnit::Settings settings;
    settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("Information"), _T("Some_Module"));
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::TRACING, _T("SomeCategory"), _T("Plugin_DeviceInfo"));
    EXPECT_FALSE(settings.IsEnabled(metaData));
}

TEST_F(Core_Messaging_MessageUnit, LoggingMessageIsEnabledIfNotConfigured)
{
    Messaging::MessageUnit::Settings::Config configuration;
    configuration.FromString(R"({"logging":{"settings":[{"category":"Startup","module":"SysLog","enabled":false}]}})");

    Messaging::MessageUnit::Settings settings;
    settings.Configure(Core_Messaging_MessageUnit::_basePath, "SomeIdentifier", configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::LOGGING, _T("Startup"), _T("SysLog"));
    // Internal Metadata::Default() is true for LOGGING but here overwritten because of element of config
    EXPECT_FALSE(settings.IsEnabled(metaData));

    metaData = Core::Messaging::Metadata(Core::Messaging::Metadata::type::LOGGING, _T("Notification"), _T("SysLog"));
    // Internal Metadata::Default() is true for LOGGING and not overwritten because of no element of config
    EXPECT_TRUE(settings.IsEnabled(metaData));
}

TEST_F(Core_Messaging_MessageUnit, MessageClientWillReturnListOfControls)
{
    //this test is using metadata (IPC) passing, so no other proces tests for now
    Messaging::MessageClient client(Messaging::MessageUnit::Instance().Identifier(), Messaging::MessageUnit::Instance().BasePath());

    client.AddInstance(0 /*id*/); //we are in framework

    Messaging::MessageUnit::Iterator it;

    client.Controls(it);

    int matches = 0;
    int count = 0;
    while (it.Next()) {
        if (it.Module() == EXPAND_AND_QUOTE(MODULE_NAME)) {
            ++matches;
        }
        ++count;
    }

    client.RemoveInstance(0);

    EXPECT_GE(count, 4);
    EXPECT_EQ(matches, 4);
}

TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldUpdateExistingDefaultConfig)
{
    // Reload with new configuration
    ToggleDefaultConfig(false);

    Messaging::MessageUnit::Settings::Config configuration;

    // If 'enabled' equals false the entry is not added to 'Settings'
    configuration.FromString(R"({"tracing":{"settings":[{"category":"ExampleCategory","module":"ExampleModule","enabled":false}]}})");

    // Populate settings with specified configuration
    Messaging::MessageUnit::Instance().Open(Core_Messaging_MessageUnit::_basePath, configuration, Core_Messaging_MessageUnit::_background, Messaging::MessageUnit::OFF);

    const Core::Messaging::Metadata toBeUpdated(Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

    Control control(toBeUpdated);
    // Add to the internal list if it is not already
    Core::Messaging::IControl::Announce(&control);

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    // Creates a MessageUnit::Client internally with the id passed in
    client.AddInstance(0); //we are in framework

    // Get the system 'status'
    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    bool enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   toBeUpdated.Type() == it.Type()
                      && toBeUpdated.Category() == it.Category()
                      && toBeUpdated.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_FALSE(enabled);

    // Enable message via metadata, eg, set enable for the previously added Control, eg, enable category
    client.Enable(toBeUpdated, true);

    client.Controls(it);

    /* bool */ enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   toBeUpdated.Type() == it.Type()
                      && toBeUpdated.Category() == it.Category()
                      && toBeUpdated.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);

    Core::Messaging::IControl::Revoke(&control);

    Messaging::MessageUnit::Instance().Close();

    ToggleDefaultConfig(true);
}

TEST_F(Core_Messaging_MessageUnit, EnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
{
    const Core::Messaging::Metadata toBeAdded(Core::Messaging::Metadata::type::TRACING, _T("ExampleCategory"), _T("ExampleModule"));

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    bool enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   toBeAdded.Type() == it.Type()
                      && toBeAdded.Category() == it.Category()
                      && toBeAdded.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_FALSE(enabled);

    Control control(toBeAdded);

    Core::Messaging::IControl::Announce(&control);

    client.Controls(it);

    enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   toBeAdded.Type() == it.Type()
                      && toBeAdded.Category() == it.Category()
                      && toBeAdded.Module() == it.Module()
                    )
                 ;
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);

    Core::Messaging::IControl::Revoke(&control);
}

TEST_F(Core_Messaging_MessageUnit, EnablingMessagesByTypeShouldEnableEverything)
{
    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    bool enabled = true;

    while (it.Next()) {
        enabled =    enabled
                  && it.Enabled()
                 ;
    }

    // Controls from the default are disabled by default, except a few
    EXPECT_FALSE(enabled);

    // Enable message via metadata, eg, set enable for the previously added Control, eg, enable category
    client.Enable({Core::Messaging::Metadata::type::TRACING, _T(""), _T("")}, true);

    client.Controls(it);

    enabled = true;

    while (it.Next()) {
        enabled =    enabled
                  && it.Enabled()
                 ;
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, LogMessagesCanToggledWhenLogModuleSpecified)
{
    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Core::Messaging::Metadata messageToToggle(Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog"));

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    int matches = 0;
    while (it.Next()) {
        if (   it.Type() == messageToToggle.Type()
            && it.Category() == messageToToggle.Category()
            && it.Module() == messageToToggle.Module()
            && it.Enabled()
           ) {
            ++matches;
        }
    }

    EXPECT_EQ(matches, 1);

    client.Enable(messageToToggle, false);

    client.Controls(it);

    matches = 0;

    while (it.Next()) {
        if (   it.Type() == messageToToggle.Type()
            && it.Category() == messageToToggle.Category()
            && it.Module() == messageToToggle.Module()
            && !it.Enabled()
           ) {
            ++matches;
        }
    }

    EXPECT_EQ(matches, 1);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, LogEnablingMessagesShouldAddToDefaultConfigListIfNotPresent)
{
    const Core::Messaging::Metadata toBeAdded(Core::Messaging::Metadata::type::LOGGING, _T("Test_Category_5"), _T("SysLog"));

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    // LOGGING is enabled and available by default

    Messaging::MessageUnit::Iterator it;

    client.Controls(it);

    bool enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   toBeAdded.Type() == it.Type()
                      && toBeAdded.Category() == it.Category()
                      && toBeAdded.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, EnablingFullySpecifiedMessageUpdateOnlyThisOne)
{
    Core::Messaging::Metadata message(Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), EXPAND_AND_QUOTE(MODULE_NAME));

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    // TRACING is not enabled but available by default

    Messaging::MessageUnit::Iterator it;

    client.Controls(it);

    bool enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   message.Type() == it.Type()
                      && message.Category() == it.Category()
                      && message.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_FALSE(enabled);

    client.Enable(message, true);

    client.Enable(message, true);

    client.Controls(it);

    enabled = false;

    while (it.Next()) {
        enabled =    enabled
                  ||
                     (   message.Type() == it.Type()
                      && message.Category() == it.Category()
                      && message.Module() == it.Module()
                      && it.Enabled()
                    )
                 ;
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByModuleShouldEnableAllCategoriesInsideIt)
{
    const Core::Messaging::Metadata message(Core::Messaging::Metadata::type::TRACING, _T(""), EXPAND_AND_QUOTE(MODULE_NAME));

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    bool enabled = true;

    while (it.Next()) {
        if (   message.Type() == it.Type()
            && message.Module() == it.Module()
        ) {
            enabled =    enabled
                      && it.Enabled()
                     ;
        }
    }

    EXPECT_FALSE(enabled);

    client.Enable(message, true);

    client.Controls(it);

    enabled = true;

    while (it.Next()) {
        if (   message.Type() == it.Type()
            && message.Module() == it.Module()
        ) {
            enabled =    enabled
                      && it.Enabled()
                     ;
        }
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, EnablingMessageSpecifiedByCategoryShouldEnableItInAllModules)
{
    const Core::Messaging::Metadata message(Core::Messaging::Metadata::type::TRACING, _T("Test_Category_1"), _T(""));

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    bool enabled = true;

    while (it.Next()) {
        if (   message.Type() == it.Type()
            && message.Category() == it.Category()
        ) {
            enabled =    enabled
                      && it.Enabled()
                     ;
        }
    }

    EXPECT_FALSE(enabled);

    client.Enable(message, true);

    client.Controls(it);

    enabled = true;

    while (it.Next()) {
        if (   message.Type() == it.Type()
            && message.Category() == it.Category()
        ) {
            enabled =    enabled
                      && it.Enabled()
                     ;
        }
    }

    EXPECT_TRUE(enabled);

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, TextMessageEventIsProperlySerializedIfBufferBigEnough)
{
    constexpr string::size_type bufferSize = 1024;

    uint8_t buffer[bufferSize];
    const string testTextMessage = _T("TEST MESSAGE");

    EXPECT_GT(bufferSize, sizeof(testTextMessage.size()));

    Messaging::TextMessage tm(testTextMessage);
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

    Messaging::TextMessage tm(testTextMessage);
    auto serialized = tm.Serialize(buffer, sizeof(buffer));
    EXPECT_GT(serialized, 0);

    auto deserialized = tm.Deserialize(buffer, serialized);
    EXPECT_EQ(serialized, deserialized);

    string result = tm.Data();

    EXPECT_STREQ(result.c_str(), testTextMessage.substr(0, bufferSize - 1).c_str());
}

TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferBigEnough)
{
    constexpr string::size_type bufferSize = 1024;

    uint8_t buffer[bufferSize];

    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    while (it.Next()) {
        Messaging::MessageUnit::Control control({it.Type(), it.Category(), it.Module()}, it.Enabled());
        auto serialized = control.Serialize(buffer, sizeof(buffer));

        EXPECT_GT(serialized, 0);

        auto deserialized = control.Deserialize(buffer, serialized);

        EXPECT_EQ(serialized, deserialized);
    }

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, ControlListIsProperlySerializedIfBufferNotBigEnough)
{
    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    Messaging::MessageUnit::Iterator it;
    client.Controls(it);

    std::vector<uint8_t> buffer;

    while (it.Next()) {
        buffer.resize(buffer.size() + sizeof(it.Type()), Core::Messaging::Metadata::type::INVALID);
        buffer.resize(buffer.size() + it.Category().size() + 1, Core::Messaging::Metadata::type::INVALID);
        buffer.resize(buffer.size() + it.Module().size() + 1, Core::Messaging::Metadata::type::INVALID);
        buffer.resize(buffer.size() + sizeof(bool), Core::Messaging::Metadata::type::INVALID);
    }

    buffer.resize(buffer.size() + 1);

    uint16_t index = 0;

    client.Controls(it);

    while (it.Next()) {
        Messaging::MessageUnit::Control control({it.Type(), it.Category(), it.Module()}, it.Enabled());
        auto serialized = control.Serialize(&(buffer.data()[index]), buffer.size());

        EXPECT_GT(serialized, 0);

        EXPECT_GT(buffer.size(), serialized);

        auto deserialized = control.Deserialize(&buffer.data()[index], serialized);

        index += serialized;

        EXPECT_EQ(serialized, deserialized);
    }

    EXPECT_LT(index, buffer.size());

    client.RemoveInstance(0);
}

TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessage)
{
    Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

    client.AddInstance(0); //we are in framework

    //factory should be added before attempting to pop data
    Messaging::TraceFactoryType<Core::Messaging::IStore::Tracing, Messaging::TextMessage> factory;
    client.AddFactory(Core::Messaging::Metadata::type::TRACING, &factory);

    Core::Messaging::Metadata metadata(Core::Messaging::Metadata::type::TRACING, _T("some_category"), EXPAND_AND_QUOTE(MODULE_NAME));

    client.Enable(metadata, true);

    const string traceMessage = _T("some trace");
    Messaging::TextMessage tm(traceMessage);

    Core::Messaging::IStore::Tracing info(Core::Messaging::MessageInfo(metadata, Core::Time::Now().Ticks()), _T("some_file"), 1337, EXPAND_AND_QUOTE(MODULE_NAME));

    Messaging::MessageUnit::Instance().Push(info, &tm);

    // Risk of blocking or unknown suitable 'waittime'
    //client.WaitForUpdates(Core::infinite);
    // Instead 'flush' and continue
    client.SkipWaiting();

    bool present = false;

    client.PopMessagesAndCall(
        [&](const Core::ProxyType<Core::Messaging::MessageInfo>& metadata, const Core::ProxyType<Core::Messaging::IEvent>& message) {
            //(*metadata).TimeStamp();
            //(*metadata).Module();
            //(*metadata).Category();

            if ((*metadata).Type() == Core::Messaging::Metadata::type::TRACING) {
                TRACE_L1(
                    _T("PopMessagesAndCall : Tracing message -> Filename : %s, Linenumber : %d, Classname : %s")
                    , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).FileName().c_str()
                    , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).LineNumber()
                    , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).ClassName().c_str()
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
}

TEST_F(Core_Messaging_MessageUnit, PopMessageShouldReturnLastPushedMessageInOtherProcess)
{
    const string traceMessage = _T("some trace");

    Core::Messaging::Metadata metadata(Core::Messaging::Metadata::type::TRACING, _T("some_category"), EXPAND_AND_QUOTE(MODULE_NAME));

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

                        Messaging::MessageClient client(DispatcherIdentifier(), DispatcherBasePath() /*, socketPort not specified, domain socket used instead */);

                        client.AddInstance(0);

                        Messaging::TraceFactoryType<Core::Messaging::IStore::Tracing, Messaging::TextMessage> factory;
                        client.AddFactory(Core::Messaging::Metadata::type::TRACING, &factory);

                        client.Enable(metadata, true);

                        client.WaitForUpdates(Core::infinite);
//                        client.SkipWaiting();

                        client.PopMessagesAndCall(
                            [&](const Core::ProxyType<Core::Messaging::MessageInfo>& metadata, const Core::ProxyType<Core::Messaging::IEvent>& message) {
                                if ((*metadata).Type() == Core::Messaging::Metadata::type::TRACING) {
                                    TRACE_L1(
                                        _T("PopMessagesAndCall : Tracing message -> Filename : %s, Linenumber : %d, Classname : %s")
                                        , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).FileName().c_str()
                                        , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).LineNumber()
                                        , static_cast<Core::Messaging::IStore::Tracing&>(*metadata).ClassName().c_str()
                                    );

                                    EXPECT_TRUE((*message).Data() == traceMessage);
                                }
                            }
                        );

                        client.RemoveFactory(Core::Messaging::Metadata::TRACING);

                        client.RemoveInstance(0);

                        break;
                    }
        default :   // parent
                    {
                        ASSERT_FALSE(sigprocmask(SIG_UNBLOCK, &sigset, nullptr) == -1);

                        Messaging::TextMessage tm(traceMessage);

                        Core::Messaging::IStore::Tracing info(Core::Messaging::MessageInfo(metadata, Core::Time::Now().Ticks()), _T("some_file"), 1337, EXPAND_AND_QUOTE(MODULE_NAME));

                        Messaging::MessageUnit::Instance().Push(info, &tm);

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
                    }
    }
}
