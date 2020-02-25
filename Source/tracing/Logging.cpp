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

#include "Logging.h"

#ifndef __WINDOWS__
#include <syslog.h>
#endif

namespace WPEFramework {
namespace Logging {

    template<>
    /* static */ LoggingType<Startup>::template LoggingControl<Startup> LoggingType<Startup>::s_LogControl;
    template<>
    /* static */ LoggingType<Shutdown>::template LoggingControl<Shutdown> LoggingType<Shutdown>::s_LogControl;
    template<>
    /* static */ LoggingType<Notification>::template LoggingControl<Notification> LoggingType<Notification>::s_LogControl;

    static const string LoggingToConsole(_T("LOGGING_TO_CONSOLE"));

    static bool DetectLoggingOutput()
    {
        string result;

        if (Core::SystemInfo::GetEnvironment(LoggingToConsole, result) == true) {
            return (result[0] != '1');
        }
        return (true);
    }

    /* static */ const char* MODULE_LOGGING = _T("SysLog");
    static uint64_t _baseTime(Core::Time::Now().Ticks());
    static bool _syslogging = DetectLoggingOutput();

    void SysLog(const bool toConsole)
    {
        _syslogging = !toConsole;
        Core::SystemInfo::SetEnvironment(LoggingToConsole, (toConsole ? _T("1") : nullptr));
    }

    void SysLog(const char fileName[], const uint32_t lineNumber, const Trace::ITrace* information)
    {
        // Time to printf...
        Core::Time now(Core::Time::Now());

#ifndef __WINDOWS__
        if (_syslogging == true) {
            string time(now.ToRFC1123(true));
            syslog(LOG_NOTICE, "[%s]:[%s:%d]: %s: %s\n", time.c_str(), Core::FileNameOnly(fileName), lineNumber, information->Category(), information->Data());
        } else
#endif
        {
            printf("[%11ju us] %s\n", static_cast<uintmax_t>(now.Ticks() - _baseTime), information->Data());
        }
    }

}
} // namespace PluginHost
