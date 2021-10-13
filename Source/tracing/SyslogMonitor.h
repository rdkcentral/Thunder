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
#include "ISyslogMonitorClient.h"
namespace WPEFramework {
namespace Logging {
    class EXTERNAL SyslogMonitor
    {
        public:
            SyslogMonitor(const SyslogMonitor& ) = delete;
            SyslogMonitor& operator= (const SyslogMonitor& ) = delete;
            void RegisterClient(ISyslogMonitorClient* client)
            {
                ASSERT(client!=nullptr);
                _lock.Lock();
                auto iter = std::find(begin(_monitorClients), end(_monitorClients), client);
                if( iter == _monitorClients.end())
                {
                    _monitorClients.push_back(client);
                }
                _lock.Unlock();
            }
            void UnregisterClient(ISyslogMonitorClient* client)
            {
                ASSERT(client != nullptr);
                _lock.Lock();
                auto iter = std::find(begin(_monitorClients), end(_monitorClients), client);
                if( iter != _monitorClients.end())
                {
                    _monitorClients.erase(iter);
                }
                _lock.Unlock();
            }
            static SyslogMonitor& Instance()
            {
                static SyslogMonitor& _syslogMonitor = Core::SingletonType<SyslogMonitor>::Instance();
                return _syslogMonitor;
            }
            void SendLogMessage(const string& logMessage)
            {
                _lock.Lock();
                for(auto& client: _monitorClients)
                {
                    client->NotifyLog(logMessage);
                }
                _lock.Unlock();
            }
            ~SyslogMonitor() = default;
        protected:
            SyslogMonitor():_monitorClients()
                            , _lock()
            {

            }

        private:
            std::vector<ISyslogMonitorClient*> _monitorClients;
            Core::CriticalSection _lock;
    };
}
}
