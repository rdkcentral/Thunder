#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <tracing/Logging.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

/* Prior to test execution, please export LOGGING_TO_CONSOLE=1*/
TEST(tracing_logging, simple_loggings)
{
   Logging::SysLog(true);
   Logging::LoggingType<Logging::Startup> loggingType("hello");
   Logging::LoggingType<Logging::Startup>::Enable((0x00000001) != 0);
   Logging::LoggingType<Logging::Shutdown>::Enable((0x00000002) != 0);
   Logging::LoggingType<Logging::Notification>::Enable((0x00000004) != 0);
   SYSLOG(Logging::Notification, ("Logging utility verification"));
   SYSLOG(Logging::Startup, ("Logging utility verification"));
   Logging::SysLog(false);
   SYSLOG(Logging::Shutdown, ("Logging utility verification"));

   ASSERT_STREQ("SysLog",Logging::LoggingType<Logging::Notification>("Hello").Module());
   ASSERT_STREQ("Notification",Logging::LoggingType<Logging::Notification>("Hello").Category());
   ASSERT_STREQ("Hello",Logging::LoggingType<Logging::Notification>("Hello").Data());
   ASSERT_EQ(Logging::LoggingType<Logging::Notification>("Hello").Length(),5);
   ASSERT_EQ(Logging::LoggingType<Logging::Startup>("Hello").Length(),5);
   ASSERT_EQ(Logging::LoggingType<Logging::Shutdown>("Hello").Length(),5);
   ASSERT_TRUE(Logging::LoggingType<Logging::Notification>("Hello").Enabled()) << "TraceControl not Enabled";
   Logging::LoggingType<Logging::Notification>("Hello").Enabled(true);
   Logging::LoggingType<Logging::Notification>("Hello").Destroy();

   Core::Singleton::Dispose();
}
