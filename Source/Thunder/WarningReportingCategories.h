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

#include "Module.h"

namespace Thunder {
namespace WarningReporting {

    class EXTERNAL TooLongPluginState {
    public:
        TooLongPluginState(const TooLongPluginState& a_Copy) = delete;
        TooLongPluginState& operator=(const TooLongPluginState& a_RHS) = delete;

        enum class StateChange : uint8_t { ACTIVATION,
            DEACTIVATION,
            SUSPEND,
            RESUME }; // HPL todo: create warnings upon changing state to SUSPEND/RESUME

        TooLongPluginState()
            : _stateChange(StateChange::ACTIVATION)
        {
        }

        ~TooLongPluginState() = default;

        bool Analyze(const char[], const char[], const StateChange change, const char* callsignforstatechange)
        {
            _callsign = callsignforstatechange;
            _stateChange = change;
            return true;
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
        {
            uint16_t serialized = 0;
            //include null terminator
            if (_callsign.size() + 1 + sizeof(_stateChange) <= length) {

                std::copy(_callsign.begin(), _callsign.end(), buffer);
                serialized = static_cast<uint16_t>(_callsign.size());
                buffer[serialized++] = 0; //terminating first string and then incrementing serialized count

                uint8_t stateChange = static_cast<uint8_t>(_stateChange);
                memcpy(buffer + serialized, &stateChange, sizeof(stateChange));
                serialized += sizeof(stateChange);
            }

            return serialized;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            string callsign = reinterpret_cast<const char*>(buffer);

            uint8_t stateChange = 0;
            memcpy(&stateChange, buffer + callsign.size() + 1, sizeof(stateChange));

            //include null terminator
            uint16_t deserialized = static_cast<uint16_t>(callsign.size() + 1 + sizeof(stateChange));
            if (deserialized <= length) {
                _callsign = callsign;
                _stateChange = static_cast<StateChange>(stateChange);
                return deserialized;
            }

            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = Core::Format(_T("It took suspiciously long to switch plugin [%s] to state [%s]"),
                _callsign.c_str(),
                StateAsString(_stateChange).c_str());
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 5000 };
        static constexpr uint32_t DefaultReportBound = { 5000 };

    private:
        static string StateAsString(const StateChange state)
        {
            switch (state) {
            case StateChange::ACTIVATION:
                return _T("Activation");
            case StateChange::DEACTIVATION:
                return _T("Deactivation");
            case StateChange::SUSPEND:
                return _T("Suspend");
            case StateChange::RESUME:
                return _T("Resume");
            default:
                return _T("Unknown");
            }
        }

    private:
        StateChange _stateChange;
        string _callsign;
    };

    class EXTERNAL TooLongInvokeMessage {
    public:
        TooLongInvokeMessage(const TooLongInvokeMessage& a_Copy) = delete;
        TooLongInvokeMessage& operator=(const TooLongInvokeMessage& a_RHS) = delete;

        enum class Type : uint8_t { STRING,
            JSONRPC,
            JSON,
            WEBREQUEST };

        TooLongInvokeMessage()
            : _type(Type::STRING)
            , _content()
        {
        }

        ~TooLongInvokeMessage() = default;

        // HPL todo: we might want to change the data being captured, or the logging

        bool Analyze(const char[], const char[], const string& message)
        {
            _type = Type::STRING;
            _content = message.substr(0, 20);

            return true;
        }

        bool Analyze(const char[], const char[], const Core::JSONRPC::Message& message)
        {
            _type = Type::JSONRPC;
            _content = message.Designator.Value();
            return true;
        }

        bool Analyze(const char[], const char[], const Web::Request& message)
        {
            _type = Type::WEBREQUEST;
            message.ToString(_content);
            _content = _content.substr(0, 40);
            return true;
        }

        bool Analyze(const char[], const char[], const Core::JSON::IElement& message)
        {
            _type = Type::JSON;
            message.ToString(_content);
            _content = _content.substr(0, 40);
            return true;
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
        {
            uint16_t serialized = 0;
            //include null terminator
            if (_content.size() + 1 + sizeof(_type) <= length) {

                std::copy(_content.begin(), _content.end(), buffer);
                serialized = static_cast<uint16_t>(_content.size());
                buffer[serialized++] = 0; //terminating first string and then incrementing serialized count

                uint8_t type = static_cast<uint8_t>(_type);
                memcpy(buffer + serialized, &type, sizeof(type));
                serialized += sizeof(type);
            }

            return serialized;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            string content = reinterpret_cast<const char*>(buffer);

            uint8_t type = 0;
            memcpy(&type, buffer + content.size() + 1, sizeof(type));

            //include null terminator
            uint16_t deserialzed = static_cast<uint16_t>(content.size() + 1 + sizeof(type));
            if (deserialzed <= length) {
                _content = content;
                _type = static_cast<Type>(type);
                return deserialzed;
            }

            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = Core::Format(_T("It took suspiciously long to handle an incoming message of type [%s] with content [%s]"),
                TypeAsString(_type).c_str(),
                (_content.c_str() == nullptr ? _T("<empty>") : _content.c_str()));
            visitor += Core::Format(_T(", value %" PRId64 " [ms], max allowed %" PRId64 " [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 500 };
        static constexpr uint32_t DefaultReportBound = { 500 };

    private:
        static string TypeAsString(const Type type)
        {
            switch (type) {
            case Type::STRING:
                return _T("String");
            case Type::JSONRPC:
                return _T("JSONRPC");
            case Type::JSON:
                return _T("JSON");
            case Type::WEBREQUEST:
                return _T("WebRequest");
            default:
                return _T("Unknown");
            }
        }

    private:
        Type _type;
        string _content;
    };
}
}
