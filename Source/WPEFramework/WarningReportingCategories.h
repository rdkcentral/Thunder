/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

        enum class StateChange { ACTIVATION, DEACTIVATION }; // HPL todo: also take the suspend resume state into acount?

        TooLongPluginState()
            : _stateChange(StateChange::ACTIVATION) {
        }

        ~TooLongPluginState() = default;

        bool Analyze(const char[], const char[], const StateChange change, const char* callsignforstatechange) {
            _callsign = callsignforstatechange;
            _stateChange = change;
            return true;
        }

        uint16_t Serialize(uint8_t[], const uint16_t) const {
            // HPL Todo: implement
            return(0); 
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t) {
            // HPL Todo: implement
            return(0); 
        }

        void ToString(string& visitor) const {
            visitor = Core::Format(_T("it took suspiciously long to switch plugin [%s] to state %s"), 
                _callsign.c_str(),
                (_stateChange == StateChange::ACTIVATION ? _T("Activated") : _T("Deactivated") ));
        };

        static void Configure(const string& configuration) {
            // HPL Todo: temporary for testing
            printf("TooLongWaitingForLock received specific settings: %s\n", configuration.c_str());
        }

        static constexpr uint32_t DefaultWarningBound = {5000}; 
        static constexpr uint32_t DefaultReportBound = {5000};

    private:
        StateChange _stateChange;
        string _callsign;
    };

    class EXTERNAL TooLongInvokeMessage { 
    public:
        TooLongInvokeMessage(const TooLongInvokeMessage& a_Copy) = delete;
        TooLongInvokeMessage& operator=(const TooLongInvokeMessage& a_RHS) = delete;

        enum class Type { STRING, JSONRPC, JSON, WEBREQUEST };


        TooLongInvokeMessage()
            : _type(Type::STRING)
            , _content()
        {
        }
        
        ~TooLongInvokeMessage() = default;

        // HPL todo: we might want to change the data being captured, or the logging

        bool Analyze(const char[], const char[], const string& message) {
            _type = Type::STRING;
            _content = message.substr(0, 20); 
            
            return true;
        }

        bool Analyze(const char[], const char[], const Core::JSONRPC::Message& message) {
            _type = Type::JSONRPC;
            _content = message.Designator.Value(); 
            return true;
        }

        bool Analyze(const char[], const char[], const Web::Request& message) {
            _type = Type::WEBREQUEST;
            message.ToString(_content);
            _content = _content.substr(0, 40);
            return true;
        }

        bool Analyze(const char[], const char[], const Core::JSON::IElement& message) {
            _type = Type::JSON;
            message.ToString(_content);
            _content = _content.substr(0, 40); 
            return true;
        }

        uint16_t Serialize(uint8_t[], const uint16_t) const {
            // HPL Todo: implement
            return(0); 
        }

        uint16_t Deserialize(const uint8_t[], const uint16_t) {
            // HPL Todo: implement
            return(0); 
        }

        void ToString(string& visitor) const {
            visitor = Core::Format(_T("it took suspiciously long to handle an incoming type %s message with content [%s]"),  
            TypeAsString(_type), 
            ( _content.c_str() == nullptr ? _T("<empty>") : _content.c_str() ));
        };

        static constexpr uint32_t DefaultWarningBound = {500}; 
        static constexpr uint32_t DefaultReportBound = {500};

    private:
        static const char* TypeAsString(const Type type) {
            const char* value = _T("Unknown");
            switch(type) {
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

    private:
        Type _type;
        string _content;
    };

}
} 


