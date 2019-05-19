#include "Module.h"

#include "ProcessContainer.h"

#include "Tracing.h"
#include <lxc/lxccontainer.h>
#include <vector>
#include <utility>

namespace WPEFramework {

class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
private:
    using LxcContainerType = struct lxc_container;
public:
    LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
    LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;

    LXCContainerAdministrator() 
        : _lock()
        , _searchpaths() {
        }

    virtual ~LXCContainerAdministrator() = default;

    class LCXContainer : public ProcessContainers::IContainerAdministrator::IContainer {
    public:
        LCXContainer(const LCXContainer&) = delete;
        LCXContainer& operator=(const LCXContainer&) = delete;

        LCXContainer(const string& name, LxcContainerType* lxccontainer)
            : _name(name)
            , _referenceCount(1)
            , _lxccontainer(lxccontainer) {
        }

        const string& Id() const override {
            return _name;
        }

        void Start() override;

        void AddRef() const override {
            WPEFramework::Core::InterlockedIncrement(_referenceCount);
            lxc_container_get(_lxccontainer);
        }

        uint32_t Release() const override {
            uint32_t retval = WPEFramework::Core::ERROR_NONE;
            uint32_t lxcresult = lxc_container_put(_lxccontainer);
            if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
                ASSERT(lxcresult == 1); // if 1 is returned, lxc also released the container
//                TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition with name %s released"), _name));

                delete this;
                retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;

            }
            return retval;
        }

        private:
            const string _name;
            mutable uint32_t _referenceCount;
            LxcContainerType* _lxccontainer;
    };

    void ContainerDefinitionSearchPaths(const std::vector<string>&& searchpaths) override {
        _lock.Lock();
        _searchpaths = std::move(searchpaths);
        _lock.Unlock();
    }

    ProcessContainers::IContainerAdministrator::IContainer* Container(const string& name) override;

    string GetNames() const override;

private:
    using SearchPathContainer = std::vector<string>;

    mutable Core::CriticalSection _lock;
    SearchPathContainer _searchpaths;
};

ProcessContainers::IContainerAdministrator::IContainer* LXCContainerAdministrator::Container(const string& name) {

    _lock.Lock();

    _searchpaths.push_back({string("/home/marcelf/.local/share/lxc/")});  // todo remove, after configuration in place. let's first get the container to work

    ProcessContainers::IContainerAdministrator::IContainer* container { nullptr };
    SearchPathContainer::const_iterator searchpath { _searchpaths.cbegin() };

    while( ( container == nullptr ) && ( searchpath != _searchpaths.cend() ) )  {
        LxcContainerType **clist = nullptr;
        int32_t numberofcontainersfound = list_defined_containers((*searchpath).c_str(), nullptr, &clist);
        int32_t index = 0;
        while( ( container == nullptr) && ( index < numberofcontainersfound ) ) {
            LxcContainerType *c = clist[index];
            if( name == c->name ) {
//                TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition with name %s retreived from %s."), c->name, (*searchpath).c_str()));
                container = new LXCContainerAdministrator::LCXContainer(name, c);
            }
            else {
                lxc_container_put(c);
            }
            ++index;
        };
        if( numberofcontainersfound > 0 ) {
            free(clist);
        }
    };

    _lock.Unlock();

    return container;
}

string LXCContainerAdministrator::GetNames() const {
    char **names;
    int32_t result = list_defined_containers("/home/marcelf/.local/share/lxc/", &names, NULL);
    string namecollection;
    for (int32_t i = 0; i < result; ++i) {
        if( i > 0 ) {
            namecollection += ", ";
        }
        namecollection += names[i];
		free(names[i]);
	}
    if( result > 0 ) {
        free(names);
    }
    return namecollection;
}

ProcessContainers::IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
{
    static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

    return myLXCContainerAdministrator;
}

void LXCContainerAdministrator::LCXContainer::Start() {
    _lxccontainer->start(_lxccontainer, true, nullptr);    
}

} //namespace WPEFramework 
