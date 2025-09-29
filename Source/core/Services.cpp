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
 
#include "Services.h"

namespace Thunder {
namespace Core {
    /* static */ ServiceAdministrator ServiceAdministrator::_systemServiceAdministrator;

    /* static */ ServiceAdministrator& ServiceAdministrator::Instance()
    {
        return (_systemServiceAdministrator);
    }

    void* ServiceAdministrator::Instantiate(const Library& library, const char name[], const uint32_t version, const uint32_t interfaceId)
    {
        bool found(false);
        void* result(nullptr);
        const IService* startPoint(nullptr);

        ASSERT(library.IsLoaded() == true);
        System::GetModuleServicesImpl service = reinterpret_cast<System::GetModuleServicesImpl>(library.LoadFunction(_T("GetModuleServices")));

        if (service != nullptr) {
            startPoint = service();
        }

        // Now lets see if we can find what we need to instantiate..
        while ( (startPoint != nullptr) && (found == false) ) {
            const IService::IMetadata* info (startPoint->Info());
            
            if ( ((version == static_cast<uint32_t>(~0)) || (version == static_cast<uint32_t>((info->Major() << 8) | info->Minor()))) &&
                 (strcmp(info->Name(), name) == 0) ) {
                found = true;
            }
            else {
                startPoint = startPoint->Next();
            }
        }

        if (startPoint != nullptr) {
            result = startPoint->Create(interfaceId);
        }

        if(result == nullptr){
            TRACE_L1("Missing implementation classname %s in library %s\n", name, library.Name().c_str());
        }

        return (result);
    }

    void ServiceAdministrator::ReleaseLibrary(Library&& reference)
    {
        _adminLock.Lock();
        _unreferencedLibraries.emplace_back(std::move(reference));
        _adminLock.Unlock();
    }

    void ServiceAdministrator::FlushLibraries()
    {
        _adminLock.Lock();
        while (_unreferencedLibraries.size() != 0) {
            Library lib = _unreferencedLibraries.back();
            _unreferencedLibraries.pop_back();
            _adminLock.Unlock();
            lib.Release();
            // A few closing code instructions might still be required for 
            // that thread that submitted the library to complete, so at 
            // least give that thread a slice to complete the last few 
            // instructions before we close down the library (if it is 
            // the last reference)
            lib.WaitUnloaded(5000);
            _adminLock.Lock();
        }
        _adminLock.Unlock();
    }
}
} // namespace Core
