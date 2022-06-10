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

#include "MessageUnit.h"

namespace WPEFramework {
namespace Core {
    namespace Messaging {
        using namespace std::placeholders;

        MetaData::MetaData()
            : _type(INVALID)
        {
        }
        /**
        * @brief Construct a new MetaData object
        *
        * NOTE: Category and module can be set as empty
        * @param type type of the message
        * @param category category name of the message
        * @param module module name of the message
        */
        MetaData::MetaData(const MessageType type, const string& category, const string& module)
            : _type(type)
            , _category(category)
            , _module(module)
        {
        }
        uint16_t MetaData::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = static_cast<uint16_t>(sizeof(_type) + _category.size() + 1 + _module.size() + 1);

            if (bufferSize >= length) {
                ASSERT(bufferSize >= length);

                Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
                Core::FrameType<0>::Writer frameWriter(frame, 0);
                frameWriter.Number(_type);
                frameWriter.NullTerminatedText(_category);
                frameWriter.NullTerminatedText(_module);
            } else {
                length = 0;
            }

            return length;
        }
        uint16_t MetaData::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t deserialized = 0;
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Reader frameReader(frame, 0);

            _type = frameReader.Number<MetaData::MessageType>();
            deserialized += sizeof(_type);
            if(_type < MessageType::INVALID){
                _category = frameReader.NullTerminatedText();
                deserialized += static_cast<uint16_t>(_category.size() + 1);

                _module = frameReader.NullTerminatedText();
                deserialized += static_cast<uint16_t>(_module.size() + 1);
            }

            return deserialized;
        }

        Information::Information(const MetaData::MessageType type, const string& category, const string& module, const string& filename, uint16_t lineNumber, const uint64_t timeStamp)
            : _metaData(type, category, module)
            , _filename(filename)
            , _lineNumber(lineNumber)
            , _timeStamp(timeStamp)
        {
        }

        uint16_t Information::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            auto length = _metaData.Serialize(buffer, bufferSize);

            if (length != 0) {
                if (bufferSize >= length + _filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp)) {

                    Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
                    Core::FrameType<0>::Writer frameWriter(frame, 0);
                    frameWriter.NullTerminatedText(_filename);
                    frameWriter.Number(_lineNumber);
                    frameWriter.Number(_timeStamp);
                    length += static_cast<uint16_t>(_filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp));

                } else {
                    length = 0;
                }
            }

            return length;
        }
        uint16_t Information::Deserialize(uint8_t buffer[], const uint16_t bufferSize)
        {
            auto length = _metaData.Deserialize(buffer, bufferSize);

            if (length <= bufferSize && length > sizeof(MetaData::MessageType)) {
                Core::FrameType<0> frame(buffer + length, bufferSize - length, bufferSize - length);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _filename = frameReader.NullTerminatedText();
                _lineNumber = frameReader.Number<uint16_t>();
                _timeStamp = frameReader.Number<uint64_t>();

                length += static_cast<uint16_t>(_filename.size() + 1 + sizeof(_lineNumber) + sizeof(_timeStamp));
            }

            return length;
        }

        //----------Entry----------
        Settings::Messages::Entry::Entry(const string& module, const string& category, const bool enabled)
            : Core::JSON::Container()
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);

            //if done in initializer list, set values are seen as "Defaults", not as "Values"
            Module = module;
            Category = category;
            Enabled = enabled;
        }
        Settings::Messages::Entry::Entry()
            : Core::JSON::Container()
            , Module()
            , Category()
            , Enabled(false)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }

        Settings::Messages::Entry::Entry(const Entry& other)
            : Core::JSON::Container()
            , Module(other.Module)
            , Category(other.Category)
            , Enabled(other.Enabled)
        {
            Add(_T("module"), &Module);
            Add(_T("category"), &Category);
            Add(_T("enabled"), &Enabled);
        }

        Settings::Messages::Entry& Settings::Messages::Entry::operator=(const Entry& other)
        {
            if (&other == this) {
                return *this;
            }

            Module = other.Module;
            Category = other.Category;
            Enabled = other.Enabled;

            return *this;
        }

        //----------Messages----------
        Settings::Messages::Messages()
            : Core::JSON::Container()
            , Entries()
        {
            Add(_T("messages"), &Entries);
        }
        Settings::Messages::Messages(const Messages& other)
            : Core::JSON::Container()
            , Entries(other.Entries)
        {
            Add(_T("messages"), &Entries);
        }
        Settings::Messages& Settings::Messages::operator=(const Messages& other)
        {
            if (&other == this) {
                return *this;
            }

            Entries = other.Entries;
            return *this;
        }

        //----------LoggingSettings----------
        Settings::LoggingSetting::LoggingSetting()
            : Messages()
            , Abbreviated(true)
        {
            Add(_T("abbreviated"), &Abbreviated);
        }
        Settings::LoggingSetting::LoggingSetting(const LoggingSetting& other)
            : Messages()
            , Abbreviated(other.Abbreviated)
        {
            Add(_T("abbreviated"), &Abbreviated);
        }
        Settings::LoggingSetting& Settings::LoggingSetting::operator=(const LoggingSetting& other)
        {
            if (&other == this) {
                return *this;
            }
            Messages::operator=(other);
            Abbreviated = other.Abbreviated;

            return *this;
        }

        //----------Settings----------
        Settings::Settings()
            : Core::JSON::Container()
            , Tracing()
            , Logging()
            , WarningReporting(false)
        {
            Add(_T("tracing"), &Tracing);
            Add(_T("logging"), &Logging);
            Add(_T("warning_reporting"), &WarningReporting);
        }
        Settings::Settings(const Settings& other)
            : Core::JSON::Container()
            , Tracing(other.Tracing)
            , Logging(other.Logging)
            , WarningReporting(other.WarningReporting)
        {
            Add(_T("tracing"), &Tracing);
            Add(_T("logging"), &Logging);
            Add(_T("warning_reporting"), &WarningReporting);
        }

        Settings& Settings::operator=(const Settings& other)
        {
            if (this == &other) {
                return *this;
            }

            Tracing = other.Tracing;
            Logging = other.Logging;
            WarningReporting = other.WarningReporting;
            return *this;
        }

        //----------MessageList----------
        /**
        * @brief Based on metadata, update specific message. If there is no match, add entry to the list
        *
        * @param metaData information about the message
        * @param isEnabled should the message be enabled
        */
        void MessageList::Update(const MetaData& metaData, const bool isEnabled)
        {
            if (metaData.Type() == MetaData::MessageType::TRACING) {
                bool found = false;
                auto it = _settings.Tracing.Entries.Elements();
                while (it.Next()) {

                    //toggle for module and category
                    if (!metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Module() == it.Current().Module.Value() && metaData.Category() == it.Current().Category.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                        //toggle all categories for module
                    } else if (!metaData.Module().empty() && metaData.Category().empty()) {
                        if (metaData.Module() == it.Current().Module.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                    }
                    //toggle category for all modules
                    else if (metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Category() == it.Current().Category.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                    }
                }
                if (!found) {
                    _settings.Tracing.Entries.Add({ metaData.Module(), metaData.Category(), isEnabled });
                }
            }

            else if (metaData.Type() == MetaData::MessageType::LOGGING) {
                bool found = false;
                auto it = _settings.Tracing.Entries.Elements();
                while (it.Next()) {
                    if (!metaData.Category().empty()) {
                        if (metaData.Category() == it.Current().Category.Value()) {
                            it.Current().Enabled = isEnabled;
                            found = true;
                        }
                    }
                }
                if (!found) {
                    _settings.Logging.Entries.Add({ _T("SysLog"), metaData.Category(), isEnabled });
                }
            }
        }
        const Settings& MessageList::JsonSettings() const
        {
            return _settings;
        }
        void MessageList::JsonSettings(const Settings& settings)
        {
            _settings = settings;
        }
        /**
        * @brief Check if speific message (control) should be enabled
        *
        * @param metaData information about the message
        * @return true should be enabled
        * @return false should not be enabled
        */
        bool MessageList::IsEnabled(const MetaData& metaData) const
        {
            bool result = false;
            if (metaData.Type() == MetaData::MessageType::TRACING) {
                auto it = _settings.Tracing.Entries.Elements();

                while (it.Next()) {
                    if ((!it.Current().Module.IsSet() && it.Current().Category.Value() == metaData.Category()) || (it.Current().Module.Value() == metaData.Module() && it.Current().Category.Value() == metaData.Category())) {
                        result = it.Current().Enabled.Value();
                    }
                }
            } else if (metaData.Type() == MetaData::MessageType::LOGGING) {
                result = true;
                auto it = _settings.Logging.Entries.Elements();
                while (it.Next()) {
                    if (it.Current().Category.Value() == metaData.Category()) {
                        result = it.Current().Enabled.Value();
                    }
                }
            }

            return result;
        }

        //----------ControlList----------

        /**
        * @brief Write information about the announced controls to the buffer
        *
        * @param buffer buffer to be written to
        * @param length max length of the buffer
        * @return uint16_t how much bytes serialized
        */
        uint16_t ControlList::Serialize(uint8_t buffer[], const uint16_t length) const
        {
            ASSERT(length > 0);

            uint16_t serialized = 0;
            uint16_t lastSerialized = 0;
            buffer[serialized++] = static_cast<uint8_t>(0); //buffer[0] will indicate how much entries was serialized

            for (const auto& control : _controls) {
                lastSerialized = control->MessageMetaData().Serialize(buffer + serialized, length - serialized);

                if (serialized + lastSerialized < length && lastSerialized != 0) {
                    serialized += lastSerialized;
                    buffer[serialized++] = control->Enable();
                    buffer[0]++;
                } else {
                    TRACE_L1(_T("ControlList is cut, not enough memory to fit all controls. (MetaDataSize too small)"));
                    //unlucky, not all entries will fit
                    break;
                }
            }

            return serialized;
        }

        /**
        * @brief Restore information about announced controls from the buffer
        *
        * @param buffer serialized buffer
        * @param length max length of the buffer
        * @return uint16_t how much bytes deserialized
        */
        uint16_t ControlList::Deserialize(uint8_t buffer[], const uint16_t length)
        {
            ASSERT(length > 0);

            uint16_t deserialized = 0;
            uint16_t lastDeserialized = 0;
            uint8_t entries = buffer[deserialized++];

            for (int i = 0; i < entries; i++) {
                MetaData metadata;
                bool isEnabled = false;
                lastDeserialized = metadata.Deserialize(buffer + deserialized, length - deserialized);

                if (deserialized < length && lastDeserialized != 0) {
                    deserialized += lastDeserialized;
                    isEnabled = buffer[deserialized++];
                } else {
                    break;
                }

                _info.push_back({ metadata, isEnabled });
            }

            return deserialized;
        }

        void ControlList::Announce(IControl* control)
        {
            ASSERT(control != nullptr);
            ASSERT(std::find(_controls.begin(), _controls.end(), control) == _controls.end());

            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _controls.emplace_back(control);
        }

        void ControlList::Revoke(IControl* control)
        {
            ASSERT(control != nullptr);
            ASSERT(std::find(_controls.begin(), _controls.end(), control) != _controls.end());

            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);

            auto entry = std::find(_controls.begin(), _controls.end(), control);
            _controls.erase(entry);
        }

        /**
         * @brief Update controls based on metadata
         *
         */
        void ControlList::Update(const MetaData& metaData, const bool enabled)
        {
            for (auto& control : _controls) {

                if (metaData.Type() == control->MessageMetaData().Type()) {

                    //toggle for module and category
                    if (!metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Module() == control->MessageMetaData().Module() && metaData.Category() == control->MessageMetaData().Category()) {
                            control->Enable(enabled);
                        }
                        //toggle all categories for module
                    } else if (!metaData.Module().empty() && metaData.Category().empty()) {
                        if (metaData.Module() == control->MessageMetaData().Module()) {
                            control->Enable(enabled);
                        }
                    }
                    //toggle category for all modules
                    else if (metaData.Module().empty() && !metaData.Category().empty()) {
                        if (metaData.Category() == control->MessageMetaData().Category()) {
                            control->Enable(enabled);
                        }
                        //toggle all categories for all modules
                    } else {
                        control->Enable(enabled);
                    }
                }
            }
        }
        /**
         * @brief Update controls based on list of messages (eg. coming from config)
         *
         * @param messages message list
         */
        void ControlList::Update(const MessageList& messages)
        {
            for (auto& control : _controls) {
                auto enabled = messages.IsEnabled(control->MessageMetaData());
                control->Enable(enabled);
            }
        }

        void ControlList::Destroy()
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);

            while (_controls.size() != 0) {
                (*_controls.begin())->Destroy();
            }
        }
        //----------LoggingOutput----------
        LoggingOutput::LoggingAssembler::LoggingAssembler(uint64_t baseTime)
            : _baseTime(baseTime)
        {
        }
        string LoggingOutput::LoggingAssembler::Prepare(const bool abbreviate, const Information& info, const IEvent* message) const
        {
            string result;
            string messageString;
            message->ToString(messageString);

            if (abbreviate) {
                result = Core::Format("[%11ju us] %s", static_cast<uintmax_t>(info.TimeStamp() - _baseTime), messageString.c_str());
            } else {
                Core::Time now(info.TimeStamp());
                string time(now.ToRFC1123(true));

                result = Core::Format("[%s]:[%s:%d]: %s: %s", time.c_str(),
                    Core::FileNameOnly(info.FileName().c_str()),
                    info.LineNumber(),
                    info.MessageMetaData().Category().c_str(),
                    messageString.c_str());
            }

            return result;
        }

        LoggingOutput::LoggingOutput()
            : _assembler(Core::Time::Now().Ticks())
        {
        }

        void LoggingOutput::Output(const Information& info, const IEvent* message) const
        {
#ifndef __WINDOWS__
            if (_isSyslog) {
                //use longer messages for syslog
                auto result = _assembler.Prepare(false, info, message);
                syslog(LOG_NOTICE, "%s\n", result.c_str());
            } else
#endif
            {
                auto result = _assembler.Prepare(_abbreviate, info, message);
                std::cout << result << std::endl;
            }
        }

        //----------MessageUNIT----------
        MessageUnit& MessageUnit::Instance()
        {
            return (Core::SingletonType<MessageUnit>::Instance());
        }

        MessageUnit::~MessageUnit()
        {
            Close();
        }
        /**
        * @brief Open MessageUnit. This method is used on the WPEFramework side.
        *        This method:
        *        - sets env variables, so the OOP components will get information (eg. where to place its files)
        *        - create buffer where all InProcess components will write
        *
        * @param pathName volatile path (/tmp/ by default)
        * @param socketPort triggers the use of using a IP socket in stead of a domain socket (in pathName) if the port value is not 0.
        * @return uint32_t ERROR_NONE: opened sucessfully
        *                  ERROR_OPENING_FAILED failed to open
        */
        uint32_t MessageUnit::Open(const string& pathName, const uint16_t socketPort)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string basePath = Directory::Normalize(pathName) + _T("MessageDispatcher");
            string identifier = _T("md");

            if (Core::File(basePath).IsDirectory()) {
                //if directory exists remove it to clear data (eg. sockets) that can remain after previous run
                Core::Directory(basePath.c_str()).Destroy();
            }
            else if (!Core::Directory(basePath.c_str()).CreatePath()) {
                TRACE_L1(_T("Unable to create MessageDispatcher directory"));
            }

            Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            Core::SystemInfo::SetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);
            if (socketPort != 0) {
                Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_SOCKETPORT_ENV, Core::NumberType<uint16_t>(socketPort).Text());
            }

            _dispatcher.reset(new MessageDispatcher(identifier, 0, true, basePath, socketPort));
            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid()) {
                    _dispatcher->RegisterDataAvailable(std::bind(&MessageUnit::ReceiveMetaData, this, _1, _2, _3, _4));
                    result = Core::ERROR_NONE;
                }
            }
            return result;
        }

        /**
        * @brief Open MessageUnit. Method used in OOP components
        *
        * @param instanceId number of the instance
        * @return uint32_t ERROR_NONE: Opened sucesfully
        *                  ERROR_OPENING_FAILED: failed to open
        *
        */
        uint32_t MessageUnit::Open(const uint32_t instanceId)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string basePath;
            string identifier;
            string isBackground;
            string socketPortText;
            uint16_t socketPort = 0;

            Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            if (Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_SOCKETPORT_ENV, socketPortText) == true) {
                socketPort = Core::NumberType<uint16_t>(socketPortText.c_str(), static_cast<uint32_t>(socketPortText.length()), NumberBase::BASE_DECIMAL).Value();
            }
            Core::SystemInfo::GetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);
            Core::SystemInfo::GetEnvironment(MESSAGE_UNIT_LOGGING_SYSLOG_ENV, isBackground);

            _dispatcher.reset(new MessageDispatcher(identifier, instanceId, true, basePath, socketPort));
            std::istringstream(isBackground) >> _isBackground;
            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid()) {
                    result = Core::ERROR_NONE;
                }
            }

            return result;
        }

        void MessageUnit::Close()
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
            _controlList.Destroy();
            _dispatcher.reset(nullptr);
        }

        void MessageUnit::IsBackground(bool background)
        {
            _loggingOutput.IsBackground(background);
            Core::SystemInfo::SetEnvironment(MESSAGE_UNIT_LOGGING_SYSLOG_ENV, std::to_string(background));
        }

        /**
        * @brief Read defaults settings form string
        * @param setting json able to be parsed by @ref MessageUnit::Settings
        */
        void MessageUnit::Defaults(const string& setting)
        {
            _adminLock.Lock();

            Settings serialized;

            Core::OptionalType<Core::JSON::Error> error;
            serialized.IElement::FromString(setting, error);
            if (error.IsSet() == true) {

                TRACE_L1(_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str());
            }

            SetDefaultSettings(serialized);
            _loggingOutput.IsAbbreviated(serialized.Logging.Abbreviated.Value());

            _adminLock.Unlock();
        }

        /**
        * @brief Read default settings from file
        *
        * @param file file containing configuraton
        */
        void MessageUnit::Defaults(Core::File& file)
        {
            _adminLock.Lock();

            Settings serialized;

            Core::OptionalType<Core::JSON::Error> error;
            serialized.IElement::FromFile(file, error);
            if (error.IsSet() == true) {

                TRACE_L1(_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str());
            }

            SetDefaultSettings(serialized);

            _adminLock.Unlock();
        }

        /**
        * @brief Set defaults acording to settings
        *
        * @param serialized settings
        */
        void MessageUnit::SetDefaultSettings(const Settings& serialized)
        {
            _messages.JsonSettings(serialized);

            //according to received config,
            //let all announced controls know, whether they should push messages
            _controlList.Update(_messages);
        }

        /**
        * @brief Get defaults settings
        *
        * @return string json containing information about default values
        */
        string MessageUnit::Defaults() const
        {
            _adminLock.Lock();

            string result;
            auto& settings = _messages.JsonSettings();
            settings.ToString(result);

            _adminLock.Unlock();
            return result;
        }

        /**
         * @brief Check if message of given metaData is enabled by default configuration
         *
         * @param metaData message information
         * @return true is enabled by default
         * @return false not enabled by default
         */
        bool MessageUnit::IsEnabledByDefault(const MetaData& metaData) const
        {
            return _messages.IsEnabled(metaData);
        }

        /**
        * @brief Push a message and its information to a buffer
        *
        * @param info contains information about the event (where it happened)
        * @param message message
        */
        void MessageUnit::Push(const Information& info, const IEvent* message)
        {
            //logging messages can happen in Core, meaning, otherside plugin can be not started yet
            //those should be just printed
            if (info.MessageMetaData().Type() == MetaData::MessageType::LOGGING) {
                _loggingOutput.Output(info, message);
            }

            if (_dispatcher != nullptr) {

                uint16_t length = 0;

                length = info.Serialize(_serializationBuffer, sizeof(_serializationBuffer));

                //only serialize message if the information could fit
                if (length != 0) {
                    length += message->Serialize(_serializationBuffer + length, sizeof(_serializationBuffer) - length);

                    if (_dispatcher->PushData(length, _serializationBuffer) != Core::ERROR_NONE) {
                        TRACE_L1(_T("Unable to push message data!"));
                    }

                } else {
                    TRACE_L1(_T("Unable to push data, buffer is too small!"));
                }
            }
        }

        /**
        * @brief When IControl spawns it should announce itself to the unit, so it can be influenced from here
        *        (For example for enabling the category it controls)
        *
        * @param control IControl implementation
        */
        void MessageUnit::Announce(IControl* control)
        {
            _controlList.Announce(control);
            //if control was announced after we received defaults config (eg. plugin re-initialized)
            //we already have information about it - let know the control if it should push messages or not
            control->Enable(_messages.IsEnabled(control->MessageMetaData()));
        }

        /**
        * @brief When IControl dies it should be unregistered
        *
        * @param control IControl implementation
        */
        void MessageUnit::Revoke(IControl* control)
        {
            _controlList.Revoke(control);
        }

        /**
        * @brief Notification, that there is metadata available
        *
        * @param size size of the buffer
        * @param data buffer containing in data
        * @param outSize size of the out buffer (initially set to the maximum one can write)
        * @param outData out buffer (passed to the other side)
        */
        void MessageUnit::ReceiveMetaData(const uint16_t size, const uint8_t* data, uint16_t& outSize, uint8_t* outData)
        {
            if (size != 0) {
                MetaData metaData;
                //last byte is enabled flag
                auto length = metaData.Deserialize(const_cast<uint8_t*>(data), size - 1); //for now, FrameType is not handling const buffers :/

                if (length <= size - 1) {
                    bool enabled = data[length];
                    _controlList.Update(metaData, enabled);
                    _messages.Update(metaData, enabled);
                }
            } else {
                auto length = _controlList.Serialize(outData, outSize);
                outSize = length;
            }
        }

    }
}
}
