 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include <fstream>

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
            syslog(LOG_NOTICE, "[%s]:[%s]:[%s:%d]: %s: %s\n", time.c_str(), information->Module(), Core::FileNameOnly(fileName), lineNumber, information->Category(), information->Data());
        } else
#endif
        {
            printf("[%11ju us]:[%s]:[%s:%d]: %s: %s\n", static_cast<uintmax_t>(now.Ticks() - _baseTime), information->Module(), Core::FileNameOnly(fileName), lineNumber, information->Category(), information->Data());
        }
    }

    static const TCHAR* UnknownCallsign = _T("NoTLSCallsign");

    void DumpException(const string& exceptionType) {
        std::list<string> stack;
        DumpCallStack(Core::Thread::ThreadId(), stack);
        #ifdef __CORE_WARNING_REPORTING__
        const TCHAR* callsign = WarningReporting::CallsignTLS::CallsignAccess<&UnknownCallsign>::Callsign();
        #else
        const TCHAR* callsign = UnknownCallsign;
        #endif
        SYSLOG (Logging::Crash, (_T("-== Unhandled exception in: %s [%s] ==-\n"), callsign, exceptionType.c_str()));
        for (const string& line : stack)
        {
            SYSLOG(Logging::Crash, (line));
        }
    }

    void DumpSystemFiles(const Core::process_t pid)
    {
        static auto logProcPath = [](const std::string& path)
        {
            std::ifstream fileStream(path);
            if (fileStream.is_open()) {
                SYSLOG (Logging::Crash, ("-== %s ==-\n", path.c_str()));
                std::string line;
                while (std::getline(fileStream, line))
                {
                    SYSLOG (Logging::Crash, (line));
                }
            }
        };

        logProcPath("/proc/meminfo");
        logProcPath("/proc/loadavg");

        if (pid > 0) {
            std::string procPath = std::string("/proc/") + std::to_string(pid) + "/status";
            logProcPath(procPath);
        }
    }


}
} // namespace PluginHost
