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

#include "Module.h"
#include "IShell.h"
#include "Configuration.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::IShell::state)

    { PluginHost::IShell::UNAVAILABLE, _TXT("Unavailable") },
    { PluginHost::IShell::DEACTIVATED, _TXT("Deactivated") },
    { PluginHost::IShell::DEACTIVATION, _TXT("Deactivation") },
    { PluginHost::IShell::ACTIVATED, _TXT("Activated") },
    { PluginHost::IShell::ACTIVATION, _TXT("Activation") },
    { PluginHost::IShell::PRECONDITION, _TXT("Precondition") },
    { PluginHost::IShell::DESTROYED, _TXT("Destroyed") },

ENUM_CONVERSION_END(PluginHost::IShell::state)

ENUM_CONVERSION_BEGIN(PluginHost::IShell::reason)

    { PluginHost::IShell::REQUESTED, _TXT("Requested") },
    { PluginHost::IShell::AUTOMATIC, _TXT("Automatic") },
    { PluginHost::IShell::FAILURE, _TXT("Failure") },
    { PluginHost::IShell::MEMORY_EXCEEDED, _TXT("MemoryExceeded") },
    { PluginHost::IShell::STARTUP, _TXT("Startup") },
    { PluginHost::IShell::SHUTDOWN, _TXT("Shutdown") },
    { PluginHost::IShell::CONDITIONS, _TXT("Conditions") },
    { PluginHost::IShell::WATCHDOG_EXPIRED, _TXT("WatchdogExpired") },

ENUM_CONVERSION_END(PluginHost::IShell::reason)

ENUM_CONVERSION_BEGIN(PluginHost::IShell::startup)

    { PluginHost::IShell::startup::UNAVAILABLE, _TXT("Unavailable") },
    { PluginHost::IShell::startup::DEACTIVATED, _TXT("Deactivated") },
    { PluginHost::IShell::startup::SUSPENDED,   _TXT("Suspended")   },
    { PluginHost::IShell::startup::RESUMED,     _TXT("Resumed")     },
    { PluginHost::IShell::startup::RESUMED,     _TXT("Activated")   },

ENUM_CONVERSION_END(PluginHost::IShell::startup)

namespace PluginHost
{
    void* IShell::Root(uint32_t & pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version)
    {
        pid = 0;
        void* result = nullptr;
        Plugin::Config::RootConfig rootConfig(this);

        // Note: when both new and old not set this one will revert to the old default which was inprocess 
        //       when both set the Old one is ignored
        if ( (( !rootConfig.Mode.IsSet() ) && ( rootConfig.OutOfProcess.Value() == false )) ||
             ((  rootConfig.Mode.IsSet() ) && ( rootConfig.Mode == Plugin::Config::RootConfig::ModeType::OFF )) ) { 

            string locator(rootConfig.Locator.Value());

            if (locator.empty() == true) {
                result = Core::ServiceAdministrator::Instance().Instantiate(Core::Library(), className.c_str(), version, interface);
            } else {
                std::vector<string> all_paths = GetLibrarySearchPaths(locator);
                std::vector<string>::const_iterator index = all_paths.begin();
                while ((result == nullptr) && (index != all_paths.end())) {
                    Core::File file(index->c_str());
                    if (file.Exists())
                    {
                        Core::Library resource(index->c_str());
                        if (resource.IsLoaded())
                            result = Core::ServiceAdministrator::Instance().Instantiate(
                                resource,
                                className.c_str(),
                                version,
                                interface);
                    }
                    index++;
                }
            }
        } else {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                string locator(rootConfig.Locator.Value());
                if (locator.empty() == true) {
                    locator = Locator();
                }
                RPC::Object definition(locator,
                    className,
                    Callsign(),
                    interface,
                    version,
                    rootConfig.User.Value(),
                    rootConfig.Group.Value(),
                    rootConfig.Threads.Value(),
                    rootConfig.Priority.Value(),
                    rootConfig.HostType(),
                    rootConfig.SystemRootPath.Value(),
                    rootConfig.RemoteAddress.Value(),
                    rootConfig.Configuration.Value());

                result = handler->Instantiate(definition, waitTime, pid);
            }
        }

        return (result);
    }
}

ENUM_CONVERSION_BEGIN(Plugin::Config::RootConfig::ModeType)

    { Plugin::Config::RootConfig::ModeType::OFF, _TXT("Off") },
    { Plugin::Config::RootConfig::ModeType::LOCAL, _TXT("Local") },
    { Plugin::Config::RootConfig::ModeType::CONTAINER, _TXT("Container") },
    { Plugin::Config::RootConfig::ModeType::DISTRIBUTED, _TXT("Distributed") },

ENUM_CONVERSION_END(Plugin::Config::RootConfig::ModeType);

} // namespace 
