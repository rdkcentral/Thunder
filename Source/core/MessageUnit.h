/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
    namespace Messaging {

        /**
        * @brief Data-Carrier class storing information about basic information about the Message.
        * 
        */
        class EXTERNAL MetaData {
        public:
            enum MessageType : uint8_t {
                TRACING = 0,
                LOGGING = 1,
                INVALID = 3
            };

            MetaData();
            MetaData(const MessageType type, const string& category, const string& module);
            MetaData(const MetaData&) = default;
            MetaData& operator=(const MetaData&) = default;
            inline bool operator==(const MetaData& other) const
            {
                return _type == other._type && _category == other._category && _module == other._module;
            }
            inline bool operator!=(const MetaData& other) const
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
        class EXTERNAL Information {
        public:
            Information() = default;
            Information(const MetaData::MessageType type, const string& category, const string& module,
                const string& filename, uint16_t lineNumber, const uint64_t timestamp);
            Information(const Information&) = default;
            Information& operator=(const Information&) = default;

            inline const MetaData& MessageMetaData() const
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
            inline uint64_t TimeStamp() const
            {
                return _timeStamp;
            }

            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
            uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize);

        private:
            MetaData _metaData;
            string _filename;
            uint16_t _lineNumber;
            uint64_t _timeStamp;
        };

        struct EXTERNAL IEvent {
            virtual ~IEvent() = default;
            virtual uint16_t Serialize(uint8_t buffer[], const uint16_t length) const = 0;
            virtual uint16_t Deserialize(uint8_t buffer[], const uint16_t length) = 0;
            virtual void ToString(string& text) const = 0;
        };

        struct EXTERNAL IControl {
            virtual ~IControl() = default;
            virtual void Enable(bool enable) = 0;
            virtual bool Enable() const = 0;
            virtual void Destroy() = 0;

            virtual const MetaData& MessageMetaData() const = 0;
        };

        struct EXTERNAL IEventFactory {
            virtual ~IEventFactory() = default;
            virtual Core::ProxyType<IEvent> Create() = 0;
        };

        /**
         * @brief JSON Settings for all messages
         * 
         */
        class EXTERNAL Settings : public Core::JSON::Container {
        private:
            class Messages : public Core::JSON::Container {
            private:
                class Entry : public Core::JSON::Container {
                public:
                    Entry(const string& module, const string& category, const bool enabled);
                    Entry();
                    ~Entry() = default;
                    Entry(const Entry& other);
                    Entry& operator=(const Entry& other);

                public:
                    Core::JSON::String Module;
                    Core::JSON::String Category;
                    Core::JSON::Boolean Enabled;
                };

            public:
                Messages();
                ~Messages() = default;
                Messages(const Messages& other);
                Messages& operator=(const Messages& other);

            public:
                Core::JSON::ArrayType<Entry> Entries;
            };

            class LoggingSetting : public Messages {
            public:
                LoggingSetting();
                ~LoggingSetting() = default;
                LoggingSetting(const LoggingSetting& other);
                LoggingSetting& operator=(const LoggingSetting& other);

            public:
                Core::JSON::Boolean Abbreviated;
            };

        public:
            Settings();
            ~Settings() = default;
            Settings(const Settings& other);
            Settings& operator=(const Settings& other);

        public:
            Messages Tracing;
            LoggingSetting Logging;
            Core::JSON::String WarningReporting;
        };

        /**
        * @brief Class responsible for storing information about all messages, so announced IControl will know if it should be enabled
        *        Initial list is retreived from thunder config, and is modified/extended when there is change requested in enabled categories.
        *        Info will be passed to another starting unit.
        * 
        */
        class EXTERNAL MessageList {
        public:
            MessageList() = default;
            ~MessageList() = default;
            MessageList(const MessageList&) = delete;
            MessageList& operator=(const MessageList&) = delete;

            void Update(const MetaData& metaData, const bool isEnabled);
            const Settings& JsonSettings() const;
            void JsonSettings(const Settings& settings);
            bool IsEnabled(const MetaData& metaData) const;

        private:
            Settings _settings;
        };

        /**
         * @brief Class responsible for storing information about announced controls and updating them based on incoming metadata or 
         *        MessageList from config. 
         *        This class can be serialized, and then recreated on the other side to get information about all announced controls on this side.
         * 
         */
        class EXTERNAL ControlList {
        public:
            using InformationElement = std::pair<MetaData, bool>;
            using InformationStorage = std::list<InformationElement>;
            using InformationIterator = Core::IteratorType<InformationStorage, InformationElement>;

            ControlList() = default;
            ~ControlList() = default;
            ControlList(const ControlList&) = delete;
            ControlList& operator=(const ControlList&) = delete;

            uint16_t Serialize(uint8_t buffer[], const uint16_t length) const;
            uint16_t Deserialize(uint8_t buffer[], const uint16_t length);

            void Announce(IControl* control);
            void Revoke(IControl* control);
            void Update(const MetaData& metaData, const bool enabled);
            void Update(const MessageList& messages);
            void Destroy();

            inline InformationIterator Information()
            {
                return InformationIterator(_info);
            }

        private:
            Core::CriticalSection _adminLock;
            InformationStorage _info;
            std::list<IControl*> _controls;
        };

        /**
         * @brief Logging can be used in Core, so messages should be printed asap. This class prepares a message and prints it
         *        to a channel.
         * 
         */
        class EXTERNAL LoggingOutput {
        private:
            class LoggingAssembler {
            public:
                LoggingAssembler(uint64_t baseTime);
                ~LoggingAssembler() = default;
                string Prepare(const bool abbreviate, const Information& info, const IEvent* message) const;

            private:
                uint64_t _baseTime;
            };

        public:
            LoggingOutput();
            ~LoggingOutput() = default;
            LoggingOutput(const LoggingOutput&) = default;
            LoggingOutput& operator=(const LoggingOutput&) = default;
            inline void IsBackground(bool background)
            {
                _isSyslog.store(background);
            }
            inline void IsAbbreviated(bool abbreviate)
            {
                _abbreviate.store(abbreviate);
            }

            void Output(const Information& info, const IEvent* message) const;

        private:
            LoggingAssembler _assembler;
            std::atomic_bool _isSyslog{ true };
            std::atomic_bool _abbreviate{ true };
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
            using Factories = std::unordered_map<MetaData::MessageType, IEventFactory*>;

        public:
            static constexpr uint32_t MetaDataSize = 10 * 1024;
            static constexpr uint32_t DataSize = 20 * 1024;
            static constexpr const char* MESSAGE_DISPATCHER_PATH_ENV = _T("MESSAGE_DISPATCHER_PATH");
            static constexpr const char* MESSAGE_DISPATCHER_SOCKETPORT_ENV = _T("MESSAGE_SOCKETPORT");
            static constexpr const char* MESSAGE_DISPACTHER_IDENTIFIER_ENV = _T("MESSAGE_DISPACTHER_IDENTIFIER");
            static constexpr const char* MESSAGE_UNIT_LOGGING_SYSLOG_ENV = _T("MESSAGE_UNIT_LOGGING_SYSLOG");

            using MessageDispatcher = Core::MessageDispatcherType<MetaDataSize, DataSize>;

        public:
            static MessageUnit& Instance();
            uint32_t Open(const string& pathName, const uint16_t doorbell = 0);
            uint32_t Open(const uint32_t instanceId);
            void Close();
            void IsBackground(bool background);

            void Defaults(const string& setting);
            void Defaults(Core::File& file);
            string Defaults() const;
            bool IsEnabledByDefault(const MetaData& metaData) const;

            void Push(const Information& info, const IEvent* message);

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

        private:
            mutable Core::CriticalSection _adminLock;
            std::unique_ptr<MessageDispatcher> _dispatcher;
            uint8_t _serializationBuffer[DataSize];

            MessageList _messages;
            ControlList _controlList;

            LoggingOutput _loggingOutput;
            bool _isBackground;
        };
    }
}
}
