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
#include "ConsoleStreamRedirect.h"

namespace Thunder {

    namespace Messaging {

        uint16_t MessageUnit::Serialize(uint8_t* buffer, const uint16_t length, const string& module)
        {
            class Handler : public Core::Messaging::IControl::IHandler {
            public:
                Handler() = delete;
                Handler(Handler&&) = delete;
                Handler(const Handler&) = delete;
                Handler& operator=(Handler&&) = delete;
                Handler& operator=(const Handler&) = delete;

                Handler(uint8_t* buffer, const uint16_t length, const string& module)
                    : _buffer(buffer)
                    , _length(length)
                    , _module(module)
                    , _offset(0)
                {
                }
                ~Handler() override = default;

            public:
                void Handle(Core::Messaging::IControl* control) override
                {
                    const Core::Messaging::Metadata& metadata = control->Metadata();

                    if (_module == metadata.Module()) {
                        Control info(metadata, control->Enable());

                        uint16_t moved = info.Serialize(&(_buffer[_offset]), _length - _offset);

                        if (moved == 0) {
                            TRACE_L1("Controls are cut, not enough memory to fit all controls (MetadataBufferSize too small)");
                        }
                        else {
                            _offset += moved;
                        }
                    }
                }

                uint16_t Offset() const {
                    return (_offset);
                }

            private:
                uint8_t* _buffer;
                const uint16_t _length;
                const string& _module;
                uint16_t _offset;
            } handler(buffer, length, module);

            Core::Messaging::IControl::Iterate(handler);

            return (handler.Offset());
        }
        
        uint16_t MessageUnit::Serialize(uint8_t* buffer, const uint16_t length)
        {
            std::vector<string> modules;

            class Handler : public Core::Messaging::IControl::IHandler {
            public:
                Handler() = delete;
                Handler(const Handler&) = delete;
                Handler& operator= (const Handler&) = delete;

                Handler(std::vector<string>& modules)
                    : _modules(modules)
                {
                }
                ~Handler() override = default;

            public:
                void Handle(Core::Messaging::IControl* control) override
                {
                    const string& module = control->Metadata().Module();
                    if (std::find(_modules.begin(), _modules.end(), module) == _modules.end()) {
                        _modules.push_back(module);
                    }
                }

            private:
                std::vector<string>& _modules;
            } handler(modules);

            Core::Messaging::IControl::Iterate(handler);

            Core::FrameType<0> frame(buffer, length, length);
            Core::FrameType<0>::Writer writer(frame, 0);

            ASSERT(modules.size() < 256);
            writer.Number<uint8_t>(static_cast<uint8_t>(modules.size()));

            std::vector<string>::const_iterator it;
            for (it = modules.begin(); it != modules.end(); ++it){
                writer.NullTerminatedText(*it);
            }

            return (writer.Offset());
        }

        void MessageUnit::Update(const Core::Messaging::Metadata& control, const bool enable)
        {
            class Handler : public Core::Messaging::IControl::IHandler {
            public:
                Handler() = delete;
                Handler(Handler&&) = delete;
                Handler(const Handler&) = delete;
                Handler& operator=(Handler&&) = delete;
                Handler& operator=(const Handler&) = delete;

                Handler(const Core::Messaging::Metadata& info, const bool enable)
                    : _info(info)
                    , _enable(enable)
                {
                }
                ~Handler() override = default;

            public:
                void Handle(Core::Messaging::IControl* control) override
                {
                    if ( (_info.Applicable(control->Metadata()) == true) && (control->Enable() ^ _enable) ) {
                        control->Enable(_enable);
                    }
                }

            private:
                const Core::Messaging::Metadata& _info;
                const bool _enable;
            } handler(control, enable);

            Core::Messaging::IControl::Iterate(handler);
        }

        void MessageUnit::Update()
        {
            class Handler : public Core::Messaging::IControl::IHandler {
            public:
                Handler() = delete;
                Handler(Handler&&) = delete;
                Handler(const Handler&) = delete;
                Handler& operator=(Handler&&) = delete;
                Handler& operator=(const Handler&) = delete;

                Handler(const Settings& settings) : _settings(settings) {}
                ~Handler() override = default;

            public:
                void Handle(Core::Messaging::IControl* control) override
                {
                    bool enabled = _settings.IsEnabled(control->Metadata());

                    if (enabled ^ control->Enable()) {
                        control->Enable(enabled);
                    }
                }

            private:
                const Settings& _settings;
            } handler(_settings);

            Core::Messaging::IControl::Iterate(handler);
        }

        MessageUnit& MessageUnit::Instance() {
            return (Core::SingletonType<MessageUnit>::Instance());
        }

        /**
        * @brief Open MessageUnit. This method is used on the Thunder side.
        *        This method:
        *        - sets env variables, so the OOP components will get information (eg. where to place its files)
        *        - create buffer where all InProcess components will write
        *
        * @param pathName volatile path (/tmp/ by default)
        */
        uint32_t MessageUnit::Open(const string& pathName, const Settings::Config& configuration, const bool background, const flush flushMode)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            string identifier = _T("md");
            _settings.Configure(pathName, identifier, configuration, background, flushMode);

            ASSERT(_dispatcher == nullptr);

            if (Core::File(_settings.BasePath()).IsDirectory() == true) {
                //if directory exists remove it to clear data (eg. sockets) that can remain after previous run
                Core::Directory(_settings.BasePath().c_str()).Destroy();
            } else {
                if (Core::Directory(_settings.BasePath().c_str()).CreatePath() == false) {
                    TRACE_L1("Unable to create MessageDispatcher directory");
                } else {
                    if (_settings.Permission()) {
                        Core::Directory(_settings.BasePath().c_str()).Permission(_settings.Permission());
                    }
                }
            }

            // Store it on an environment variable so other instances can pick this info up..
            _settings.Save();

            _dispatcher.reset(new MessageDispatcher(*this, identifier, 0, _settings.BasePath().c_str(), _settings.DataSize(), _settings.SocketPort()));
            ASSERT(_dispatcher != nullptr);

            if ((_dispatcher != nullptr) && (_dispatcher->IsValid() == true)) {

                _direct.Mode(_settings.IsBackground(), _settings.IsAbbreviated());

                Core::Messaging::IStore::Set(this);

                // according to received config,
                // let all announced controls know, whether they should push messages
                Update();

                // Redirect the standard out and standard error if requested
                if (_settings.HasRedirectedError() == true) {
                    Messaging::ConsoleStandardError::Instance().Open();
                }
                if (_settings.HasRedirectedOut() == true) {
                    // Line-buffering on text streams can still lead to messages not being displayed even if they end with a new line (only \n)
                    // So we disable buffering for stdout (line-buffered by default), as we do it in ProcessBuffer() before outputting the message anyway
                    ::setvbuf(stdout, NULL, _IONBF, 0);
                    Messaging::ConsoleStandardOut::Instance().Open();
                }
 
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

            if (instanceId != static_cast<uint32_t>(~0)) {
                _settings.Load();

                _dispatcher.reset(new MessageDispatcher(*this, _settings.Identifier(), instanceId, _settings.BasePath(), _settings.DataSize(), _settings.SocketPort()));
                ASSERT(_dispatcher != nullptr);

                if ((_dispatcher != nullptr) && (_dispatcher->IsValid() == true)) {

                    _direct.Mode(_settings.IsBackground(), _settings.IsAbbreviated());

                    Core::Messaging::IStore::Set(this);

                    // according to received config,
                    // let all announced controls know, whether they should push messages
                    Update();

                    result = Core::ERROR_NONE;
                }
            }

            return (result);
        }

        void MessageUnit::Close()
        {
            class Handler : public Core::Messaging::IControl::IHandler {
            public:
                Handler() = default;
                Handler(Handler&&) = delete;
                Handler(const Handler&) = delete;
                Handler& operator=(Handler&&) = delete;
                Handler& operator=(const Handler&) = delete;
                ~Handler() override = default;

            public:
                void Handle (Core::Messaging::IControl* control) override
                {
                    control->Destroy();
                }
            } handler;

            if (_dispatcher != nullptr) {
                if (_settings.HasRedirectedError() == true) {
                    Messaging::ConsoleStandardError::Instance().Close();
                }
                if (_settings.HasRedirectedOut() == true) {
                    Messaging::ConsoleStandardOut::Instance().Close();
                }
                Core::Messaging::IStore::Set(nullptr);
                Core::Messaging::IControl::Iterate(handler);

                _adminLock.Lock();
                _dispatcher.reset(nullptr);
                _adminLock.Unlock();
            }
        }

        /* virtual */ bool MessageUnit::Default(const Core::Messaging::Metadata& control) const {
            return (_settings.IsEnabled(control));
        }

        /**
        * @brief Push a message of any type and its information to a buffer
        */
        /* virtual */ void MessageUnit::Push(const Core::Messaging::MessageInfo& messageInfo, const Core::Messaging::IEvent* message)
        {
            //logging messages can happen in Core, meaning, otherside plugin can be not started yet
            //those should be just printed
            if (_settings.IsDirect() == true) {
                _direct.Output(messageInfo, message);
            } else if (_dispatcher != nullptr) {
                uint8_t serializationBuffer[TempDataBufferSize];
                uint16_t length = 0;

                ASSERT(messageInfo.Type() != Core::Messaging::Metadata::type::INVALID);

                length = messageInfo.Serialize(serializationBuffer, sizeof(serializationBuffer));

                //only serialize message if the information could fit
                if (length != 0) {
                    length += message->Serialize(serializationBuffer + length, sizeof(serializationBuffer) - length);

                    if (_dispatcher->PushData(length, serializationBuffer) != Core::ERROR_NONE) {
                        TRACE_L1("Unable to push message data!");
                    }
                }
                else {
                    TRACE_L1("Unable to push data, buffer is too small!");
                }
            }
        }
    } // namespace Messaging
}
