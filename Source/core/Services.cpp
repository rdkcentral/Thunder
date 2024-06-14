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

    void ServiceAdministrator::Announce(IService* service)
    {
        _adminLock.Lock();

        // Only register a service once !!!
        ASSERT(std::find(_services.begin(), _services.end(), service) == _services.end());

        _services.push_back(service);

        _adminLock.Unlock();
    }

    void ServiceAdministrator::Revoke(IService* service)
    {
        _adminLock.Lock();

        Services::iterator index = std::find(_services.begin(), _services.end(), service);

        // Only unregister a service once !!!
        ASSERT(index != _services.end());

        _services.erase(index);

        _adminLock.Unlock();
    }

    /* static */ ServiceAdministrator& ServiceAdministrator::Instance()
    {
        return (_systemServiceAdministrator);
    }

    void* ServiceAdministrator::Instantiate(const Library& library, const char name[], const uint32_t version, const uint32_t interfaceNumber)
    {
        void* result = nullptr;

        _adminLock.Lock();

        Services::iterator index = _services.begin();

        while ((index != _services.end()) && (result == nullptr)) {
            const IService::IMetadata* info((*index)->Metadata());

            if ((strcmp(info->ServiceName(), name) == 0) && ((version == static_cast<uint32_t>(~0)) || (version == static_cast<uint32_t>((info->Major() << 8) | info->Minor())))) {
                result = (*index)->Create(library, interfaceNumber);
            }
            index++;
        }

        _adminLock.Unlock();

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
            // A few closing code instructions might still be required for 
            // that thread that submitted the librray to complete, so at 
            // least give that thread a slice to complete the last few 
            // instructions before we close down the librray (if it is 
            // the last reference)
            std::this_thread::yield();

            _unreferencedLibraries.pop_front();
        }
        _adminLock.Unlock();
    }
}
} // namespace Core
