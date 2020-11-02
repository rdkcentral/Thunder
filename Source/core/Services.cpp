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
 
#include "Services.h"

namespace WPEFramework {
namespace Core {
    /* static */ ServiceAdministrator ServiceAdministrator::_systemServiceAdministrator;

    ServiceAdministrator::ServiceAdministrator()
        : _adminLock()
        , _services()
        , _instanceCount(0)
        , _callback(nullptr)
        , _unreferencedLibraries()
    {
    }

    /* virtual */ ServiceAdministrator::~ServiceAdministrator()
    {
    }

    void ServiceAdministrator::Register(IServiceMetadata* service)
    {
        // Only register a service once !!!
        ASSERT(std::find(_services.begin(), _services.end(), service) == _services.end());

        _services.push_back(service);
    }

    void ServiceAdministrator::Unregister(IServiceMetadata* service)
    {
        std::list<IServiceMetadata*>::iterator index = std::find(_services.begin(), _services.end(), service);

        // Only unregister a service once !!!
        ASSERT(index != _services.end());

        _services.erase(index);
    }

    /* static */ ServiceAdministrator& ServiceAdministrator::Instance()
    {
        return (_systemServiceAdministrator);
    }

    void* ServiceAdministrator::Instantiate(const Library& library, const char name[], const uint32_t version, const uint32_t interfaceNumber)
    {
        bool found = false;
        std::list<IServiceMetadata*>::iterator index = _services.begin();

        while ((index != _services.end()) && (found == false)) {
            const char* thisName = (*index)->Name().c_str();
            found = ((strcmp(thisName, name) == 0) && ((version == static_cast<uint32_t>(~0)) || (version == (*index)->Version())));

            if (found == false) {
                index++;
            }
        }

        if(found == false){
            TRACE_L1("Missing implementation classname %s in library %s\n", name, library.Name().c_str());
        }

        return (found == true ? (*index)->Create(library, interfaceNumber) : nullptr);
    }

    void ServiceAdministrator::ReleaseLibrary(Library& reference)
    {
        _adminLock.Lock();
            _unreferencedLibraries.push_back(reference);
        reference.Release();
        _adminLock.Unlock();
    }

    void ServiceAdministrator::FlushLibraries()
    {
        _adminLock.Lock();
        while (_unreferencedLibraries.size() != 0) {
            _unreferencedLibraries.pop_front();
        }
        _adminLock.Unlock();
    }
}
} // namespace Core
