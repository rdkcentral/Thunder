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

        enum MessageType : uint8_t {
            INVALID = 0,
            TRACING = 1,
            LOGGING = 2
        };

        /**
        * @brief Data-Carrier class storing information about basic information about the Message.
        *
        */
        class EXTERNAL MetaData {
        public:
            using MessageType = Messaging::MessageType;
            MetaData()
                : _type(INVALID)
                , _category()
                , _module()
            {
            }
            MetaData(const MessageType type, const string& category, const string& module)
                : _type(type)
                , _category(category)
                , _module(module)
            {
            }
            MetaData(const MetaData&) = default;
            MetaData& operator=(const MetaData&) = default;

            bool operator==(const MetaData& other) const
            {
                return ((_type == other._type) && (_category == other._category) && (_module == other._module));
            }
            bool operator!=(const MetaData& other) const
            {
                return !operator==(other);
            }

        public:
            MessageType Type() const
            {
                return _type;
            }
            const string& Category() const
            {
                return _category;
            }
            const string& Module() const
            {
                return _module;
            }

        public:
            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize);

        private:
            MessageType _type;
            string _category;
            string _module;
        };

        /**
        * @brief Data-Carrier, extended information about the message
        */
        class EXTERNAL Information {
        public:
            Information()
                : _metaData()
                , _fileName()
                , _lineNumber(0)
                , _className()
                , _timeStamp(0)
            {
            }
            Information(const MetaData::MessageType type, const string& category, const string& module,
                    const string& fileName, const uint16_t lineNumber, const string& className, const uint64_t timeStamp)
                : _metaData(type, category, module)
                , _fileName(fileName)
                , _lineNumber(lineNumber)
                , _className(className)
                , _timeStamp(timeStamp)
            {
            }
            Information(const MetaData& metaData, const string& fileName, const uint16_t lineNumber, const string& className,
                    const uint64_t timeStamp)
                : _metaData(metaData)
                , _fileName(fileName)
                , _lineNumber(lineNumber)
                , _className(className)
                , _timeStamp(timeStamp)
            {
            }
            ~Information() = default;
            Information(const Information&) = default;
            Information& operator=(const Information&) = default;

        public:
            const MetaData& MessageMetaData() const
            {
                return (_metaData);
            }
            const string& FileName() const
            {
                return (_fileName);
            }
            uint16_t LineNumber() const
            {
                return (_lineNumber);
            }
            const string& ClassName() const
            {
                return (_className);
            }
            uint64_t TimeStamp() const
            {
                return (_timeStamp);
            }

        public:
            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize);

        private:
            MetaData _metaData;
            string _fileName;
            uint16_t _lineNumber;
            string _className;
            uint64_t _timeStamp;
        };

        struct EXTERNAL IEvent {
            virtual ~IEvent() = default;
            virtual uint16_t Serialize(uint8_t buffer[], const uint16_t length) const = 0;
            virtual uint16_t Deserialize(const uint8_t buffer[], const uint16_t length) = 0;
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
         */
        class EXTERNAL Config : public Core::JSON::Container {
        private:
            class TracingSection : public Core::JSON::Container {
            private:
                class Entry : public Core::JSON::Container {
                public:
                    Entry()
                        : Core::JSON::Container()
                    {
                        Add(_T("module"), &Module);
                        Add(_T("category"), &Category);
                        Add(_T("enabled"), &Enabled);
                    }
                    Entry(const string& module, const string& category, const bool enabled)
                        : Entry()
                    {
                        Module = module;
                        Category = category;
                        Enabled = enabled;
                    }
                    Entry(const Entry& other)
                        : Entry()
                    {
                        Module = other.Module;
                        Category = other.Category;
                        Enabled = other.Enabled;
                    }
                    Entry& operator=(const Entry& other)
                    {
                        if (&other != this) {
                            Module = other.Module;
                            Category = other.Category;
                            Enabled = other.Enabled;
                        }
                        return (*this);
                    }
                    ~Entry() = default;

                public:
                    Core::JSON::String Module;
                    Core::JSON::String Category;
                    Core::JSON::Boolean Enabled;
                };

            public:
                TracingSection()
                    : Core::JSON::Container()
                    , Settings()
                {
                    Add(_T("settings"), &Settings);
                }
                ~TracingSection() = default;
                TracingSection(const TracingSection& other) = delete;
                TracingSection& operator=(const TracingSection& other) = delete;

            public:
                Core::JSON::ArrayType<Entry> Settings;
            };

            class LoggingSection: public TracingSection {
            public:
                LoggingSection()
                    : TracingSection()
                    , Abbreviated(true)
                {
                    Add(_T("abbreviated"), &Abbreviated);
                }
                ~LoggingSection() = default;
                LoggingSection(const LoggingSection& other) = delete;
                LoggingSection& operator=(const LoggingSection& other) = delete;

            public:
                Core::JSON::Boolean Abbreviated;
            };

        public:
            Config()
                : Core::JSON::Container()
                , Tracing()
                , Logging()
            {
                Add(_T("tracing"), &Tracing);
                Add(_T("logging"), &Logging);
            }
            ~Config() = default;
            Config(const Config& other) = delete;
            Config& operator=(const Config& other) = delete;

        public:
            TracingSection Tracing;
            LoggingSection Logging;
        };

        /**
        * @brief Class responsible for storing control settings, so announced IControl will know if it should be enabled
        *        Initial list is retreived from Thunder config, and is modified/extended when there is change requested
        *        in enabled categories. Info will be passed to another starting unit.
        */
        class EXTERNAL SettingsList {
        public:
            struct Record {
                string Category;
                string Module;
                bool Enabled;
            };

        public:
            SettingsList()
                : _adminLock()
                , _tracing()
                , _logging()
            {
            }
            ~SettingsList() = default;
            SettingsList(const SettingsList&) = delete;
            SettingsList& operator=(const SettingsList&) = delete;

        public:
            void Update(const MetaData& metaData, const bool isEnabled);
            void ToConfig(Config& config) const;
            void FromConfig(const Config& config);
            bool IsEnabled(const MetaData& metaData) const;

        private:
            mutable Core::CriticalSection _adminLock;
            std::list<Record> _tracing;
            std::list<Record> _logging;
        };

        /**
         * @brief Class responsible for storing information about announced controls and updating them based on incoming
         *        metadata or  SettingsList from config. This class can be serialized, and then recreated on the other
         *        side to get information about all announced controls on this side.
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

        public:
            uint16_t Serialize(uint8_t buffer[], const uint16_t length) const;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t length);

        public:
            void Announce(IControl* control);
            void Revoke(IControl* control);
            void Update(const MetaData& metaData, const bool enabled);
            void Update(const SettingsList& messages);
            void Destroy();

            InformationIterator Information()
            {
                return InformationIterator(_info);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            InformationStorage _info;
            std::list<IControl*> _controls;
        };

        /**
         * @brief Logging can be used in Core, so messages should be printed asap. This class prepares a message and prints it
         *        to a channel.
         */
        class EXTERNAL LoggingOutput {
        private:
            class LoggingAssembler {
            public:
                LoggingAssembler(uint64_t baseTime)
                    : _baseTime(baseTime)
                {
                }
                ~LoggingAssembler() = default;
                LoggingAssembler(const LoggingAssembler&) = delete;
                LoggingAssembler& operator=(const LoggingAssembler&) = delete;

            public:
                string Prepare(const bool abbreviate, const Information& info, const IEvent* message) const;

            private:
                uint64_t _baseTime;
            };

        public:
            LoggingOutput()
                : _assembler(Core::Time::Now().Ticks())
                , _isSyslog(true)
                , _abbreviate(true)
            {
            }
            ~LoggingOutput() = default;
            LoggingOutput(const LoggingOutput&) = delete;
            LoggingOutput& operator=(const LoggingOutput&) = delete;

        public:
            void IsBackground(bool background)
            {
                _isSyslog.store(background);
            }
            void IsAbbreviated(bool abbreviate)
            {
                _abbreviate.store(abbreviate);
            }

            void Output(const Information& info, const IEvent* message) const;

        private:
            LoggingAssembler _assembler;
            std::atomic_bool _isSyslog;
            std::atomic_bool _abbreviate;
        };

        /**
        * @brief Class responsible for:
        *        - opening buffers
        *        - reading configuration and setting message configuration accordingly
        *        - a center, where messages (and its information) from specific componenets can be pushed
        *        - receiving information that specific message should be enabled or disabled
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

            void Configure(const string& setting);
            void Configure(Core::File& file);
            string Configuration() const;

            void Push(const Information& info, const IEvent* message);

            void Announce(IControl* control);
            void Revoke(IControl* control);

        private:
            friend class Core::SingletonType<MessageUnit>;

            MessageUnit()
                : _adminLock()
                , _dispatcher()
                , _settingsList()
                , _controlList()
                , _loggingOutput()
                , _isBackground(false)
            {
            }
            ~MessageUnit()
            {
                Close();
            }
            MessageUnit(const MessageUnit&) = delete;
            MessageUnit& operator=(const MessageUnit&) = delete;

        private:
            void ReceiveMetaData(const uint16_t size, const uint8_t* data, uint16_t& outSize, uint8_t* outData);
            void Configure(const Config& config);

        private:
            mutable Core::CriticalSection _adminLock;
            std::unique_ptr<MessageDispatcher> _dispatcher;
            uint8_t _serializationBuffer[DataSize];

            SettingsList _settingsList;
            ControlList _controlList;

            LoggingOutput _loggingOutput;
            bool _isBackground;
        };

    } // namespace Messaging

} // namespace Core
}
