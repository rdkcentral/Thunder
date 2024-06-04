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

#include "Module.h"
#include "MessageDispatcher.h"
#include "TraceFactory.h"
#include "DirectOutput.h"

namespace Thunder {

    namespace Messaging {

        /**
        * @brief Class responsible for:
        *        - opening buffers
        *        - reading configuration and setting message configuration accordingly
        *        - a center, where messages (and its information) from specific componenets can be pushed
        *        - receiving information that specific message should be enabled or disabled
        */
        class EXTERNAL MessageUnit : public Core::Messaging::IStore {
        public:
            static constexpr uint16_t MetadataBufferSize = 4 * 1024;
            static constexpr uint16_t TempMetadataBufferSize = 128;
            static constexpr uint16_t MaxDataBufferSize = 63 * 1024;
            static constexpr uint16_t TempDataBufferSize = 1024;

            enum metadataFrameProtocol : uint8_t {
                UPDATE      = 0,
                CONTROLS    = 1,
                MODULES     = 2
            };

            enum flush : uint8_t {
                OFF                = 0,
                FLUSH              = 1,
                FLUSH_ABBREVIATED  = 2
            };

            class EXTERNAL Buffer : public Core::IPC::BufferType<static_cast<uint16_t>(~0)> {
            public:
                Buffer()
                    : Core::IPC::BufferType<static_cast<uint16_t>(~0)>(MetadataBufferSize)
                {
                }
                ~Buffer() = default;

                Buffer(const Buffer&) = delete;
                Buffer& operator=(const Buffer&) = delete;
            };

            using MetadataFrame = Core::IPCMessageType<1, Buffer, Buffer>;

            /**
             * @brief Class responsible for maintaining the state of a specific Message module/category known to
             *        the system..
             */
            class EXTERNAL Control : public Core::Messaging::Metadata {
            public:
                Control& operator=(const Control& copy) = delete;

                Control()
                    : Core::Messaging::Metadata()
                    , _enabled(false)
                {
                }
                Control(const Metadata& info, const bool enabled)
                    : Core::Messaging::Metadata(info)
                    , _enabled(enabled)
                {
                }
                Control(Control&& rhs) noexcept
                    : Core::Messaging::Metadata(rhs)
                    , _enabled(rhs._enabled)
                {
                }
                Control(const Control& copy)
                    : Core::Messaging::Metadata(copy)
                    , _enabled(copy._enabled)
                {
                }
                ~Control() = default;

                Control& operator=(Control&& move) noexcept
                {
                    if (this != &move) {
                        Core::Messaging::Metadata::operator=(move);
                        _enabled = move._enabled;
                        move._enabled = false;
                    }
                    return (*this);
                }

            public:
                bool Enabled() const {
                    return (_enabled);
                }
                
                uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const
                {
                    uint16_t length = Metadata::Serialize(buffer, bufferSize);

                    if ((length == 0) || (length >= bufferSize)) {
                        TRACE_L1("Could not serialize control !!!");
                        length = 0;
                    }
                    else {
                        buffer[length++] = (_enabled ? 1 : 0);
                    }

                    return (length);
                }
                
                uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
                {
                    uint16_t length = Metadata::Deserialize(buffer, bufferSize);

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
                Iterator& operator=(const Iterator&) = delete;

                Iterator()
                    : _position(0)
                    , _container()
                    , _index()
                {
                }
                Iterator(const ControlList& copy)
                    : _position(0)
                    , _container(copy)
                    , _index(_container.begin())
                {
                }
                Iterator(Iterator&& move) noexcept
                    : _position(0)
                    , _container(std::move(move._container))
                    , _index(_container.begin())
                {
                }
                ~Iterator() = default;

                Iterator& operator=(Iterator&& rhs) noexcept
                {
                    _position = 0;
                    _container = std::move(rhs._container);
                    _index = _container.begin();

                    return (*this);
                }
                Iterator& operator=(ControlList&& rhs) noexcept
                {
                    _position = 0;
                    _container = std::move(rhs);
                    _index = _container.begin();

                    return (*this);
                }

            public:
                bool IsValid() const {
                    return ((_position > 0) && (_index != _container.end()));
                }

                void Reset()
                {
                    _position = 0;
                    _index = _container.begin();
                }

                bool Next()
                {
                    if (_position == 0) {
                        _position++;
                    }
                    else if (_index != _container.end()) {
                        _position++;
                        _index++;
                    }

                    return (_index != _container.end());
                }

                Core::Messaging::Metadata::type Type() const
                {
                    ASSERT(IsValid());

                    return (_index->Type());
                }

                const string& Module() const
                {
                    ASSERT(IsValid());

                    return (_index->Module());
                }

                const string& Category() const
                {
                    ASSERT(IsValid());

                    return (_index->Category());
                }

                bool Enabled() const
                {
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

                enum mode : uint8_t {
                    BACKGROUND     = 0x01,
                    DIRECT         = 0x02,
                    ABBREVIATED    = 0x04,
                    REDIRECT_OUT   = 0x08,
                    REDIRECT_ERROR = 0x10
                };

            public:
                /**
                 * @brief JSON Settings for all messages
                 */
                class EXTERNAL Config : public Core::JSON::Container {
                public:
                    class Section : public Core::JSON::Container {
                    public:
                        class Entry : public Core::JSON::Container {
                        public:
                            Entry()
                                : Core::JSON::Container()
                                , Module()
                                , Category()
                                , Enabled(false)
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
                            Entry(Entry&& other)
                                : Core::JSON::Container()
                                , Module(std::move(other.Module))
                                , Category(std::move(other.Category))
                                , Enabled(std::move(other.Enabled))
                            {
                                Add(_T("module"), &Module);
                                Add(_T("category"), &Category);
                                Add(_T("enabled"), &Enabled);
                            }
                            Entry(const Entry& other)
                                : Core::JSON::Container()
                                , Module(other.Module)
                                , Category(other.Category)
                                , Enabled(other.Enabled)
                            {
                                Add(_T("module"), &Module);
                                Add(_T("category"), &Category);
                                Add(_T("enabled"), &Enabled);
                            }

                            Entry& operator=(Entry&& other)
                            {
                                if (&other != this) {
                                    Module = std::move(other.Module);
                                    Category = std::move(other.Category);
                                    Enabled = std::move(other.Enabled);
                                }
                                
                                return (*this);
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
                        Section(Section&& other) = delete;
                        Section(const Section& other) = delete;
                        Section& operator=(Section&& other) = delete;
                        Section& operator=(const Section& other) = delete;

                        Section()
                            : Core::JSON::Container()
                            , Settings()
                            , Abbreviated(true) {
                            Add(_T("settings"), &Settings);
                            Add(_T("abbreviated"), &Abbreviated);
                        }
                        ~Section() = default;

                    public:
                        Core::JSON::ArrayType<Entry> Settings;
                        Core::JSON::Boolean Abbreviated;
                    };

                public:
                    Config()
                        : Core::JSON::Container()
                        , Tracing()
                        , Logging()
                        , Reporting()
                        , Port(0)
                        , Path(_T("MessageDispatcher"))
                        , Flush(false)
                        , Out(true)
                        , Error(true)
                        , DataSize(20 * 1024)
                    {
                        Add(_T("tracing"), &Tracing);
                        Add(_T("logging"), &Logging);
                        Add(_T("reporting"), &Reporting);
                        Add(_T("path"), &Path);
                        Add(_T("port"), &Port);
                        Add(_T("flush"), &Flush);
                        Add(_T("stdout"), &Out);
                        Add(_T("stderr"), &Error);
                        Add(_T("datasize"), &DataSize);
                    }
                    ~Config() = default;
                    Config(const Config& other) = delete;
                    Config& operator=(const Config& other) = delete;

                public:
                    Section Tracing;
                    Section Logging;
                    Section Reporting;
                    Core::JSON::DecUInt16 Port;
                    Core::JSON::String Path;
                    Core::JSON::Boolean Flush;
                    Core::JSON::Boolean Out;
                    Core::JSON::Boolean Error;
                    Core::JSON::DecUInt16 DataSize;
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
                    , _permission(0)
                    , _mode(static_cast<mode>(0))
                    , _dataSize()
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

                uint16_t DataSize() const {
                    return (_dataSize);
                }

                uint16_t Permission() const {
                    return (_permission);
                }

                bool IsBackground() const {
                    return ((_mode & mode::BACKGROUND) != 0);
                }

                bool IsDirect() const {
                    return ((_mode & mode::DIRECT) != 0);
                }

                bool HasRedirectedOut() const {
                    return ((_mode & mode::REDIRECT_OUT) != 0);
                }

                bool HasRedirectedError() const {
                    return ((_mode & mode::REDIRECT_ERROR) != 0);
                }

                Core::Messaging::MessageInfo::abbreviate IsAbbreviated() const {
                    Core::Messaging::MessageInfo::abbreviate abbreviate;

                    if ((_mode & mode::ABBREVIATED) == 0) {
                        abbreviate = Core::Messaging::MessageInfo::abbreviate::FULL;
                    }
                    else {
                        abbreviate = Core::Messaging::MessageInfo::abbreviate::ABBREVIATED;
                    }

                    return (abbreviate);
                }

                void Configure(const string& basePath, const string& identifier, const Config& jsonParsed, const bool background, const flush flushMode)
                {
                    _settings.clear();
                    string messagingFolder;
                    Core::ParsePathInfo(jsonParsed.Path.Value(), messagingFolder, _permission);

                    _path = Core::Directory::Normalize(basePath) + messagingFolder;
                    _identifier = identifier;
                    _socketPort = jsonParsed.Port.Value();
                    _mode = (background ? mode::BACKGROUND : 0) | 
                            (((flushMode != flush::OFF) || (jsonParsed.Flush.Value())) ? mode::DIRECT : 0) | 
                            (flushMode == flush::FLUSH_ABBREVIATED ? mode::ABBREVIATED : 0) |
                            (jsonParsed.Error.Value() ? mode::REDIRECT_ERROR : 0) |
                            (jsonParsed.Out.IsSet() ? (jsonParsed.Out.Value() ? mode::REDIRECT_OUT : 0) : (background ? mode::REDIRECT_OUT : 0));
                    if (jsonParsed.DataSize.Value() > MaxDataBufferSize) {
                        TRACE_L1("Data buffer size set in the config is too large! The maximum has been used instead");
                        _dataSize = MaxDataBufferSize;

                        ASSERT(false);
                    }
                    else {
                        _dataSize = jsonParsed.DataSize.Value();
                    }

                    FromConfig(jsonParsed);
                }

                /**
                 * @brief Based on new metadata, update a specific setting. If there is no match, add entry to the list
                 */
                void Update(const Core::Messaging::Metadata& metaData, const bool isEnabled)
                {
                    bool enabled = metaData.Default();

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
                bool IsEnabled(const Core::Messaging::Metadata& metaData) const
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
                void Save() const
                {
                    // Store all config info..
                    string settings = _path + DELIMITER +
                               _identifier + DELIMITER +
                               Core::NumberType<uint16_t>(_socketPort).Text() + DELIMITER +
                               Core::NumberType<uint8_t>(_mode & (mode::BACKGROUND|mode::DIRECT|mode::ABBREVIATED)).Text() + DELIMITER +
                               Core::NumberType<uint16_t>(_dataSize).Text();

                    for (auto& entry : _settings) {
                        settings += DELIMITER + Core::NumberType<uint8_t>(entry.Type()).Text() +
                                    DELIMITER + entry.Module() +
                                    DELIMITER + entry.Category() +
                                    DELIMITER + (entry.Enabled() ? '1' : '0');
                    }

                    Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_CONFIG_ENV, settings, true);
                }

                void Load()
                {
                    string settings;
                    Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_CONFIG_ENV, settings);
                    Core::TextSegmentIterator iterator(Core::TextFragment(settings, 0, static_cast<uint16_t>(settings.length())), false, DELIMITER);

                    _path.clear();
                    _identifier.clear();
                    _socketPort = 0;
                    _mode = 0;
                    _dataSize = 0;
                    _settings.clear();

                    if (iterator.Next() == true) {
                        _path = iterator.Current().Text();
                        if (iterator.Next() == true) {
                            _identifier = iterator.Current().Text();
                            if (iterator.Next() == true) {
                                _socketPort = Core::NumberType<uint16_t>(iterator.Current()).Value();
                                if (iterator.Next() == true) {
                                    _mode = Core::NumberType<uint8_t>(iterator.Current()).Value();
                                    if (iterator.Next() == true) {
                                        _dataSize = Core::NumberType<uint16_t>(iterator.Current()).Value();
                                    }
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
                                    if ((type >= Core::Messaging::Metadata::type::TRACING) && (type <= Core::Messaging::Metadata::type::REPORTING) &&
                                        (enabled.length() == 1) &&
                                        ((enabled[0] == '0') || (enabled[0] == '1'))) {
                                        _settings.emplace_back(Core::Messaging::Metadata(static_cast<Core::Messaging::Metadata::type>(type), category, module), (enabled[0] == '1'));
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
                            Core::Messaging::Metadata info(Core::Messaging::Metadata::type::TRACING, it.Current().Category.Value(), it.Current().Module.Value());
                            if (info.Default() != it.Current().Enabled.Value()) {
                                _settings.emplace_back(info, it.Current().Enabled.Value());
                            }
                        }
                    }

                    if (config.Logging.IsSet() == true) {
                        auto it = config.Logging.Settings.Elements();
                        while (it.Next() == true) {
                            Core::Messaging::Metadata info(Core::Messaging::Metadata::type::LOGGING, it.Current().Category.Value(), it.Current().Module.Value());
                            if (info.Default() != it.Current().Enabled.Value()) {
                                _settings.emplace_back(info, it.Current().Enabled.Value());
                            }
                        }
                    }

                    if (config.Reporting.IsSet() == true) {
                        auto it = config.Reporting.Settings.Elements();
                        while (it.Next() == true) {
                            Core::Messaging::Metadata info(Core::Messaging::Metadata::type::REPORTING, it.Current().Category.Value(), it.Current().Module.Value());
                            _settings.emplace_back(info, it.Current().Enabled.Value());
                        }
                    }

                    _adminLock.Unlock();
                }

                void ToConfig(Config& config) const
                {
                    config.Tracing.Settings.Clear();
                    config.Logging.Settings.Clear();
                    config.Reporting.Settings.Clear();

                    _adminLock.Lock();

                    for (auto it = _settings.crbegin(); it != _settings.crend(); ++it) {
                        if (it->Type() == Core::Messaging::Metadata::type::TRACING) {
                            config.Tracing.Settings.Add(Config::Section::Entry(it->Category(), it->Module(), it->Enabled()));
                        }
                        else if (it->Type() == Core::Messaging::Metadata::type::LOGGING) {
                            config.Logging.Settings.Add(Config::Section::Entry(it->Category(), it->Module(), it->Enabled()));
                        }
                        else if (it->Type() == Core::Messaging::Metadata::type::REPORTING) {
                            config.Reporting.Settings.Add(Config::Section::Entry(it->Category(), it->Module(), it->Enabled()));
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
                uint16_t _permission;
                uint8_t _mode;
                uint16_t _dataSize;
            };

            class EXTERNAL Client : public MessageDataBuffer {
            private:
                using BaseClass = MessageDataBuffer;

            public:
                Client() = delete;
                Client(Client&&) = delete;
                Client(const Client&) = delete;
                Client& operator=(Client&&) = delete;
                Client& operator=(const Client&) = delete;

                Client(const string& identifier, const uint32_t instanceId, const string& baseDirectory, const uint16_t socketPort = 0)
                    : MessageDataBuffer(identifier, instanceId, baseDirectory, MessageUnit::Instance().DataSize(), socketPort, false)
                    , _channel(Core::NodeId(MetadataName().c_str()), MetadataBufferSize) {
                    _channel.Open(Core::infinite);
                }
                ~Client() {
                    _channel.Close(100);
                }

            public:
                bool IsValid() const {
                    return (_channel.IsOpen());
                }

                /**
                 * @brief Exchanges metadata with the server. Reader needs to register for notifications to recevie this message.
                 *        Passed buffer will be filled with data from the other side
                 *
                 * @param length length of the message
                 * @param value buffer
                 * @param maxLength maximum size of the buffer
                 * @return uint16_t how much data was written back to the buffer
                 */
                uint32_t Update(const uint32_t waitTime, const Core::Messaging::Metadata& control, const bool enabled)
                {
                    uint32_t result = Core::ERROR_ILLEGAL_STATE;

                    if (_channel.IsOpen() == true) {

                        uint8_t dataBuffer[TempMetadataBufferSize];

                        // We got a connection to the spawned process side, get the list of traces from
                        // there and send our settings from here...
                        Core::ProxyType<MetadataFrame> metaDataFrame(Core::ProxyType<MetadataFrame>::Create());

                        Core::FrameType<0> frame(dataBuffer, TempMetadataBufferSize, TempMetadataBufferSize);
                        Core::FrameType<0>::Writer writer(frame, 0);
                        writer.Number<metadataFrameProtocol>(metadataFrameProtocol::UPDATE);

                        Control message(control, enabled);
                        uint16_t length = message.Serialize(dataBuffer + writer.Offset(), sizeof(dataBuffer) - writer.Offset());

                        metaDataFrame->Parameters().Set(writer.Offset() + length, dataBuffer);

                        result = _channel.Invoke(metaDataFrame, waitTime);
                    }

                    return (result);
                }

                void Load(ControlList& info, const string& module) const
                {
                    if (_channel.IsOpen() == true) {

                        // We got a connection to the spawned process side, get the list of traces from
                        // there and send our settings from here...
                        Core::ProxyType<MetadataFrame> metaDataFrame(Core::ProxyType<MetadataFrame>::Create());

                        uint8_t buffer[64];
                        Core::FrameType<0> frame(buffer, sizeof(buffer), sizeof(buffer));
                        Core::FrameType<0>::Writer writer(frame, 0);

                        writer.Number<metadataFrameProtocol>(metadataFrameProtocol::CONTROLS);
                        writer.NullTerminatedText(module);

                        metaDataFrame->Parameters().Set(writer.Offset(), buffer);

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

                void Modules(std::vector<string>& modules) const
                {
                    if (_channel.IsOpen() == true) {
                        Core::ProxyType<MetadataFrame> metaDataFrame(Core::ProxyType<MetadataFrame>::Create());

                        uint8_t buffer[1];
                        Core::FrameType<0> frame(buffer, sizeof(buffer), sizeof(buffer));
                        Core::FrameType<0>::Writer writer(frame, 0);

                        writer.Number<metadataFrameProtocol>(metadataFrameProtocol::MODULES);

                        metaDataFrame->Parameters().Set(writer.Offset(), buffer);

                        uint32_t result = _channel.Invoke(metaDataFrame, Core::infinite);

                        if (result == Core::ERROR_NONE) {
                            uint16_t bufferSize = metaDataFrame->Response().Length();
                            const uint8_t* buffer = metaDataFrame->Response().Value();

                            Core::FrameType<0> frame(const_cast<uint8_t*>(buffer), bufferSize, bufferSize);
                            Core::FrameType<0>::Reader reader(frame, 0);

                            uint8_t iterator = reader.Number<uint8_t>();

                            while(iterator > 0) {
                                const string& module = reader.NullTerminatedText();

                                if (std::find(modules.begin(), modules.end(), module) == modules.end()) {
                                    modules.push_back(module);
                                }
                                --iterator;
                            }
                        }
                    }
                }

            private:
                mutable Core::IPCChannelClientType<Core::Void, false, true> _channel;
            };

        private:
            using Factories = std::unordered_map<Core::Messaging::Metadata::type, IEventFactory*>;

            // This is the listening end-point, and it is created as the master in which we push messages
            class MessageDispatcher : public MessageDataBuffer {
            private:
                using BaseClass = MessageDataBuffer;
                class MetaDataBuffer : public Core::IPCChannelClientType<Core::Void, true, true> {
                private:
                    using BaseClass = Core::IPCChannelClientType<Core::Void, true, true>;

                    class MetadataFrameHandler : public Core::IIPCServer {
                    public:
                        MetadataFrameHandler() = delete;
                        MetadataFrameHandler(const MetadataFrameHandler&) = delete;
                        MetadataFrameHandler& operator=(const MetadataFrameHandler&) = delete;

                        MetadataFrameHandler(MessageUnit& parent)
                            : _parent(parent) {
                        }
                        ~MetadataFrameHandler() override = default;

                    public:
                        void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data) override
                        {
                            uint8_t outBuffer[MetadataBufferSize];

                            auto message = Core::ProxyType<MetadataFrame>(data);

                            Core::FrameType<0> frame(const_cast<uint8_t*>(message->Parameters().Value()), message->Parameters().Length(), message->Parameters().Length());
                            Core::FrameType<0>::Reader reader(frame, 0);

                            ASSERT(reader.HasData());
                            metadataFrameProtocol protocol = reader.Number<metadataFrameProtocol>();

                            if (protocol == metadataFrameProtocol::UPDATE) {
                                Control newSettings;
                                newSettings.Deserialize(reader.Data(), reader.Length());
                                _parent.Update(newSettings, newSettings.Enabled());
                                message->Response().Set(0, nullptr);
                            }
                            else if (protocol == metadataFrameProtocol::CONTROLS) {
                                ASSERT(reader.HasData());
                                string module = reader.NullTerminatedText();
                                uint16_t length = _parent.Serialize(outBuffer, sizeof(outBuffer), module);
                                message->Response().Set(length, outBuffer);
                            }
                            else if (protocol == metadataFrameProtocol::MODULES) {
                                uint16_t length = _parent.Serialize(outBuffer, sizeof(outBuffer));
                                message->Response().Set(length, outBuffer);
                            }
                            else {
                                ASSERT(false);
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
                        : BaseClass(Core::NodeId(binding.c_str()), MetadataBufferSize)
                        , _handler(parent)
                    {
                        _handler.AddRef();
                        CreateFactory<MetadataFrame>(1);
                        Register(MetadataFrame::Id(), Core::ProxyType<Core::IIPCServer>(_handler));
                        Open(Core::infinite);
                    }
                    ~MetaDataBuffer() override
                    {
                        Close(Core::infinite);
                        Unregister(MetadataFrame::Id());
                        DestroyFactory<MetadataFrame>();
                        _handler.CompositRelease();
                    }

                private:
                    Core::ProxyObject<MetadataFrameHandler> _handler;
                };

            public:
                MessageDispatcher() = delete;
                MessageDispatcher(MessageDispatcher&&) = delete;
                MessageDispatcher(const MessageDispatcher&) = delete;
                MessageDispatcher& operator=(MessageDispatcher&&) = delete;
                MessageDispatcher& operator=(const MessageDispatcher&) = delete;

                /**
                 * @brief Construct a new Message Dispatcher object
                 *
                 * @param identifier name of the instance
                 * @param instanceId number of the instance
                 * @param initialize should dispatcher be initialzied. Should be done only once, on the server side
                 * @param baseDirectory where to place all the necessary files. This directory should exist before creating this class.
                 * @param dataSize size of the data buffer in bytes
                 * @param socketPort triggers the use of using a IP socket in stead of a domain socket if the port value is not 0.
                 */
                MessageDispatcher(MessageUnit& parent, const string& identifier, const uint32_t instanceId, const string& basePath, const uint16_t dataSize, const uint16_t socketPort)
                    : BaseClass(identifier, instanceId, basePath, dataSize, socketPort, true)
                    , _metaDataBuffer(parent, BaseClass::MetadataName())
                {
                }
                ~MessageDispatcher() = default;

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
                , _settings()
                , _direct()
            {
            }

        public:
            MessageUnit(const MessageUnit&) = delete;
            MessageUnit& operator=(const MessageUnit&) = delete;

            static MessageUnit& Instance();

            ~MessageUnit() {
                ASSERT(_dispatcher == nullptr);
            }

        public:
            const string& BasePath() const {
                return (_settings.BasePath());
            }

            const string& Identifier() const {
                return (_settings.Identifier());
            }

            uint16_t SocketPort() const {
                return (_settings.SocketPort());
            }

            uint16_t DataSize() const {
                return (_settings.DataSize());
            }

            uint32_t Open(const string& pathName, const Settings::Config& configuration, const bool background, const flush flushMode);
            uint32_t Open(const uint32_t instanceId);
            void Close();

            bool Default(const Core::Messaging::Metadata& control) const override;
            void Push(const Core::Messaging::MessageInfo& messageInfo, const Core::Messaging::IEvent* message) override;

        private:
            uint16_t Serialize(uint8_t* buffer, const uint16_t length, const string& module);
            uint16_t Serialize(uint8_t* buffer, const uint16_t length);
            void Update(const Core::Messaging::Metadata& control, const bool enable);
            void Update();

        private:
            mutable Core::CriticalSection _adminLock;
            std::unique_ptr<MessageDispatcher> _dispatcher;
            Settings _settings;
            DirectOutput _direct;
        };

    } // namespace Messaging
}
