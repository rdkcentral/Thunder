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

#include <list>
#include <string>
#include <utility>

#if !defined(__CORE_WARNING_REPORTING__) && !defined(__CORE_EXCEPTION_CATCHING__)

#define WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(CALLSIGN)

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)

#else

#define WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(CALLSIGN) \
    WPEFramework::Core::CallsignTLS::CallSignTLSGuard callsignguard(CALLSIGN); 

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)    \
    WPEFramework::Core::CallsignTLS::Callsign(CALLSIGN);

namespace WPEFramework {

namespace Core {

    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType;

    class EXTERNAL CallsignTLS {
    public:

        template <const char** MODULENAME>
        struct CallsignAccess {
            static const char* Callsign() {
                return CallsignTLS::CallsignForModule(*MODULENAME);
            }
        };

        class EXTERNAL CallSignTLSGuard {
        public:
            CallSignTLSGuard(const CallSignTLSGuard&) = delete;
            CallSignTLSGuard& operator=(const CallSignTLSGuard&) = delete;

            explicit CallSignTLSGuard(const char* callsign) {
                CallsignTLS::Callsign(callsign);
            }

            ~CallSignTLSGuard() {
                CallsignTLS::Callsign(nullptr);
            }

        };

        static const char* Callsign();
        static void Callsign(const char* callsign);
        static const char* CallsignForModule(const char moduleName[]);

    private:
        friend class Core::ThreadLocalStorageType<CallsignTLS>;
    
        CallsignTLS(const CallsignTLS&) = delete;
        CallsignTLS& operator=(const CallsignTLS&) = delete;

        CallsignTLS() : _name(), _fallbacks() {};
        ~CallsignTLS() = default;

        void Name(const char* name) {  
            if ( name != nullptr ) {
                _name = name; 
            } else {
                _name.clear(); 
            }
        }
        const char* Name() const { 
            return ( _name.empty() == false ? _name.c_str() : nullptr ); 
        }

    private:
        const char* Fallback(const char moduleName[])
        {
            const char* result = nullptr;
            for (const auto& entry : _fallbacks) {
                if (entry.first == moduleName) {
                    result = entry.second.c_str();
                    break;
                }
            }
            if (result == nullptr) {
                _fallbacks.emplace_back(moduleName, std::string("???  (Module:") + moduleName + ')');
                result = _fallbacks.back().second.c_str();
            }
            return result;
        }

        std::string _name;
        std::list<std::pair<std::string, std::string>> _fallbacks;
    };
}

} 

#endif
