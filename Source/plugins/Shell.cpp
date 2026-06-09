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

namespace Thunder {

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
                locator = (this->Locator());
            } 

            if (locator.empty() == true) {
                SYSLOG(Logging::Error, (_T("Root object [%s] for plugin [%s] could not be instantiated in process: no locator configured"), className.c_str(), Callsign().c_str()));
            } else {
                RPC::IStringIterator* all_paths = GetLibrarySearchPaths(locator);
                ASSERT(all_paths != nullptr);

                string lastError;
                string lastPath;
                string element;
                while ((all_paths->Next(element) == true) && (result == nullptr)) {
                    Core::File file(element.c_str());
                    if (file.Exists()) {
                        lastPath = element;

                        Core::Library resource(element.c_str());
                        const Core::IService* loader;

                        if ( (resource.IsLoaded()) && ((loader = Core::ServiceAdministrator::LibraryToService(resource)) != nullptr) ) {

                            result = Core::ServiceAdministrator::Instance().Instantiate(
                                loader,
                                className.c_str(),
                                version,
                                interface);
                            if (result == nullptr) {
                                lastError = _T("Class/interface not found in library");
                            }
                        } else if (resource.IsLoaded() == false) {
                            lastError = resource.Error().empty() == false ? resource.Error() : _T("Library load failed");
                        } else {
                            lastError = _T("No service metadata found in library");
                        }
                    }
                }
                all_paths->Release();

                if (result == nullptr) {
                    if (lastPath.empty() == false) {
                        SYSLOG(Logging::Error, (_T("Root object [%s] for plugin [%s] could not be instantiated in process from locator [%s]. Candidate [%s], error [%s]"), className.c_str(), Callsign().c_str(), locator.c_str(), lastPath.c_str(), lastError.c_str()));
                    } else {
                        SYSLOG(Logging::Error, (_T("Root object [%s] for plugin [%s] could not be instantiated in process from locator [%s]: no library candidate found"), className.c_str(), Callsign().c_str(), locator.c_str()));
                    }
                }
            }
        } else {

            bool allowed = true;
            switch (rootConfig.Mode) {
                case Plugin::Config::RootConfig::ModeType::OFF:
                    ASSERT(false);
                    break;
                case Plugin::Config::RootConfig::ModeType::LOCAL:
                    allowed = AllowedLocal();
                    break;
                case Plugin::Config::RootConfig::ModeType::CONTAINER:
                    allowed = AllowedContainer();
                    break;
                case Plugin::Config::RootConfig::ModeType::DISTRIBUTED:
                    allowed = AllowedDistributed();
                    break;
                default:
                    ASSERT(false);
                    break;
                }

            if (allowed == true) {

                ICOMLink* handler(QueryInterface<ICOMLink>());

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
                        SystemRootPath(),
                        rootConfig.RemoteAddress.Value(),
                        rootConfig.Configuration.Value(),
                        rootConfig.Environment());

                    result = handler->Instantiate(definition, waitTime, pid);
                } else {
                    SYSLOG(Logging::Error, (_T("Root object [%s] for plugin [%s] could not be instantiated out-of-process, ICOMLink is unavailable"), className.c_str(), Callsign().c_str()));
                }
            } else {
                SYSLOG(Logging::Error, (_T("Root object [%s] for plugin [%s] configured root mode [%s], but this shell does not support it"), className.c_str(), Callsign().c_str(), rootConfig.Mode.Data()));
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

ENUM_CONVERSION_END(Plugin::Config::RootConfig::ModeType)

} // namespace 
