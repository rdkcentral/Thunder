/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological Management
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

#include "Module.h"
namespace WPEFramework {
namespace Logging {
    class EXTERNAL SyslogMonitorClient
    {
        public:
            virtual void NotifyLog(const std::string& logmessage) = 0;
    };
    class EXTERNAL SyslogMonitor
    {
        public:
            SyslogMonitor(const SyslogMonitor& ) = delete;
            SyslogMonitor& operator= (const SyslogMonitor& ) = delete;
            void RegisterClient(SyslogMonitorClient* client)
            {
                _monitorClients.push_back(client);
                return;
            }
            void UnregisterClient(SyslogMonitorClient* client)
            {
                auto iter = std::find(begin(_monitorClients), end(_monitorClients), client);
                if( iter != _monitorClients.end())
                {
                    _monitorClients.erase(iter);
                }
                return;
            }
            static SyslogMonitor& Instance()
            {
                static SyslogMonitor syslogMonitor;
                return syslogMonitor;
            }
            void SendLogMessage(const std::string& logMessage)
            {
                for(auto& client: _monitorClients)
                {
                    client->NotifyLog(logMessage);
                }
                return;
            }
            ~SyslogMonitor() = default;
        private:
            SyslogMonitor():_monitorClients(){}

        private:
            std::vector<SyslogMonitorClient*> _monitorClients;
    };
}
}