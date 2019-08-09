#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <tracing/Logging.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(tracing_logging, simple_loggings)
{
   Logging::LoggingType<Logging::Startup>::Enable((0x00000001) != 0);
   Logging::LoggingType<Logging::Shutdown>::Enable((0x00000002) != 0);
   Logging::LoggingType<Logging::Notification>::Enable((0x00000004) != 0);
   Logging::SysLog(false);
   Logging::SysLog(true);
   SYSLOG(Logging::Startup, ("Logging utility verification"));
   SYSLOG(Logging::Shutdown, ("Logging utility verification"));
   SYSLOG(Logging::Notification, ("Logging utility verification"));
   Core::Singleton::Dispose();
}
