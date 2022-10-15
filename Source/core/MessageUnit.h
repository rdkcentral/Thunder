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
            INVALID   = 0,
            TRACING   = 1,
            LOGGING   = 2,
            REPORTING = 3
        };

        /**
        * @brief Data-Carrier class storing information about basic information about the Message.
        *
        */
        class EXTERNAL MetaData {
        public:
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
            MetaData(MetaData&&) = default;
            MetaData& operator=(MetaData&&) = default;

            bool operator==(const MetaData& other) const
            {
                return ((_type == other._type) && (_category == other._category) && (_module == other._module));
            }
            bool operator!=(const MetaData& other) const
            {
                return !operator==(other);
            }

        public:
            MessageType Type() const {
                return _type;
            }
            const string& Category() const {
                return _category;
            }
            const string& Module() const {
                return _module;
            }
            bool Default() const {
                return (_type == MessageType::TRACING ? false : true);
            }
            bool Specific() const {
                return ((_type == MessageType::LOGGING) || ((Category().empty() == false) && (Module().empty() == false)));
            }
            bool Applicable(const MetaData& rhs) const {
                return ((rhs.Type() == Type()) &&
                    (rhs.Module().empty() || Module().empty() || (rhs.Module() == Module())) &&
                    (rhs.Category().empty() || Category().empty() || (rhs.Category() == Category())));
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
            Information(const MessageType type, const string& category, const string& module,
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
            virtual const string& Data() const = 0;
        };

        struct EXTERNAL IEventFactory {
            virtual ~IEventFactory() = default;
            virtual Core::ProxyType<IEvent> Create() = 0;
        };

        struct EXTERNAL IControl {
            virtual ~IControl() = default;
            virtual void Enable(bool enable) = 0;
            virtual bool Enable() const = 0;
            virtual void Destroy() = 0;

            virtual const MetaData& MessageMetaData() const = 0;
        };

        struct EXTERNAL IOutput {
            virtual ~IOutput() = default;
            virtual void Output(const Information& info, const IEvent* message) = 0;
        };

        /**
         * @brief Logging can be used in Core, so messages should be printed asap. This class prepares a message and prints it
         *        to a channel.
         */
        class EXTERNAL LoggingOutput : public IOutput {
        public:
            LoggingOutput()
                : _baseTime(Core::Time::Now().Ticks())
                , _isSyslog(true)
                , _abbreviate(true)
            {
            }
            ~LoggingOutput() override = default;
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

            void Output(const Information& info, const IEvent* message) override;

        private:
            string Prepare(const bool abbreviate, const Information& info, const IEvent* message) const;

        private:
            uint64_t _baseTime;
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
        public:
            static constexpr uint32_t MetaDataSize = 10 * 1024;
            static constexpr uint32_t DataSize = 20 * 1024;

            /**
             * @brief Class responsible for maintaining the state of a specific Message module/category known to
             *        the system..
             */
            class EXTERNAL Control : public MetaData {
            public:
                Control& operator= (const Control& copy) = delete;

                Control()
                    : MetaData()
                    , _enabled(false) {
                }
                Control(const MetaData& info, const bool enabled)
                    : MetaData(info)
                    , _enabled(enabled) {
                }
                Control(Control&& rhs)
                    : MetaData(rhs)
                    , _enabled(rhs._enabled) {
                }
                Control(const Control& copy)
                    : MetaData(copy)
                    , _enabled(copy._enabled) {
                }
                ~Control() = default;

                Control& operator= (Control&& rhs) {
                    MetaData::operator=(rhs);
                    _enabled = rhs._enabled;

                    return (*this);
                }

            public:
                bool Enabled() const {
                    return (_enabled);
                }
                uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const {
                    uint16_t length = MetaData::Serialize(buffer, bufferSize);

                    if ((length == 0) || (length >= bufferSize)) {
                        TRACE_L1("Could not serialize control !!!");
                        length = 0;
                    }
                    else {
                        buffer[length++] = (_enabled ? 1 : 0);
                    }

                    return (length);
                }
                uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) {
                    uint16_t length = MetaData::Deserialize(buffer, bufferSize);

                    if ((length == 0) || (length >= bufferSize)) {
                        TRACE_L1("Could not deserialize control !!!");
                        length = 0;
                    }
                    else {
                        _enabled = (buffer[length++] == 0 ? false : true);
                    }

                    return (length);
                }

            private:
                bool _enabled;
            };
            using ControlList = std::vector<Control>;
            class EXTERNAL Iterator {
            public:
                Iterator(const Iterator&) = delete;
                Iterator& operator= (const Iterator&) = delete;

                Iterator()
                    : _position(0)
                    , _container()
                    , _index() {
                }
                Iterator(const ControlList& copy)
                    : _position(0)
                    , _container(copy)
                    , _index(_container.begin()) {
                }
                Iterator(Iterator&& move)
                    : _position(0)
                    , _container(move._container)
                    , _index(_container.begin()) {
                }
                ~Iterator() = default;

                Iterator& operator= (Iterator&& rhs) {
                    _position = 0;
                    _container = std::move(rhs._container);
                    _index = _container.begin();

                    return (*this);
                }
                Iterator& operator= (ControlList&& rhs) {
                    _position = 0;
                    _container = std::move(rhs);
                    _index = _container.begin();

                    return (*this);
                }

            public:
                bool IsValid() const {
                    return ((_position > 0) && (_index != _container.end()));
                }
                void Reset() {
                    _position = 0;
                    _index = _container.begin();
                }
                bool Next() {
                    if (_position == 0) {
                        _position++;
                    }
                    else if (_index != _container.end()) {
                        _position++;
                        _index++;
                    }

                    return (_index != _container.end());
                }
                Messaging::MessageType Type() const {
                    ASSERT(IsValid());
                    return (_index->Type());
                }
                const string& Module() const {
                    ASSERT(IsValid());
                    return (_index->Module());
                }
                const string& Category() const {
                    ASSERT(IsValid());
                    return (_index->Category());
                }
                bool Enabled() const {
                    ASSERT(IsValid());
                    return (_index->Enabled());
                }

            private:
                uint32_t _position;
                ControlList _container;
                ControlList::iterator _index;
            };
            class EXTERNAL Settings {
            private:
                static constexpr const TCHAR* MESSAGE_DISPATCHER_CONFIG_ENV = _T("MESSAGE_DISPATCHER_CONFIG");
                static constexpr const TCHAR  DELIMITER = '|';

                /**
                 * @brief JSON Settings for all messages
                 */
                class Config : public Core::JSON::Container {
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

                    class LoggingSection : public TracingSection {
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

            public:
                Settings(const Settings&) = delete;
                Settings& operator=(const Settings&) = delete;

                Settings()
                    : _adminLock()
                    , _settings()
                    , _path()
                    , _identifier()
                    , _socketPort()
                    , _mode()
                {
                }
                ~Settings() = default;

            public:
                const string& BasePath() const {
                    return(_path);
                }
                const string& Identifier() const {
                    return (_identifier);
                }
                uint16_t SocketPort() const {
                    return (_socketPort);
                }
                uint8_t Mode() const {
                    return (_mode);
                }
                void Configure (const string& path, const string& identifier, const uint16_t socketPort, const uint8_t mode, const string& config)
                {
                    _settings.clear();
                    _path = path;
                    _identifier = identifier;
                    _socketPort = socketPort;
                    _mode = mode;

                    Config jsonParsed;
                    jsonParsed.FromString(config);
                    FromConfig(jsonParsed);
                }
                /**
                 * @brief Based on new metadata, update a specific setting. If there is no match, add entry to the list
                 */
                void Update(const MetaData& metaData, const bool isEnabled)
                {
                    bool enabled = metaData.Default();
                    bool found = false;

                    TRACE_L1("Updating settings(s): '%s':'%s'->%u\n", metaData.Category().c_str(), metaData.Module().c_str(), isEnabled);

                    _adminLock.Lock();

                    ControlList::iterator index = _settings.begin();

                    // First see if we have an exact match..
                    while ((index != _settings.end()) && (*index != metaData)) {
                        if (index->Applicable(metaData) == true) {
                            enabled = index->Enabled();
                        }
                        index++;
                    }

                    if (index != _settings.end()) {
                        index = _settings.erase(index);
                        while (index != _settings.end()) {
                            if (index->Applicable(metaData) == true) {
                                enabled = index->Enabled();
                            }
                            index++;
                        }
                    }

                    if (enabled != isEnabled) {
                        _settings.emplace_back(metaData, isEnabled);
                    }

                    _adminLock.Unlock();
                }

                /**
                 * @brief Check if specific based on current settings a control should be enabled
                 */
                bool IsEnabled(const MetaData& metaData) const
                {
                    bool done = false;
                    bool result = metaData.Default();

                    _adminLock.Lock();

                    ControlList::const_iterator index = _settings.cbegin();

                    // First see if we have an exact match..
                    while ((done == false) && (index != _settings.end())) {
                        if (*index == metaData) {
                            done = true;
                            result = index->Enabled();
                        }
                        else if (index->Applicable(metaData) == true) {
                            result = index->Enabled();
                            index++;
                        }
                        else {
                            index++;
                        }
                    }

                    _adminLock.Unlock();

                    return (result);
                }
                void Save() const {
                    uint16_t index = 0;

                    // Store all config info..
                    string settings = _path + DELIMITER + 
                               _identifier + DELIMITER +
                               Core::NumberType<uint16_t>(_socketPort).Text() + DELIMITER +
                               Core::NumberType<uint8_t>(_mode).Text();

                    for (auto& entry : _settings) {
                        settings += DELIMITER + Core::NumberType<uint8_t>(entry.Type()).Text() +
                                    DELIMITER + entry.Module() +
                                    DELIMITER + entry.Category() +
                                    DELIMITER + (entry.Enabled() ? '1' : '0');
                    }

                    Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_CONFIG_ENV, settings, true);
                }
                void Load() {
                    string settings;
                    Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_CONFIG_ENV, settings);
                    Core::TextSegmentIterator iterator(Core::TextFragment(settings, 0, static_cast<uint16_t>(settings.length())), false, DELIMITER);

                    _path.clear();
                    _identifier.clear();
                    _socketPort = 0;
                    _mode = 0;
                    _settings.clear();

                    if (iterator.Next() == true) {
                        _path = iterator.Current().Text();
                        if (iterator.Next() == true) {
                            _identifier = iterator.Current().Text();
                            if (iterator.Next() == true) {
                                _socketPort = Core::NumberType<uint16_t>(iterator.Current()).Value();
                                if (iterator.Next() == true) {
                                    _mode = Core::NumberType<uint8_t>(iterator.Current()).Value();
                                }
                            }
                        }
                    }

                    while (iterator.Next()) {
                        uint8_t type = Core::NumberType<uint8_t>(iterator.Current()).Value();
                        if (iterator.Next() == true) {
                            string module = iterator.Current().Text();
                            if (iterator.Next() == true) {
                                string category = iterator.Current().Text();
                                if (iterator.Next() == true) {
                                    string enabled = iterator.Current().Text();

                                    if ((type >= Messaging::MessageType::TRACING) && (type <= Messaging::MessageType::REPORTING) &&
                                        (enabled.length() == 1) && 
                                        ((enabled[0] == '0') || (enabled[0] == '1'))) {
                                        _settings.emplace_back(MetaData(static_cast<Messaging::MessageType>(type), category, module), (enabled[0] == '1'));
                                    }
                                }
                            }
                        }
                    }
                }

            private:
                void FromConfig(const Config& config)
                {
                    _adminLock.Lock();

                    if (config.Tracing.IsSet() == true) {
                        auto it = config.Tracing.Settings.Elements();
                        while (it.Next() == true) {
                            MetaData info(MessageType::TRACING, it.Current().Category.Value(), it.Current().Module.Value());
                            if (info.Default() != it.Current().Enabled.Value()) {
                                _settings.emplace_back(info, it.Current().Enabled.Value());
                            }
                        }
                    }

                    if (config.Logging.IsSet() == true) {
                        auto it = config.Logging.Settings.Elements();
                        while (it.Next() == true) {
                            MetaData info(MessageType::LOGGING, it.Current().Category.Value(), it.Current().Module.Value());
                            if (info.Default() != it.Current().Enabled.Value()) {
                                _settings.emplace_back(info, it.Current().Enabled.Value());
                            }
                        }
                    }

                    _adminLock.Unlock();
                }
                void ToConfig(Config& config) const
                {
                    config.Tracing.Settings.Clear();
                    config.Logging.Settings.Clear();

                    _adminLock.Lock();

                    for (auto it = _settings.crbegin(); it != _settings.crend(); ++it) {
                        if (it->Type() == MessageType::TRACING) {
                            config.Tracing.Settings.Add({ it->Category(), it->Module(), it->Enabled() });
                        }
                        else if (it->Type() == MessageType::LOGGING) {
                            config.Logging.Settings.Add({ it->Category(), it->Module(), it->Enabled() });
                        }
                    }

                    _adminLock.Unlock();
                }

            private:
                mutable Core::CriticalSection _adminLock;
                ControlList _settings;
                string _path;
                string _identifier;
                uint16_t _socketPort;
                uint8_t _mode;
            };
            class EXTERNAL Client : public Core::MessageDataBufferType<DataSize, MetaDataSize> {
            public:
                Client() = delete;
                Client(const Client&) = delete;
                Client& operator= (const Client&) = delete;

                Client(const string& identifier, const uint32_t instanceId, const string& baseDirectory, const uint16_t socketPort = 0)
                    : Core::MessageDataBufferType < DataSize, MetaDataSize>(identifier, instanceId, baseDirectory, socketPort, false)
                    , _channel(Core::NodeId(MetadataName().c_str()), MetaDataSize) {

                    _channel.Open(Core::infinite);
                }
                ~Client() {
                    _channel.Close(Core::infinite);
                }

            public:
                bool IsValid() const {
                    return (_channel.IsOpen());
                }

                /**
                 * @brief Exchanges metadata with the server. Reader needs to register for notifications to recevie this message.
                 *        Passed buffer will be filled with data from thr other side
                 *
                 * @param length length of the message
                 * @param value buffer
                 * @param maxLength maximum size of the buffer
                 * @return uint16_t how much data was written back to the buffer
                 */
                uint32_t Update(const uint32_t waitTime, const MetaData& control, const bool enabled) {
                    uint32_t result = Core::ERROR_ILLEGAL_STATE;

                    if (_channel.IsOpen() == true) {

                        uint8_t dataBuffer[MetaDataSize];

                        // We got a connection to the spawned process side, get the list of traces from 
                        // there and send our settings from here...
                        Core::ProxyType<MetaDataFrame> metaDataFrame(Core::ProxyType<MetaDataFrame>::Create());
                        Control message(control, enabled);
                        uint16_t length = message.Serialize(dataBuffer, sizeof(dataBuffer));
                        metaDataFrame->Parameters().Set(length, dataBuffer);

                        result = _channel.Invoke(metaDataFrame, Core::infinite);
                    }

                    return (result);
                }
                void Load(ControlList& info) const {

                    if (_channel.IsOpen() == true) {

                        // We got a connection to the spawned process side, get the list of traces from 
                        // there and send our settings from here...
                        Core::ProxyType<MetaDataFrame> metaDataFrame(Core::ProxyType<MetaDataFrame>::Create());

                        metaDataFrame->Parameters().Set(0, nullptr);

                        uint32_t result = _channel.Invoke(metaDataFrame, Core::infinite);

                        if (result == Core::ERROR_NONE) {
                            uint16_t index = 0;
                            uint16_t bufferSize = metaDataFrame->Response().Length();
                            const uint8_t* buffer = metaDataFrame->Response().Value();

                            while (index < bufferSize) {
                                Control entry;
                                uint16_t length = entry.Deserialize(&(buffer[index]), bufferSize - index);
                                if (length == 0) {
                                    TRACE_L1("Could not deserialize all controls !!!");
                                    index = bufferSize;
                                }
                                else {
                                    index += length;
                                    if (std::find(info.begin(), info.end(), entry) == info.end()) {
                                        info.emplace_back(entry);
                                    }
                                }
                            }

                            if (index != bufferSize) {
                                TRACE_L1("Could not deserialize all controls !!!");
                            }
                        }
                    }
                }

            private:
                mutable Core::IPCChannelClientType<Core::Void, false, true> _channel;
            };

        private:
            /**
             * @brief Class responsible for storing information about announced controls and updating them based on incoming
             *        metadata or  SettingsList from config. This class can be serialized, and then recreated on the other
             *        side to get information about all announced controls on this side.
             */
            class Controls {
            private:
                using ControlList = std::vector<IControl*>;

            public:
                Controls(const Controls&) = delete;
                Controls& operator=(const Controls&) = delete;

                Controls() = default;
                ~Controls() = default;

            public:
                uint16_t Serialize(uint8_t buffer[], const uint16_t length) const;

                void Announce(IControl* control)
                {
                    ASSERT(control != nullptr);

                    _adminLock.Lock();

                    ASSERT(std::find(_controlList.begin(), _controlList.end(), control) == _controlList.end());
                    _controlList.push_back(control);

                    _adminLock.Unlock();
                }
                void Revoke(IControl* control)
                {
                    ASSERT(control != nullptr);

                    _adminLock.Lock();

                    ASSERT(std::find(_controlList.begin(), _controlList.end(), control) != _controlList.end());

                    auto entry = std::find(_controlList.begin(), _controlList.end(), control);
                    if (entry != _controlList.end()) {
                        _controlList.erase(entry);
                    }

                    _adminLock.Unlock();
                }
                void Update(const MetaData& metaData, const bool enabled)
                {
                    TRACE_L1("Updating control(s): '%s':'%s'->%u\n", metaData.Category().c_str(), metaData.Module().c_str(), enabled);

                    _adminLock.Lock();

                    for (auto& control : _controlList) {
                        if ( (metaData.Applicable(control->MessageMetaData()) == true) && (control->Enable() ^ enabled) ) {
                            control->Enable(enabled);
                        }
                    }

                    _adminLock.Unlock();
                }
                void Update(const Settings& messages)
                {
                    _adminLock.Lock();

                    for (IControl*& control : _controlList) {
                        bool enabled = messages.IsEnabled(control->MessageMetaData());
                        if (enabled ^ control->Enable()) {
                            control->Enable(enabled);
                        }
                    }

                    _adminLock.Unlock();
                }
                void Destroy()
                {
                    _adminLock.Lock();

                    while (_controlList.size() != 0) {
                        (*_controlList.begin())->Destroy();
                    }

                    _adminLock.Unlock();
                }

            private:
                mutable Core::CriticalSection _adminLock;
                ControlList _controlList;
            };

        private:
            using Factories = std::unordered_map<MessageType, IEventFactory*>;

            // This is the listening end-point, and it is created as the master in which we push messages
            class MessageDispatcher : public Core::MessageDataBufferType<DataSize, MetaDataSize> {
            private:
                using BaseClass = Core::MessageDataBufferType<DataSize, MetaDataSize>;
                using MetaDataFrame = BaseClass::MetaDataFrame;

                class MetaDataBuffer : public Core::IPCChannelClientType<Core::Void, true, true> {
                private:
                    using BaseClass = Core::IPCChannelClientType<Core::Void, true, true>;

                    class MetaDataFrameHandler : public Core::IIPCServer {
                    public:
                        MetaDataFrameHandler() = delete;
                        MetaDataFrameHandler(const MetaDataFrameHandler&) = delete;
                        MetaDataFrameHandler& operator=(const MetaDataFrameHandler&) = delete;

                        MetaDataFrameHandler(MessageUnit& parent)
                            : _parent(parent) {
                        }
                        ~MetaDataFrameHandler() override = default;

                    public:
                        void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data) override
                        {
                            uint8_t outBuffer[MetaDataSize];

                            auto message = Core::ProxyType<MetaDataFrame>(data);

                            // What is coming in, is an update?
                            if (message->Parameters().Length() > 0) {
                                Control newSettings;
                                newSettings.Deserialize(message->Parameters().Value(), message->Parameters().Length());
                                _parent.Update(newSettings, newSettings.Enabled());
                                message->Response().Set(0, nullptr);
                            }
                            else {
                                uint16_t length = _parent.Serialize(outBuffer, sizeof(outBuffer));
                                message->Response().Set(length, outBuffer);
                            }
                            source.ReportResponse(data);
                        }

                    private:
                        MessageUnit& _parent;
                    };

                public:
                    MetaDataBuffer() = delete;
                    MetaDataBuffer(const MetaDataBuffer&) = delete;
                    MetaDataBuffer& operator=(const MetaDataBuffer&) = delete;

                    MetaDataBuffer(MessageUnit& parent, const string& binding)
                        : BaseClass(Core::NodeId(binding.c_str()), MetaDataSize)
                        , _handler(parent)
                    {
                        _handler.AddRef();
                        CreateFactory<MetaDataFrame>(1);
                        Register(MetaDataFrame::Id(), Core::ProxyType<Core::IIPCServer>(_handler));
                        Open(Core::infinite);
                    }
                    ~MetaDataBuffer() override
                    {
                        Close(Core::infinite);
                        Unregister(MetaDataFrame::Id());
                        DestroyFactory<MetaDataFrame>();
                        _handler.CompositRelease();
                    }

                private:
                    Core::ProxyObject<MetaDataFrameHandler> _handler;
                };

            public:
                MessageDispatcher() = delete;
                MessageDispatcher(const MessageDispatcher&) = delete;
                MessageDispatcher& operator= (const MessageDispatcher&) = delete;

                /**
                 * @brief Construct a new Message Dispatcher object
                 *
                 * @param identifier name of the instance
                 * @param instanceId number of the instance
                 * @param initialize should dispatcher be initialzied. Should be done only once, on the server side
                 * @param baseDirectory where to place all the necessary files. This directory should exist before creating this class.
                 * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
                 */
                MessageDispatcher(MessageUnit& parent, const string& identifier, const uint32_t instanceId, const string& basePath, const uint16_t socketPort)
                    : BaseClass(identifier, instanceId, basePath, socketPort, true)
                    , _metaDataBuffer(parent, BaseClass::MetadataName()) {
                }
                virtual ~MessageDispatcher() = default;

            public:
                bool IsValid() const {
                    return ((BaseClass::IsValid()) && (_metaDataBuffer.IsOpen()));
                }

            private:
                MetaDataBuffer _metaDataBuffer;
            };

        private:
            friend class Core::SingletonType<MessageUnit>;
            MessageUnit()
                : _adminLock()
                , _dispatcher()
                , _controlList()
                , _settings()
                , _loggingOutput()
                , _isBackground(false) {
            }

        public:
            MessageUnit(const MessageUnit&) = delete;
            MessageUnit& operator=(const MessageUnit&) = delete;

            static MessageUnit& Instance();

            ~MessageUnit() {
                Close();
            }

        public:
            uint32_t Open(const string& pathName, const uint16_t doorbell, const string& configuration, const bool background);
            uint32_t Open(const uint32_t instanceId);
            void Close();

            void Push(const Information& info, const IEvent* message);

            void Announce(IControl* control);
            void Revoke(IControl* control);

        private:
            uint16_t Serialize(uint8_t* buffer, const uint16_t length) {
                return (_controlList.Serialize(buffer, length));
            }
            void Update(const MetaData& control, const bool enable) {
                _controlList.Update(control, enable);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            std::unique_ptr<MessageDispatcher> _dispatcher;
            Controls _controlList;
            Settings _settings;
            LoggingOutput _loggingOutput;
            bool _isBackground;
        };
    } // namespace Messaging
} // namespace Core
}
