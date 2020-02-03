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
