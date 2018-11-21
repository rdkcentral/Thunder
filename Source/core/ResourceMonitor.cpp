#include "ResourceMonitor.h"
#include "Singleton.h"

namespace WPEFramework {

namespace Core {

/* static */ ResourceMonitor& ResourceMonitor::_instance (SingletonType< ResourceMonitor >::Instance() );

} } // namespace WPEFramework::Core
