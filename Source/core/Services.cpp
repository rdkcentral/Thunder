#include "Services.h"

namespace WPEFramework {
namespace Core {
    static std::list<Library> UnreferencedLibraries;
    /* static */ ServiceAdministrator ServiceAdministrator::_systemServiceAdministrator;

    ServiceAdministrator::ServiceAdministrator()
        : _services()
        , _instanceCount(0)
    {
    }

    /* virtual */ ServiceAdministrator::~ServiceAdministrator()
    {
    }

    void ServiceAdministrator::Register(IServiceMetadata* service)
    {
        std::list<IServiceMetadata*>::iterator index = std::find(_services.begin(), _services.end(), service);

        // Only register a service once !!!
        ASSERT(index == _services.end());
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

    void ServiceAdministrator::ReleaseLibrary(const Library& reference)
    {
        UnreferencedLibraries.push_back(reference);
    }

    void ServiceAdministrator::FlushLibraries()
    {
        while (UnreferencedLibraries.size() != 0) {
            UnreferencedLibraries.pop_front();
        }
    }
}
} // namespace Core
