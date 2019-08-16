#include "ResourceMonitor.h"
#include "Singleton.h"

namespace WPEFramework {

namespace Core {

    /* static */ ResourceMonitor& ResourceMonitor::Instance()
    {
        // Tests build/destroy the ResourceMonitor for each test. In production the
        // ResourceMonitor only gets built/destroyed once. The static variable "_instance"
        // makes this method about 5% faster. So here test code and production code are
        // different, but its impact has been thoroughly researched.
#ifdef BUILD_TESTS
        return SingletonType<ResourceMonitor>::Instance();
#else
        static ResourceMonitor& _instance = SingletonType<ResourceMonitor>::Instance();
        return (_instance);
#endif
    }
}
} // namespace WPEFramework::Core
