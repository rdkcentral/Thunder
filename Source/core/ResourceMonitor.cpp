#include "ResourceMonitor.h"
#include "Singleton.h"

namespace WPEFramework {

namespace Core {

    /* static */ ResourceMonitor& ResourceMonitor::Instance()
    {
        static ResourceMonitor& _instance = SingletonType<ResourceMonitor>::Instance();
        return (_instance);
    }
}
} // namespace WPEFramework::Core
