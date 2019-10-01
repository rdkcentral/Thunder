#include "ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    IContainerAdministrator::IContainer* IContainerAdministrator::Find(const string& name)
    {
        auto iterator = Containers();
        IContainer* result = nullptr;

        while(iterator.Next() == true) {
            if (iterator.Current()->Id() == name) {
                result = &(*iterator.Current());
            }
        }

        return result;
    }

}
}