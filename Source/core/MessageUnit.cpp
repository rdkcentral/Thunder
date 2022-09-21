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

        uint16_t MetaData::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = static_cast<uint16_t>(sizeof(_type) + (_category.size() + 1) + (_module.size() + 1));
            ASSERT(bufferSize >= length);

            if (bufferSize >= length) {
                Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
                Core::FrameType<0>::Writer frameWriter(frame, 0);
                frameWriter.Number(_type);
                frameWriter.NullTerminatedText(_category);
                frameWriter.NullTerminatedText(_module);
            }
            else {
                length = 0;
            }

            return (length);
        }

        uint16_t MetaData::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t length = 0;

            ASSERT(bufferSize > (sizeof(_type) + (sizeof(_category[0]) * 2)));

            if (bufferSize > (sizeof(_type) + (sizeof(_category[0]) * 2))) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer), bufferSize, bufferSize);
                Core::FrameType<0>::Reader frameReader(frame, 0);

                _type = frameReader.Number<MetaData::MessageType>();
                ASSERT(_type != MessageType::INVALID);

                if (_type != MessageType::INVALID) {
                    _category = frameReader.NullTerminatedText();
                    _module = frameReader.NullTerminatedText();
                    length = (sizeof(_type) + (static_cast<uint16_t>(_category.size()) + 1) + (static_cast<uint16_t>(_module.size()) + 1));
                }
            }

            return (length);
        }

        uint16_t Information::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = _metaData.Serialize(buffer, bufferSize);

            if (length != 0) {
                const uint16_t extra = (_className.size() + 1) + (_fileName.size() + 1) + sizeof(_lineNumber) + sizeof(_timeStamp);
                ASSERT(bufferSize >= (length + extra));

                if (bufferSize >= (length + extra)) {
                    Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                    Core::FrameType<0>::Writer frameWriter(frame, 0);
                    frameWriter.NullTerminatedText(_className);
                    frameWriter.NullTerminatedText(_fileName);
                    frameWriter.Number(_lineNumber);
                    frameWriter.Number(_timeStamp);
                    length += extra;
                } else {
                    length = 0;
                }
            }

            return (length);
        }

        uint16_t Information::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t length = _metaData.Deserialize(buffer, bufferSize);
            ASSERT(length <= bufferSize);

            if ((length <= bufferSize) && (length != 0)) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _className = frameReader.NullTerminatedText();
                _fileName = frameReader.NullTerminatedText();
                _lineNumber = frameReader.Number<uint16_t>();
                _timeStamp = frameReader.Number<uint64_t>();

                length += static_cast<uint16_t>((_className.size() + 1) + (_fileName.size() + 1) + sizeof(_lineNumber) + sizeof(_timeStamp));
            } else {
                length = 0;
            }

            return (length);
        }

        //----------SettingsList----------

        void SettingsList::FromConfig(const Config& config)
        {
            _adminLock.Lock();

            if (config.Tracing.IsSet() == true) {
                auto it = config.Tracing.Settings.Elements();
                while (it.Next() == true) {
                    // Ensure the list is reversed, giving the bottom-most settings priority.
                    _tracing.push_front({it.Current().Category.Value(), it.Current().Module.Value(), it.Current().Enabled.Value()});
                }
            }

            if (config.Logging.IsSet() == true) {
                auto it = config.Logging.Settings.Elements();
                while (it.Next() == true) {
                    _logging.push_front({it.Current().Category.Value(), it.Current().Module.Value(), it.Current().Enabled.Value()});
                }
            }

            _adminLock.Unlock();
        }

        void SettingsList::ToConfig(Config& config) const
        {
            config.Tracing.Settings.Clear();
            config.Logging.Settings.Clear();

            _adminLock.Lock();

            for (auto it = _tracing.crbegin(); it != _tracing.crend(); ++it) {
                config.Tracing.Settings.Add({(*it).Category, (*it).Module, (*it).Enabled});
            }

            for (auto it = _logging.crbegin(); it != _logging.crend(); ++it) {
                config.Logging.Settings.Add({(*it).Category, (*it).Module, (*it).Enabled});
            }

            _adminLock.Unlock();
        }

        /**
        * @brief Based on new metadata, update a specific setting. If there is no match, add entry to the list
        */
        void SettingsList::Update(const MetaData& metaData, const bool isEnabled)
        {
            TRACE_L1("Updating settings(s): '%s':'%s'->%u\n", metaData.Category().c_str(), metaData.Module().c_str(), isEnabled);

            _adminLock.Lock();

            if (metaData.Type() == MessageType::TRACING) {
                bool found = false;

                // This code will ensure that the updated record is moved to the front of the settings list,
                // so the most recent update is considered when checking if a new control should be enabled upon anouncement.

                // module and category set
                if ((metaData.Module().empty() == false) && (metaData.Category().empty() == false)) {
                    for (auto it = _tracing.begin(); it != _tracing.end(); ++it) {
                        if ((metaData.Module() == (*it).Module) && (metaData.Category() == (*it).Category)) {
                            (*it).Enabled = isEnabled;
                            _tracing.splice(_tracing.begin(), _tracing, it);
                            found = true;
                            break;
                        }
                    }
                }
                // all categories for module
                else if ((metaData.Module().empty() == false) && (metaData.Category().empty() == true)) {
                    for (auto it = _tracing.begin(); it != _tracing.end(); ++it) {
                        if ((metaData.Module() == (*it).Module) && ((*it).Category.empty() == true)) {
                            (*it).Enabled = isEnabled;
                            _tracing.splice(_tracing.begin(), _tracing, it);
                            found = true;
                            break;
                        }
                    }
                }
                // category for all modules
                else if ((metaData.Module().empty() == true) && (metaData.Category().empty() == false)) {
                    for (auto it = _tracing.begin(); it != _tracing.end(); ++it) {
                        if ((metaData.Category() == (*it).Category) && ((*it).Module.empty() == true)) {
                            (*it).Enabled = isEnabled;
                            _tracing.splice(_tracing.begin(), _tracing, it);
                            break;
                        }
                    }
                }
                // all categories for all modules
                else if ((metaData.Module().empty() == true) && (metaData.Category().empty() == true)) {
                    for (auto it = _tracing.begin(); it != _tracing.end(); ++it) {
                        if (((*it).Module.empty() == true) && ((*it).Category.empty() == true)) {
                            (*it).Enabled = isEnabled;
                            _tracing.splice(_tracing.begin(), _tracing, it);
                            found = true;
                            break;
                        }
                    }
                }

                if (found == false) {
                    // Have a new one. Again put it in front of the list.
                    _tracing.push_front({metaData.Category(), metaData.Module(), isEnabled});
                }
            }
            else if (metaData.Type() == MessageType::LOGGING) {
                bool found = false;

                // category
                if (metaData.Category().empty() == false) {
                    for (auto it = _logging.begin(); it != _logging.end(); ++it) {
                        if (metaData.Category() == (*it).Category) {
                            (*it).Enabled = isEnabled;
                            _logging.splice(_logging.begin(), _logging, it);
                            break;
                        }
                    }
                }
                // all categories
                else if (metaData.Category().empty() == true) {
                    for (auto it = _logging.begin(); it != _logging.end(); ++it) {
                        if ((*it).Category.empty() == true) {
                            (*it).Enabled = isEnabled;
                            _logging.splice(_logging.begin(), _logging, it);
                            found = true;
                            break;
                        }
                    }
                }

                if (found == false) {
                    _logging.push_front({metaData.Category(), _T("") /* don't care */, isEnabled});
                }
            }
            else {
                ASSERT(!"Invalid message type");
            }

            _adminLock.Unlock();
        }

        /**
        * @brief Check if speific based on current settings a control should be enabled
        */
        bool SettingsList::IsEnabled(const MetaData& metaData) const
        {
            bool result = false;

            ASSERT(metaData.Category().empty() == false);

            _adminLock.Lock();

            if (metaData.Type() == MetaData::MessageType::TRACING) {
                for (auto it = _tracing.cbegin(); it != _tracing.cend(); ++it) {
                    if ((((*it).Module == metaData.Module()) && (((*it).Category.empty() == true) || ((*it).Category == metaData.Category())))
                            || (((*it).Category == metaData.Category()) && (((*it).Module.empty() == true) || ((*it).Module == metaData.Module())))
                            || (((*it).Category.empty() == true) && ((*it).Module.empty() == true))) {
                        result = (*it).Enabled;
                        // The settings are in a particular order, so we can break the loop.
                        break;
                    }
                }
            }
            else if (metaData.Type() == MetaData::MessageType::LOGGING) {
                for (auto it = _logging.cbegin(); it != _logging.cend(); ++it) {
                    if (((*it).Category.empty() == true) || ((*it).Category == metaData.Category())) {
                        result = (*it).Enabled;
                        break;
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
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

            _adminLock.Lock();

            for (const auto& control : _controls) {
                lastSerialized = control->MessageMetaData().Serialize(buffer + serialized, length - serialized);

                if ((serialized + lastSerialized < length) && (lastSerialized != 0)) {
                    serialized += lastSerialized;
                    buffer[serialized++] = control->Enable();
                    buffer[0]++;
                } else {
                    TRACE_L1("ControlList is cut, not enough memory to fit all controls (MetaDataSize too small)");
                    //unlucky, not all entries will fit
                    break;
                }
            }

            _adminLock.Unlock();

            return (serialized);
        }

        /**
        * @brief Restore information about announced controls from the buffer
        *
        * @param buffer serialized buffer
        * @param length max length of the buffer
        * @return uint16_t how much bytes deserialized
        */
        uint16_t ControlList::Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            ASSERT(length > 0);
            ASSERT(buffer != nullptr);

            uint16_t deserialized = 0;
            uint16_t lastDeserialized = 0;
            uint8_t entries = buffer[deserialized++];

            _adminLock.Lock();

            for (int i = 0; i < entries; i++) {
                MetaData metadata;
                bool isEnabled = false;
                lastDeserialized = metadata.Deserialize(buffer + deserialized, length - deserialized);

                if ((deserialized < length) && (lastDeserialized != 0)) {
                    deserialized += lastDeserialized;
                    isEnabled = buffer[deserialized++];
                } else {
                    break;
                }

                _info.push_back({ metadata, isEnabled });
            }


            return (deserialized);
        }

        void ControlList::Announce(IControl* control)
        {
            ASSERT(control != nullptr);

            _adminLock.Lock();

            ASSERT(std::find(_controls.begin(), _controls.end(), control) == _controls.end());
            _controls.push_back(control);

            _adminLock.Unlock();
        }

        void ControlList::Revoke(IControl* control)
        {
            ASSERT(control != nullptr);

            _adminLock.Lock();

            ASSERT(std::find(_controls.begin(), _controls.end(), control) != _controls.end());

            auto entry = std::find(_controls.begin(), _controls.end(), control);
            if (entry != _controls.end()) {
                _controls.erase(entry);
            }

            _adminLock.Unlock();
        }

        /**
         * @brief Update controls based on metadata
         */
        void ControlList::Update(const MetaData& metaData, const bool enabled)
        {
            TRACE_L1("Updating control(s): '%s':'%s'->%u\n", metaData.Category().c_str(), metaData.Module().c_str(), enabled);

            _adminLock.Lock();

            // module and category
            if ((metaData.Module().empty() == false) && (metaData.Category().empty() == false)) {
                for (auto& control : _controls) {
                    if ((metaData.Type() == control->MessageMetaData().Type())
                            && (metaData.Module() == control->MessageMetaData().Module())
                            && (metaData.Category() == control->MessageMetaData().Category())) {
                        control->Enable(enabled);
                        break;
                    }
                }
            }
            // all categories for module
            else if ((metaData.Module().empty() == false) && (metaData.Category().empty() == true)) {
                for (auto& control : _controls) {
                    if ((metaData.Type() == control->MessageMetaData().Type())
                            && (metaData.Module() == control->MessageMetaData().Module())) {
                        control->Enable(enabled);
                    }
                }
            }
            // category for all modules
            else if ((metaData.Module().empty() == true) && (metaData.Category().empty() == false)) {
                for (auto& control : _controls) {
                    if ((metaData.Type() == control->MessageMetaData().Type())
                            && (metaData.Category() == control->MessageMetaData().Category())) {
                        control->Enable(enabled);
                    }
                }
            // all categories for all modules
            } else {
                for (auto& control : _controls) {
                    if (metaData.Type() == control->MessageMetaData().Type()) {
                        control->Enable(enabled);
                    }
                }
            }

            _adminLock.Unlock();
        }

        /**
         * @brief Update controls based on settings
         */
        void ControlList::Update(const SettingsList& messages)
        {
            _adminLock.Lock();

            for (auto& control : _controls) {
                auto enabled = messages.IsEnabled(control->MessageMetaData());
                control->Enable(enabled);
            }

            _adminLock.Unlock();
        }

        void ControlList::Destroy()
        {
            _adminLock.Lock();

            while (_controls.size() != 0) {
                (*_controls.begin())->Destroy();
            }

            _adminLock.Unlock();
        }

        //----------LoggingOutput----------

        string LoggingOutput::LoggingAssembler::Prepare(const bool abbreviate, const Information& info, const IEvent* message) const
        {
            string result;
            string messageString;

            ASSERT(message != nullptr);

            message->ToString(messageString);

            if (abbreviate == true) {
                result = Core::Format("[%11ju us]:[%s] %s",
                    static_cast<uintmax_t>(info.TimeStamp() - _baseTime),
                    info.MessageMetaData().Category().c_str(),
                    messageString.c_str());
            } else {
                Core::Time now(info.TimeStamp());
                string time(now.ToRFC1123(true));

                result = Core::Format("[%s]:[%s:%d]:[%s]:[%s]: %s", time.c_str(),
                    Core::FileNameOnly(info.FileName().c_str()),
                    info.LineNumber(),
                    info.ClassName().c_str(),
                    info.MessageMetaData().Category().c_str(),
                    messageString.c_str());
            }

            return (result);
        }

        void LoggingOutput::Output(const Information& info, const IEvent* message) const
        {
            ASSERT(message != nullptr);

#ifndef __WINDOWS__
            if (_isSyslog == true) {
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

        //----------MessageUnit----------

        MessageUnit& MessageUnit::Instance()
        {
            return (Core::SingletonType<MessageUnit>::Instance());
        }

        /**
        * @brief Open MessageUnit. This method is used on the WPEFramework side.
        *        This method:
        *        - sets env variables, so the OOP components will get information (eg. where to place its files)
        *        - create buffer where all InProcess components will write
        *
        * @param pathName volatile path (/tmp/ by default)
        * @param socketPort triggers the use of using a IP socket in stead of a domain socket (in pathName) if the port value is not 0.
        */
        uint32_t MessageUnit::Open(const string& pathName, const uint16_t socketPort)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string basePath = Directory::Normalize(pathName) + _T("MessageDispatcher");
            string identifier = _T("md");

            ASSERT(_dispatcher == nullptr);

            if (Core::File(basePath).IsDirectory() == true) {
                //if directory exists remove it to clear data (eg. sockets) that can remain after previous run
                Core::Directory(basePath.c_str()).Destroy();
            }

            if (Core::Directory(basePath.c_str()).CreatePath() == false) {
                TRACE_L1("Unable to create MessageDispatcher directory");
            }

            Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            Core::SystemInfo::SetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);
            if (socketPort != 0) {
                Core::SystemInfo::SetEnvironment(MESSAGE_DISPATCHER_SOCKETPORT_ENV, Core::NumberType<uint16_t>(socketPort).Text());
            }

            _dispatcher.reset(new MessageDispatcher(identifier, 0, true, basePath, socketPort));
            ASSERT(_dispatcher != nullptr);

            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid() == true) {
                    _dispatcher->RegisterDataAvailable(std::bind(&MessageUnit::ReceiveMetaData, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
                    result = Core::ERROR_NONE;
                }
            }

            return (result);
        }

        /**
        * @brief Open MessageUnit. Method used in OOP components
        * @param instanceId number of the instance
        */
        uint32_t MessageUnit::Open(const uint32_t instanceId)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;
            string basePath;
            string identifier;
            string isBackground;
            string socketPortText;
            uint16_t socketPort = 0;

            ASSERT(_dispatcher == nullptr);

            Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_PATH_ENV, basePath);
            if (Core::SystemInfo::GetEnvironment(MESSAGE_DISPATCHER_SOCKETPORT_ENV, socketPortText) == true) {
                socketPort = Core::NumberType<uint16_t>(socketPortText.c_str(), static_cast<uint32_t>(socketPortText.length()), NumberBase::BASE_DECIMAL).Value();
            }
            Core::SystemInfo::GetEnvironment(MESSAGE_DISPACTHER_IDENTIFIER_ENV, identifier);
            Core::SystemInfo::GetEnvironment(MESSAGE_UNIT_LOGGING_SYSLOG_ENV, isBackground);

            _dispatcher.reset(new MessageDispatcher(identifier, instanceId, true, basePath, socketPort));
            ASSERT(_dispatcher != nullptr);

            std::istringstream(isBackground) >> _isBackground;
            if (_dispatcher != nullptr) {
                if (_dispatcher->IsValid() == true) {
                    result = Core::ERROR_NONE;
                }
            }

            return (result);
        }

        void MessageUnit::Close()
        {
            _controlList.Destroy();

            _adminLock.Lock();
            _dispatcher.reset(nullptr);
            _adminLock.Unlock();
        }

        void MessageUnit::IsBackground(bool background)
        {
            _loggingOutput.IsBackground(background);
            Core::SystemInfo::SetEnvironment(MESSAGE_UNIT_LOGGING_SYSLOG_ENV, std::to_string(background));
        }

        /**
        * @brief Read default configuration form JSON string
        */
        void MessageUnit::Configure(const string& config)
        {
            Config deserialized;

            Core::OptionalType<Core::JSON::Error> error;
            deserialized.IElement::FromString(config, error);
            if (error.IsSet() == true) {
                TRACE_L1("Parsing failed with %s", ErrorDisplayMessage(error.Value()).c_str());
            }

            Configure(deserialized);
            _loggingOutput.IsAbbreviated(deserialized.Logging.Abbreviated.Value());
        }

        /**
        * @brief Read default configuration from .json file
        */
        void MessageUnit::Configure(Core::File& config)
        {
            Config deserialized;

            Core::OptionalType<Core::JSON::Error> error;
            deserialized.IElement::FromFile(config, error);
            if (error.IsSet() == true) {
                TRACE_L1("Parsing failed with %s", ErrorDisplayMessage(error.Value()).c_str());
            }

            Configure(deserialized);
        }

        /**
        * @brief Set defaults acording to configuration
        */
        void MessageUnit::Configure(const Config& config)
        {
            _settingsList.FromConfig(config);

            //according to received config,
            //let all announced controls know, whether they should push messages
            _controlList.Update(_settingsList);
        }

        /**
        * @brief Get current configuration as JSON string
        */
        string MessageUnit::Configuration() const
        {
            string result;
            Config config;

            _settingsList.ToConfig(config);
            config.ToString(result);

            return (result);
        }

        /**
        * @brief Push a message and its information to a buffer
        */
        void MessageUnit::Push(const Information& info, const IEvent* message)
        {
            //logging messages can happen in Core, meaning, otherside plugin can be not started yet
            //those should be just printed
            if (info.MessageMetaData().Type() == MetaData::MessageType::LOGGING) {
                _loggingOutput.Output(info, message);
            }

            if (_dispatcher != nullptr) {
                uint16_t length = info.Serialize(_serializationBuffer, sizeof(_serializationBuffer));

                //only serialize message if the information could fit
                if (length != 0) {
                    length += message->Serialize(_serializationBuffer + length, sizeof(_serializationBuffer) - length);

                    if (_dispatcher->PushData(length, _serializationBuffer) != Core::ERROR_NONE) {
                        TRACE_L1("Unable to push message data!");
                    }

                } else {
                    TRACE_L1("Unable to push data, buffer is too small!");
                }
            }
        }

        /**
        * @brief When IControl spawns it should announce itself to the unit, so it can be influenced from here
        *        (For example for enabling the category it controls)
        */
        void MessageUnit::Announce(IControl* control)
        {
            _controlList.Announce(control);

            //if control was announced after we received defaults config (eg. plugin re-initialized)
            //we already have information about it - let know the control if it should push messages or not
            control->Enable(_settingsList.IsEnabled(control->MessageMetaData()));
        }

        /**
        * @brief When IControl dies it should be unregistered
        */
        void MessageUnit::Revoke(IControl* control)
        {
            _controlList.Revoke(control);
        }

        /**
        * @brief Notification, that there is metadata available
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
                auto length = metaData.Deserialize(data, size - 1); //for now, FrameType is not handling const buffers :/

                if (length <= size - 1) {
                    const bool enabled = data[length];
                    _controlList.Update(metaData, enabled);
                    _settingsList.Update(metaData, enabled);
                }
            } else {
                outSize = _controlList.Serialize(outData, outSize);;
            }
        }

    }
}
}
