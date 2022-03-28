/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

        const char* MODULE_LOGGING = _T("SysLog");
    }
    template <>
    WPEFramework::Messaging::ControlLifetime<Logging::Startup, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::template Control<Logging::Startup, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING> Messaging::ControlLifetime<Logging::Startup, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::_messageControl;
    template <>
    WPEFramework::Messaging::ControlLifetime<Logging::Shutdown, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::template Control<Logging::Shutdown, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING> Messaging::ControlLifetime<Logging::Shutdown, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::_messageControl;
    template <>
    WPEFramework::Messaging::ControlLifetime<Logging::Notification, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::template Control<Logging::Notification, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING> Messaging::ControlLifetime<Logging::Notification, &Logging::MODULE_LOGGING, WPEFramework::Core::Messaging::MetaData::MessageType::LOGGING>::_messageControl;

namespace Logging {
    static const TCHAR* UnknownCallsign = _T("NoTLSCallsign");

    void DumpException(const string& exceptionType)
    {
        std::list<string> stack;
        DumpCallStack(Core::Thread::ThreadId(), stack);
#if defined(__CORE_EXCEPTION_CATCHING__) || defined(__CORE_WARNING_REPORTING__)
        const TCHAR* callsign = Core::CallsignTLS::CallsignAccess<&UnknownCallsign>::Callsign();
#else
        const TCHAR* callsign = UnknownCallsign;
#endif
        SYSLOG(Logging::Crash, (_T("-== Unhandled exception in: %s [%s] ==-\n"), callsign, exceptionType.c_str()));
        for (const string& line : stack) {
            SYSLOG(Logging::Crash, (line));
        }
    }

    void DumpSystemFiles(const Core::process_t pid)
    {
        static auto logProcPath = [](const std::string& path) {
            std::ifstream fileStream(path);
            if (fileStream.is_open()) {
                SYSLOG(Logging::Crash, ("-== %s ==-\n", path.c_str()));
                std::string line;
                while (std::getline(fileStream, line)) {
                    SYSLOG(Logging::Crash, (line));
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

#ifdef __CORE_EXCEPTION_CATCHING__
    namespace {
        class ExceptionCatcher : Core::Thread::IExceptionCallback {
        public:
            ExceptionCatcher()
            {
                Core::Thread::ExceptionCallback(this);
            }
            ~ExceptionCatcher() override
            {
                Core::Thread::ExceptionCallback(nullptr);
            }

            void Exception(const string& message) override
            {
                DumpException(message);
            }
        };

        static ExceptionCatcher exceptionCatcher;

    }
#endif
}
} // namespace PluginHost
