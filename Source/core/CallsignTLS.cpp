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

#include "CallsignTLS.h"
#include "Thread.h"

#if defined(__CORE_WARNING_REPORTING__) || defined(__CORE_EXCEPTION_CATCHING__)

namespace Thunder {
namespace Core {

    const TCHAR* CallsignTLS::Callsign() {

        Core::ThreadLocalStorageType<CallsignTLS>& instance = Core::ThreadLocalStorageType<CallsignTLS>::Instance();
        const TCHAR* name = nullptr;
        if( ( instance.IsSet() == true ) && ( instance.Context().Name() != nullptr ) ) {
            name = instance.Context().Name(); // should be safe, nobody should for this thread be able to change this while we are using it 
        }
        return name;
    }

    void CallsignTLS::Callsign(const TCHAR* callsign) {
        Core::ThreadLocalStorageType<CallsignTLS>::Instance().Context().Name(callsign);
    }
}
} 

#endif // defined(__CORE_WARNING_REPORTING__) || defined(__CORE_EXCEPTION_CATCHING__)
