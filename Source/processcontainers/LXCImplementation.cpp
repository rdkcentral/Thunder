#include "Module.h"

#include "ProcessContainer.h"

#include <lxc/lxccontainer.h>


namespace WPEFramework {

class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
public:
    LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
    LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;
    LXCContainerAdministrator() = default;

    class LCXContainer : public ProcessContainers::IContainerAdministrator::IContainer {
    public:
        LCXContainer(const LCXContainer&) = delete;
        LCXContainer& operator=(const LCXContainer&) = delete;
    };

    ProcessContainers::IContainerAdministrator::IContainer* Create() override;

    string GetNames() const override;


private:
};

ProcessContainers::IContainerAdministrator::IContainer* LXCContainerAdministrator::Create() {
    return nullptr;
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


ProcessContainers::IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance(const std::string& configuration)
{
    static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

    return myLXCContainerAdministrator;
}

} //namespace WPEFramework 
