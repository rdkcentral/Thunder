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
    /**
     * @brief Data-Carrier class storing information about basic information about the Message.
     * 
     */
    class EXTERNAL MessageMetaData {
    public:
        enum MessageType : uint8_t {
            TRACING = 0,
            LOGGING = 1,
            WARNING_REPORTING = 2,
            INVALID = 3
        };

        MessageMetaData();
        MessageMetaData(const MessageType type, const string& category, const string& module);
        MessageMetaData(const MessageMetaData&) = default;
        MessageMetaData& operator=(const MessageMetaData&) = default;
        inline bool operator==(const MessageMetaData& other) const
        {
            return _type == other._type && _category == other._category && _module == other._module;
        }
        inline bool operator!=(const MessageMetaData& other) const
        {
            return !operator==(other);
        }

        inline MessageType Type() const
        {
            return _type;
        }
        inline string Category() const
        {
            return _category;
        }
        inline string Module() const
        {
            return _module;
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize);

    private:
        MessageType _type;
        string _category;
        string _module;
    };

    /**
     * @brief Data-Carrier, extended information about the message
     * 
     */
    class EXTERNAL MessageInformation {
    public:
        MessageInformation() = default;
        MessageInformation(const MessageMetaData::MessageType type, const string& category, const string& module,
            const string& filename, uint16_t lineNumber, const uint64_t timestamp);
        MessageInformation(const MessageInformation&) = default;
        MessageInformation& operator=(const MessageInformation&) = default;

        inline const MessageMetaData& MetaData() const
        {
            return _metaData;
        }
        inline string FileName() const
        {
            return _filename;
        }
        inline uint16_t LineNumber() const
        {
            return _lineNumber;
        }
        inline uint16_t TimeStamp() const
        {
            return _timeStamp;
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize);

    private:
        MessageMetaData _metaData;
        string _filename;
        uint16_t _lineNumber;
        uint64_t _timeStamp;
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

        virtual const Core::MessageMetaData& MetaData() const = 0;
    };

    struct EXTERNAL IMessageEventFactory {
        virtual ~IMessageEventFactory() = default;
        virtual Core::ProxyType<IMessageEvent> Create() = 0;
    };

    struct EXTERNAL IMessageAssembler {
        virtual ~IMessageAssembler() = default;
        virtual string Prepare(const bool abbreviateMessage, const Core::MessageInformation& info, const Core::IMessageEvent* message) const = 0;
    };

    struct EXTERNAL IJsonSetting {
        virtual ~IJsonSetting() = default;
    };

    class TraceSetting : public Core::JSON::Container {
    public:
        TraceSetting(const string& module, const string& category, const bool enabled);
        TraceSetting();
        ~TraceSetting() = default;
        TraceSetting(const TraceSetting& other);
        TraceSetting& operator=(const TraceSetting& other);

    public:
        Core::JSON::String Module;
        Core::JSON::String Category;
        Core::JSON::Boolean Enabled;
    };

    class Settings : public Core::JSON::Container {
    public:
        Settings();
        ~Settings() = default;
        Settings(const Settings& other);
        Settings& operator=(const Settings& other);

    public:
        Core::JSON::ArrayType<TraceSetting> Tracing;
        Core::JSON::String Logging;
        Core::JSON::String WarningReporting;
    };

    /**
     * @brief Class responsible for storing information about all messages, so announced IControl will know if it should be enabled
     *        Initial list is retreived from thunder config, and is modified/extended when there is change requested in enabled categories.
     *        Info will be passed to another starting unit.
     * 
     */
    class MessageList {
    public:
        MessageList() = default;
        ~MessageList() = default;
        MessageList(const MessageList&) = delete;
        MessageList& operator=(const MessageList&) = delete;

        void Update(const MessageMetaData& metaData, const bool isEnabled);
        Settings JsonSettings() const;
        void JsonSettings(const Settings& settings);
        bool IsEnabled(const MessageMetaData& metaData) const;

    private:
        Settings _settings;
    };

    class EXTERNAL ControlList {
    public:
        using Element = std::pair<MessageMetaData, bool>;
        using Storage = std::list<Element>;
        using Iterator = Core::IteratorType<Storage, Element>;

        ControlList() = default;
        ~ControlList() = default;
        ControlList(const ControlList&) = delete;
        ControlList& operator=(const ControlList&) = delete;

        uint16_t Serialize(uint8_t buffer[], const uint16_t length, const std::list<IControl*>& controls) const;
        uint16_t Deserialize(uint8_t buffer[], const uint16_t length);

        inline Iterator Controls()
        {
            return Iterator(_info);
        }

    private:
        Storage _info;
    };

    /**
     * @brief Class responsible for:
     *        - opening buffers
     *        - reading configuration and setting message configuration accordingly
     *        - a center, where messages (and its information) from specific componenets can be pushed 
     *        - receiving information that specific message should be enabled or disabled
     * 
     */
    class EXTERNAL MessageUnit {

        using Controls = std::list<IControl*>;
        using Factories = std::unordered_map<MessageMetaData::MessageType, Core::IMessageEventFactory*>;

    public:
        static constexpr uint32_t MetaDataSize = 1 * 1024;
        static constexpr uint32_t DataSize = 9 * 1024;
        static constexpr const char* MESSAGE_DISPATCHER_PATH_ENV = _T("MESSAGE_DISPATCHER_PATH");
        static constexpr const char* MESSAGE_DISPACTHER_IDENTIFIER_ENV = _T("MESSAGE_DISPACTHER_IDENTIFIER");
        using MessageDispatcher = Core::MessageDispatcherType<MetaDataSize, DataSize>;

    public:
        static MessageUnit& Instance();
        uint32_t Open(const uint32_t instanceId);
        uint32_t Open(const string& pathName);
        void Close();

        void Defaults(const string& setting);
        void Defaults(Core::File& file);

        string Defaults() const;
        bool IsControlEnabled(const IControl* control);

        void Push(const MessageInformation& info, const IMessageEvent* message);

        void Announce(IControl* control);
        void Revoke(IControl* control);

    private:
        friend class Core::SingletonType<MessageUnit>;
        MessageUnit() = default;
        ~MessageUnit();
        MessageUnit(const MessageUnit&) = delete;
        MessageUnit& operator=(const MessageUnit&) = delete;

        void ReceiveMetaData(const uint16_t size, const uint8_t* data, uint16_t& outSize, uint8_t* outData);
        void SetDefaultSettings(const Settings& serialized);
        void UpdateControls(const MessageMetaData& metaData, const bool enabled);

    private:
        mutable Core::CriticalSection _adminLock;
        std::unique_ptr<MessageDispatcher> _dispatcher;
        uint8_t _serializationBuffer[DataSize];

        Controls _controls;
        MessageList _messages;
        ControlList _controlList;
    };
}
}