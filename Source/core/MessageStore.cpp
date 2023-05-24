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

#include "MessageStore.h"

namespace WPEFramework {

    namespace {
        /**
        * @brief Class responsible for storing information about announced controls and updating them based on incoming
        *        metadata or  SettingsList from config. This class can be serialized, and then recreated on the other
        *        side to get information about all announced controls on this side.
        */
        class Controls {
        private:
            using ControlList = std::vector<Core::Messaging::IControl*>;
        public:
            Controls(const Controls&) = delete;
            Controls& operator=(const Controls&) = delete;

            Controls() = default;
            ~Controls() = default;

        public:
            void Announce(Core::Messaging::IControl* control)
            {
                ASSERT(control != nullptr);

                _adminLock.Lock();

                ASSERT(std::find(_controlList.begin(), _controlList.end(), control) == _controlList.end());
                _controlList.push_back(control);

                _adminLock.Unlock();
            }

            void Revoke(Core::Messaging::IControl* control)
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

            void Iterate(Core::Messaging::IControl::IHandler& handler)
            {
                _adminLock.Lock();

                for (auto& control : _controlList) {
                    handler.Handle(control);
                }

                _adminLock.Unlock();
            }

        private:
            mutable Core::CriticalSection _adminLock;
            ControlList _controlList;
        };

        static Controls _registeredControls;
        static Core::Messaging::IStore* _storage;
    }

namespace Core {

    namespace Messaging {

        const char* MODULE_LOGGING = _T("SysLog");
        const char* MODULE_REPORTING = _T("Reporting");

        uint16_t Metadata::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = static_cast<uint16_t>(sizeof(_type) + (_category.size() + 1));
            
            if (_type == TRACING) {
                length += static_cast<uint16_t>(_module.size() + 1);
            }

            ASSERT(bufferSize >= length);

            if (bufferSize >= length) {
                Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
                Core::FrameType<0>::Writer frameWriter(frame, 0);
                frameWriter.Number(_type);
                frameWriter.NullTerminatedText(_category);

                if (_type == TRACING) {
                    frameWriter.NullTerminatedText(_module);
                }
            }
            else {
                length = 0;
            }

            return (length);
        }

        uint16_t Metadata::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t length = 0;

            ASSERT(bufferSize > (sizeof(_type) + (sizeof(_category[0]) * 2)));

            if (bufferSize > (sizeof(_type) + (sizeof(_category[0]) * 2))) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer), bufferSize, bufferSize);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _type = frameReader.Number<type>();
                _category = frameReader.NullTerminatedText();

                if (_type == TRACING) {
                    _module = frameReader.NullTerminatedText();
                }
                else if (_type == LOGGING) {
                    _module = MODULE_LOGGING;
                }
                else if (_type == REPORTING) {
                    _module = MODULE_REPORTING;
                }
                else {
                    ASSERT(_type != Metadata::type::INVALID);
                }
                
                if (_type == TRACING) {
                    length = std::min<uint16_t>(bufferSize, static_cast<uint16_t>(sizeof(_type) + (static_cast<uint16_t>(_category.size()) + 1) + (static_cast<uint16_t>(_module.size()) + 1)));
                }
                else {
                    length = std::min<uint16_t>(bufferSize, static_cast<uint16_t>(sizeof(_type) + (static_cast<uint16_t>(_category.size()) + 1)));
                }
            }

            return (length);
        }

        uint16_t MessageInfo::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = Metadata::Serialize(buffer, bufferSize);

            if (length != 0) {
                const uint16_t extra = static_cast<uint16_t>(sizeof(_timeStamp));

                ASSERT(bufferSize >= (length + extra));

                if (bufferSize >= (length + extra)) {
                    Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                    Core::FrameType<0>::Writer frameWriter(frame, 0);
                    frameWriter.Number(_timeStamp);
                    length += extra;
                }
                else {
                    length = 0;
                }
            }

            return (length);
        }

        uint16_t MessageInfo::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t length = Metadata::Deserialize(buffer, bufferSize);

            ASSERT(length <= bufferSize);

            if ((length <= bufferSize) && (length != 0)) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _timeStamp = frameReader.Number<uint64_t>();
                length += static_cast<uint16_t>(sizeof(_timeStamp));
            }
            else {
                length = 0;
            }

            return (length);
        }

        uint16_t IStore::Tracing::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t length = MessageInfo::Serialize(buffer, bufferSize);

            if (length != 0) {
                const uint16_t extra = static_cast<uint16_t>((_className.size() + 1) + (_fileName.size() + 1) + sizeof(_lineNumber));

                ASSERT(bufferSize >= (length + extra));

                if (bufferSize >= (length + extra)) {
                    Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                    Core::FrameType<0>::Writer frameWriter(frame, 0);
                    frameWriter.NullTerminatedText(_className);
                    frameWriter.NullTerminatedText(_fileName);
                    frameWriter.Number(_lineNumber);
                    length += extra;
                }
                else {
                    length = 0;
                }
            }

            return (length);
        }

        uint16_t IStore::Tracing::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t length = MessageInfo::Deserialize(buffer, bufferSize);

            ASSERT(length <= bufferSize);

            if ((length <= bufferSize) && (length != 0)) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer) + length, bufferSize - length, bufferSize - length);
                Core::FrameType<0>::Reader frameReader(frame, 0);
                _className = frameReader.NullTerminatedText();
                _fileName = frameReader.NullTerminatedText();
                _lineNumber = frameReader.Number<uint16_t>();
                length += static_cast<uint16_t>((_className.size() + 1) + (_fileName.size() + 1) + sizeof(_lineNumber));
            }
            else {
                length = 0;
            }

            return (length);
        }

        /* static */ void IControl::Announce(IControl* control)
        {
            _registeredControls.Announce(control);

            if (_storage != nullptr) {
                control->Enable(_storage->Default(control->Metadata()));
            }
        }

        /* static */ void IControl::Revoke(IControl* control) {
            _registeredControls.Revoke(control);
        }

        /* static */ void IControl::Iterate(IControl::IHandler& handler) {
            _registeredControls.Iterate(handler);
        }

        /* static */ IStore* IStore::Instance() {
            return (_storage);
        }
        
        /* static */ void IStore::Set(IStore* storage)
        {
            ASSERT ((_storage == nullptr) ^ (storage == nullptr));
            _storage = storage;
        }
    } // namespace Messaging
} // namespace Core
}
