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
                result = Core::ServiceAdministrator::Instance().Instantiate(Core::Library(), className.c_str(), version, interface);
            } else {
                RPC::IStringIterator* all_paths = GetLibrarySearchPaths(locator);
                ASSERT(all_paths != nullptr);

                string element;
                while ((all_paths->Next(element) == true) && (result == nullptr)) {
                    Core::File file(element.c_str());
                    if (file.Exists()) {

                        Core::Library resource = Core::ServiceAdministrator::Instance().LoadLibrary(element.c_str());
                        if (resource.IsLoaded())
                            result = Core::ServiceAdministrator::Instance().Instantiate(
                                resource,
                                className.c_str(),
                                version,
                                interface);
                    }
                }
                all_paths->Release();
            }
        } else {
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

ENUM_CONVERSION_END(Plugin::Config::RootConfig::ModeType)

} // namespace 
