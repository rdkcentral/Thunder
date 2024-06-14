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

#if !defined(__CORE_WARNING_REPORTING__) && !defined(__CORE_EXCEPTION_CATCHING__)

#define WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(CALLSIGN)

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)

#else

#define WARNING_REPORTING_THREAD_SETCALLSIGN_GUARD(CALLSIGN) \
    Thunder::Core::CallsignTLS::CallSignTLSGuard callsignguard(CALLSIGN);

#define WARNING_REPORTING_THREAD_SETCALLSIGN(CALLSIGN)    \
    Thunder::Core::CallsignTLS::Callsign(CALLSIGN);

namespace Thunder {

namespace Core {

    template <typename THREADLOCALSTORAGE>
    class ThreadLocalStorageType;

    class EXTERNAL CallsignTLS {
    public:
        CallsignTLS(CallsignTLS&&) = delete;
        CallsignTLS(const CallsignTLS&) = delete;
        CallsignTLS& operator=(CallsignTLS&&) = delete;
        CallsignTLS& operator=(const CallsignTLS&) = delete;

    private:
        CallsignTLS() : _name() {};
        ~CallsignTLS() = default;

    public:
        class EXTERNAL CallSignTLSGuard {
        public:
            CallSignTLSGuard(CallSignTLSGuard&&) = delete;
            CallSignTLSGuard(const CallSignTLSGuard&) = delete;
            CallSignTLSGuard& operator=(CallSignTLSGuard&&) = delete;
            CallSignTLSGuard& operator=(const CallSignTLSGuard&) = delete;

            explicit CallSignTLSGuard(const TCHAR* callsign) {
                CallsignTLS::Callsign(callsign);
            }

            ~CallSignTLSGuard() {
                CallsignTLS::Callsign(nullptr);
            }
        };

        template <const TCHAR** MODULENAME>
        struct CallsignAccess {
            static const TCHAR* Callsign() {
                if (_moduleName.empty()) {
                    _moduleName = (string(_T("???  (Module:")) + *MODULENAME + _T(')'));
                }
                const TCHAR* callsign = CallsignTLS::Callsign();
                if (callsign == nullptr) {
                    callsign = _moduleName.c_str();
                }
                return callsign;
            }

        private:
            static string _moduleName;
        };

        static const TCHAR* Callsign();
        static void Callsign(const TCHAR* callsign);

    private:
        friend class Core::ThreadLocalStorageType<CallsignTLS>;
    
        void Name(const TCHAR* name) {  
            if ( name != nullptr ) {
                _name = name; 
            } else {
                _name.clear(); 
            }
        }
        const TCHAR* Name() const { 
            return ( _name.empty() == false ? _name.c_str() : nullptr ); 
        }

    private:
        string _name;
    };

    template <const TCHAR** MODULENAME>
    EXTERNAL_HIDDEN string CallsignTLS::CallsignAccess<MODULENAME>::_moduleName;
}

} 

#endif

