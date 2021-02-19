/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

   EXPECT_STREQ("SysLog",Logging::LoggingType<Logging::Notification>("Hello").Module());
   EXPECT_STREQ("Notification",Logging::LoggingType<Logging::Notification>("Hello").Category());
   EXPECT_STREQ("Hello",Logging::LoggingType<Logging::Notification>("Hello").Data());
   EXPECT_EQ(Logging::LoggingType<Logging::Notification>("Hello").Length(),5);
   EXPECT_EQ(Logging::LoggingType<Logging::Startup>("Hello").Length(),5);
   EXPECT_EQ(Logging::LoggingType<Logging::Shutdown>("Hello").Length(),5);
   EXPECT_TRUE(Logging::LoggingType<Logging::Notification>("Hello").Enabled()) << "TraceControl not Enabled";
   Logging::LoggingType<Logging::Notification>("Hello").Enabled(true);
   Logging::LoggingType<Logging::Notification>("Hello").Destroy();

   Core::Singleton::Dispose();
}
