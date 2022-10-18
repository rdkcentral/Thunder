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

                _type = frameReader.Number<MessageType>();
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
                const uint16_t extra = static_cast<uint16_t>((_className.size() + 1) + (_fileName.size() + 1) + sizeof(_lineNumber) + sizeof(_timeStamp));
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
            }
            else {
                length = 0;
            }

            return (length);
        }

        string LoggingOutput::Prepare(const bool abbreviate, const Information& info, const IEvent* message) const
        {
            string result;

            ASSERT(message != nullptr);

            if (abbreviate == true) {
                result = Core::Format("[%11ju us]:[%s] %s",
                    static_cast<uintmax_t>(info.TimeStamp() - _baseTime),
                    info.MessageMetaData().Category().c_str(),
                    message->Data().c_str());
            }
            else {
                Core::Time now(info.TimeStamp());
                string time(now.ToRFC1123(true));

                result = Core::Format("[%s]:[%s:%d]:[%s]:[%s]: %s", time.c_str(),
                    Core::FileNameOnly(info.FileName().c_str()),
                    info.LineNumber(),
                    info.ClassName().c_str(),
                    info.MessageMetaData().Category().c_str(),
                    message->Data().c_str());
            }

            return (result);
        }

        void LoggingOutput::Output(const Information& info, const IEvent* message)
        {
            ASSERT(message != nullptr);

#ifndef __WINDOWS__
            if (_isSyslog == true) {
                //use longer messages for syslog
                syslog(LOG_NOTICE, "%s\n", Prepare(false, info, message).c_str());
            }
            else
#endif
            {
                std::cout << Prepare(_abbreviate, info, message) << std::endl;
            }
        }

        /**
        * @brief Write information about the announced controls to the buffer
        *
        * @param buffer buffer to be written to
        * @param length max length of the buffer
        * @return uint16_t how much bytes serialized
        */
        uint16_t MessageUnit::Controls::Serialize(uint8_t buffer[], const uint16_t length) const
        {
            ASSERT(length > 0);

            uint16_t index = 0;

            _adminLock.Lock();

            for (const auto& control : _controlList) {
                Control info(control->MessageMetaData(), control->Enable());

                uint16_t moved = info.Serialize(&(buffer[index]), length - index);

                if (moved == 0) {
                    TRACE_L1("Controls is cut, not enough memory to fit all controls (MetaDataSize too small)");
                }
                else {
                    index += moved;
                }
            }

            _adminLock.Unlock();

            return (index);
        }

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
        uint32_t MessageUnit::Open(const string& pathName, const uint16_t socketPort, const string& configuration, const bool background)
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

            _settings.Configure(basePath, identifier, socketPort, (background ? 0x02 : 0x00), configuration);
            
            // Store it on an environment variable so other instances can pick this info up..
            _settings.Save();

            _dispatcher.reset(new MessageDispatcher(*this, identifier, 0, basePath, socketPort));
            ASSERT(_dispatcher != nullptr);

            if ( (_dispatcher != nullptr) && (_dispatcher->IsValid() == true) )  {
                // according to received config,
                // let all announced controls know, whether they should push messages
                _controlList.Update(_settings);

                result = Core::ERROR_NONE;
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

            ASSERT(_dispatcher == nullptr);

            _settings.Load();

            _dispatcher.reset(new MessageDispatcher(*this, _settings.Identifier(), instanceId, _settings.BasePath(), _settings.SocketPort()));
            ASSERT(_dispatcher != nullptr);

            _isBackground = ((_settings.Mode() & 0x02) != 0);
            _loggingOutput.IsAbbreviated((_settings.Mode() & 0x04) != 0);

            if ( (_dispatcher != nullptr) && (_dispatcher->IsValid() == true) ) {

                // according to received config,
                // let all announced controls know, whether they should push messages
                _controlList.Update(_settings);

                result = Core::ERROR_NONE;
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

        /**
        * @brief Push a message and its information to a buffer
        */
        void MessageUnit::Push(const Information& info, const IEvent* message)
        {
            //logging messages can happen in Core, meaning, otherside plugin can be not started yet
            //those should be just printed
            if (info.MessageMetaData().Type() == MessageType::LOGGING) {
                _loggingOutput.Output(info, message);
            }

            if (_dispatcher != nullptr) {
                uint8_t serializationBuffer[DataSize];

                uint16_t length = info.Serialize(serializationBuffer, sizeof(serializationBuffer));

                //only serialize message if the information could fit
                if (length != 0) {
                    length += message->Serialize(serializationBuffer + length, sizeof(serializationBuffer) - length);

                    if (_dispatcher->PushData(length, serializationBuffer) != Core::ERROR_NONE) {
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
        void MessageUnit::Announce(IControl* control) {
            _controlList.Announce(control);

            // Check for the startup setting..
            bool enabled = _settings.IsEnabled(control->MessageMetaData());

            if (enabled ^ control->Enable()) {
                control->Enable(enabled);
            }
        }

        /**
        * @brief When IControl dies it should be unregistered
        */
        void MessageUnit::Revoke(IControl* control) {
            _controlList.Revoke(control);
        }
    }
}
}
