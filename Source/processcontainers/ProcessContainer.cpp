#include "ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    IContainer* IContainerAdministrator::Find(const string& id)
    {
        auto iterator = Containers();
        IContainer* result = nullptr;

        while(iterator.Next() == true) {
            if (iterator.Current()->Id() == id) {
                result = &(*iterator.Current());
            }
        }

        return result;
    }

}
}