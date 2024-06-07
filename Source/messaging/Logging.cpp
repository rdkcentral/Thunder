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
#include "LoggingCategories.h"
#include <fstream>

namespace Thunder {

namespace Logging {

    // Announce upfront all SYSLOG categories...
    SYSLOG_ANNOUNCE(Crash);
    SYSLOG_ANNOUNCE(Startup);
    SYSLOG_ANNOUNCE(Shutdown);
    SYSLOG_ANNOUNCE(Fatal);
    SYSLOG_ANNOUNCE(Error);
    SYSLOG_ANNOUNCE(ParsingError);
    SYSLOG_ANNOUNCE(Notification);

    static const TCHAR* UnknownCallsign = {_T("NoTLSCallsign") };

    // force linkage of UnknownCallsign so it can be used as template argument (seems that C++17 onwards it should no longer be needed)
    extern void Force(const TCHAR**);
    template<typename DUMMY> 
    void ForceLinkage() {
        Force(&UnknownCallsign);
    }

    void DumpException(const string& exceptionType)
    {

        uint8_t counter = 0;
        std::list<Core::callstack_info> stack;
        DumpCallStack(Core::Thread::ThreadId(), stack);

#if defined(__CORE_EXCEPTION_CATCHING__) || defined(__CORE_WARNING_REPORTING__)

        const TCHAR* callsign = Core::CallsignTLS::CallsignAccess<&UnknownCallsign>::Callsign();
#else
        const TCHAR* callsign = UnknownCallsign;
#endif

        SYSLOG(Logging::Crash, (_T("-== Unhandled exception in: %s [%s] ==-\n"), callsign, exceptionType.c_str()));
        
        for (const Core::callstack_info& entry : stack) {
            if (entry.line != static_cast<uint32_t>(~0)) {
                SYSLOG(Logging::Crash, (Core::Format(_T("[%03d] [%p] %.30s %s [%d]"), counter, entry.address, entry.module.c_str(), entry.function.c_str(), entry.line)));
            }
            else {
                SYSLOG(Logging::Crash, (Core::Format(_T("[%03d] [%p] %.30s %s"), counter, entry.address, entry.module.c_str(), entry.function.c_str())));
            }
            counter++;
        }
    }

    void DumpSystemFiles(const Core::process_t pid)
    {
        static auto logProcPath = [](const std::string& path) {
            std::ifstream fileStream(path);
            
            if (fileStream.is_open()) {
                SYSLOG(Logging::Crash, (_T("-== %s ==-\n"), path.c_str()));
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

} // namespace Logging
}
