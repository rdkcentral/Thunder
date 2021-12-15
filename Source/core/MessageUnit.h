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
#pragma once
#include "JSON.h"
#include "MessageDispatcher.h"
#include "Module.h"
#include "Proxy.h"
#include "Sync.h"

namespace WPEFramework {
namespace Core {
    class EXTERNAL MessageInformation {
    public:
        enum MessageType : uint8_t {
            LOGGING,
            TRACING,
            WARNING_REPORTING,
            INVALID
        };
        MessageInformation();
        MessageInformation(MessageType type, string category, string filename, uint16_t lineNumber);

        MessageType Type() const;
        string Category() const;
        string FileName() const;
        uint16_t LineNumber() const;

        void Type(MessageType type);
        void Category(string category);
        void FileName(string filename);
        void LineNumber(uint16_t lineNumber);

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize);

    private:
        MessageType _type;
        string _category;
        string _filename;
        uint16_t _lineNumber;
    };

    struct EXTERNAL IMessageEvent {
        virtual ~IMessageEvent() = default;
        virtual uint16_t Serialize(uint8_t buffer[], const uint16_t length) const = 0;
        virtual uint16_t Deserialize(uint8_t buffer[], const uint16_t length) = 0;
        virtual void ToString(string& text) const = 0;
    };

    struct EXTERNAL IControl {
        virtual ~IControl() = default;
        virtual void Enable(bool enable) = 0;
        virtual bool Enable() const = 0;
        virtual void Destroy() = 0;

        virtual MessageInformation::MessageType Type() const = 0;
        virtual string Category() const = 0;

        virtual void Configure(const string&) {}
        virtual string Module() const
        {
            return _T("MODULE_UNKNOWN");
        }
    };

    struct EXTERNAL IMessageEventFactory {
        virtual ~IMessageEventFactory() = default;
        virtual Core::ProxyType<IMessageEvent> Create() = 0;
    };

    class TraceSetting : public Core::JSON::Container {
    public:
        TraceSetting& operator=(const TraceSetting&) = delete;
        TraceSetting()
            : Core::JSON::Container()
            , Module()
            , Category()
            , Enabled(false)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }
        TraceSetting(const TraceSetting& copy)
            : Core::JSON::Container()
            , Module(copy.Module)
            , Category(copy.Category)
            , Enabled(copy.Enabled)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }

        ~TraceSetting() override = default;

    public:
        Core::JSON::String Module;
        Core::JSON::String Category;
        Core::JSON::Boolean Enabled;
    };

    class EXTERNAL MessageUnit {

        struct pair_hash {
            template <class T1, class T2>
            std::size_t operator()(const std::pair<T1, T2>& pair) const
            {
                return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
            }
        };
        using ControlKey = std::pair<MessageInformation::MessageType, string>;
        using Controls = std::unordered_map<ControlKey, IControl*, pair_hash>;
        using Factories = std::unordered_map<MessageInformation::MessageType, Core::IMessageEventFactory*>;

    public:
        static constexpr uint32_t MetaDataSize = 1 * 1024;
        static constexpr uint32_t DataSize = 9 * 1024;
        static constexpr const char* MESSAGE_DISPATCHER_PATH_ENV = _T("MESSAGE_DISPATCHER_PATH");
        static constexpr const char* MESSAGE_DISPACTHER_IDENTIFIER_ENV = _T("MESSAGE_DISPACTHER_IDENTIFIER");
        using MessageDispatcher = Core::MessageDispatcherType<MetaDataSize, DataSize>;

        class Settings : public Core::JSON::Container {
        public:
            Settings& operator=(const Settings&) = delete;
            Settings()
                : Core::JSON::Container()
                , Tracing()
                , Logging()
                , WarningReporting(false)
            {
                Add(_T("tracing"), &Tracing);
                Add(_T("logging"), &Logging);
                Add(_T("warning_reporting"), &WarningReporting);
            }
            Settings(const Settings& other)
                : Core::JSON::Container()
                , Tracing(other.Tracing)
                , Logging(other.Logging)
                , WarningReporting(other.WarningReporting)
            {
                Add(_T("tracing"), &Tracing);
                Add(_T("logging"), &Logging);
                Add(_T("warning_reporting"), &WarningReporting);
            }

            ~Settings() override = default;

        public:
            Core::JSON::String Tracing;
            Core::JSON::String Logging;
            Core::JSON::String WarningReporting;
        };

    public:
        ~MessageUnit() = default;
        MessageUnit(const MessageUnit&) = delete;
        MessageUnit& operator=(const MessageUnit&) = delete;

    public:
        static MessageUnit& Instance();
        uint32_t Open(const uint32_t instanceId);
        uint32_t Open(const string& pathName);
        uint32_t Close();

        void Ring();

        void Defaults(const string& setting);
        string Defaults() const;
        void FetchDefaultSettingsForCategory(const IControl* control, bool& outIsEnabled, bool& outIsDefault);

        void Push(const MessageInformation& info, const IMessageEvent* message);

        void Announce(Core::MessageInformation::MessageType type, const string& category, IControl* control);
        void Revoke(Core::MessageInformation::MessageType type, const string& category);

    protected:
        MessageUnit() = default;

    private:
        mutable Core::CriticalSection _adminLock;
        std::unique_ptr<MessageDispatcher> _dispatcher;
        string _defaultSettings;

        Controls _controls;
        std::unordered_map<string, TraceSetting> _defaultTraceSettings;
    };

}
}