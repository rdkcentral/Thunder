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

namespace WPEFramework {
namespace WarningReporting {

    class EXTERNAL TooLongPluginState {
    public:
        TooLongPluginState(const TooLongPluginState& a_Copy) = delete;
        TooLongPluginState& operator=(const TooLongPluginState& a_RHS) = delete;

        enum class StateChange { ACTIVATION, DEACTIVATION, SUSPEND, RESUME }; // HPL todo: also take the suspend resume state into acount?

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
            string stateString = StateAsString(_stateChange);
            uint16_t serialized = 0;

            //including null-terminators
            if (length >= _callsign.size() + stateString.size() + 2) {
                std::copy(_callsign.begin(), _callsign.end(), buffer);
                serialized = _callsign.size();
                buffer[serialized] = 0; //terminating first string
                ++serialized;

                std::copy(stateString.begin(), stateString.end(), buffer + serialized);
                serialized += stateString.size();
                buffer[serialized] = 0; //terminating second string
                ++serialized;

                return serialized;
            }

            return 0;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {
            string callsign = reinterpret_cast<const char*>(buffer);
            string stateString = reinterpret_cast<const char*>(buffer + callsign.size() + 1);

            if ((callsign.size() + stateString.size() + 2) <= length) {
                _callsign = callsign;
                _stateChange = StateFromString(stateString);
                return callsign.size() + stateString.size() + 2; //including null terminators
            }

            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = Core::Format(_T("It took suspiciously long to switch plugin [%s] to state %s"),
                _callsign.c_str(),
                StateAsString(_stateChange).c_str());
            visitor += Core::Format(_T(", value %lld [ms], max allowed %lld [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 5000 };
        static constexpr uint32_t DefaultReportBound = { 5000 };

    private:
        static string StateAsString(const StateChange state)
        {
            string value = _T("Unknown");
            switch (state) {
            case StateChange::ACTIVATION:
                value = _T("Activation");
                break;
            case StateChange::DEACTIVATION:
                value = _T("Deactivation");
                break;
            case StateChange::SUSPEND:
                value = _T("Suspend");
                break;
            case StateChange::RESUME:
                value = _T("Resume");
                break;
            }
            
            return value;
        }
        
        static StateChange StateFromString(const string stateString)
        {
            StateChange state = StateChange::ACTIVATION;
            if (stateString == _T("Activation")) {
                state = StateChange::ACTIVATION;
            } else if (stateString == _T("Deactivation")) {
                state = StateChange::DEACTIVATION;
            } else if (stateString == _T("Suspend")) {
                state = StateChange::SUSPEND;
            } else if (stateString == _T("Resume")) {
                state = StateChange::RESUME;
            }

            return state;
        }

    private:
        StateChange _stateChange;
        string _callsign;
    };

    class EXTERNAL TooLongInvokeMessage {
    public:
        TooLongInvokeMessage(const TooLongInvokeMessage& a_Copy) = delete;
        TooLongInvokeMessage& operator=(const TooLongInvokeMessage& a_RHS) = delete;

        enum class Type { STRING,
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
            string typeString = TypeAsString(_type);
            uint16_t serialized = 0;

            //including null-terminators
            if (length >= _content.size() + typeString.size() + 2) {
                std::copy(_content.begin(), _content.end(), buffer);
                serialized = _content.size();
                buffer[serialized] = 0; //terminating first string
                ++serialized;

                std::copy(typeString.begin(), typeString.end(), buffer + serialized);
                serialized += typeString.size();
                buffer[serialized] = 0; //terminating second string
                ++serialized;

                return serialized;
            }

            return 0;
        }

        uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
        {

            string content = reinterpret_cast<const char*>(buffer);
            string typeString = reinterpret_cast<const char*>(buffer + content.size() + 1);

            if ((content.size() + typeString.size() + 2) <= length) {
                _content = content;
                _type = TypeFromString(typeString);
                return content.size() + typeString.size() + 2; //including null terminators
            }

            return 0;
        }

        void ToString(string& visitor, const int64_t actualValue, const int64_t maxValue) const
        {
            visitor = Core::Format(_T("It took suspiciously long to handle an incoming message of type [%s] with content [%s]"),
                TypeAsString(_type).c_str(),
                (_content.c_str() == nullptr ? _T("<empty>") : _content.c_str()));
            visitor += Core::Format(_T(", value %lld [ms], max allowed %lld [ms]"), actualValue, maxValue);
        };

        static constexpr uint32_t DefaultWarningBound = { 500 };
        static constexpr uint32_t DefaultReportBound = { 500 };

    private:
        static string TypeAsString(const Type type)
        {
            string value = _T("Unknown");
            switch (type) {
            case Type::STRING:
                value = _T("String");
                break;
            case Type::JSONRPC:
                value = _T("JSONRPC");
                break;
            case Type::JSON:
                value = _T("JSON");
                break;
            case Type::WEBREQUEST:
                value = _T("WebRequest");
                break;
            }
            return value;
        }

        static Type TypeFromString(const string typeString)
        {
            Type type = Type::STRING;
            if (typeString == _T("String")) {
                type = Type::STRING;
            } else if (typeString == _T("JSONRPC")) {
                type = Type::JSONRPC;
            } else if (typeString == _T("JSON")) {
                type = Type::JSON;
            } else if (typeString == _T("WebRequest")) {
                type = Type::WEBREQUEST;
            }
            return type;
        }

    private:
        Type _type;
        string _content;
    };
}
}
