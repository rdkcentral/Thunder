/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

#include "ContainerAdministrator.h"
#include "common/BaseContainerIterator.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(ProcessContainers::IContainer::containertype)

    { ProcessContainers::IContainer::containertype::LXC, _TXT("lxc") },
    { ProcessContainers::IContainer::containertype::RUNC, _TXT("runc") },
    { ProcessContainers::IContainer::containertype::CRUN, _TXT("crun") },
    { ProcessContainers::IContainer::containertype::DOBBY, _TXT("dobby") },
    { ProcessContainers::IContainer::containertype::AWC, _TXT("awc") },

ENUM_CONVERSION_END(ProcessContainers::IContainer::containertype)


namespace ProcessContainers {

    uint32_t ContainerAdministrator::Initialize(const string& configuration)
    {
        uint32_t result = Core::ERROR_NONE;

        _adminLock.Lock();

        for (auto& runtime : _producers) {

            const Core::EnumerateType<IContainer::containertype> value(runtime.first);
            ASSERT(value.IsSet() == true);

            // Extract runtime system specific config, if any
            Core::JSON::String specific;
            Core::JSON::Container container;
            container.Add(value.Data(), &specific);
            container.FromString(configuration);

            TRACE(Trace::Information, (_T("Initializing container runtime system '%s'..."), value.Data()));

            // Pass the configuration to the runtime
            const uint32_t initResult = runtime.second->Initialize(specific);

            if (initResult != Core::ERROR_NONE) {
                TRACE(Trace::Error, (_T("Initialization failure")));
                result = Core::ERROR_GENERAL;
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    void ContainerAdministrator::Deinitialize()
    {
        _adminLock.Lock();

        for (auto& runtime : _producers) {
            runtime.second->Deinitialize();
        }

        _adminLock.Unlock();
    }

    Core::ProxyType<IContainer> ContainerAdministrator::Container(const IContainer::containertype type, const string& id,
                                        IStringIterator& searchPaths, const string& logPath, const string& configuration)
    {
        Core::ProxyType<IContainer> container;

        _adminLock.Lock();

        auto it = _producers.find(type);

        const Core::EnumerateType<IContainer::containertype> value(type);
        DEBUG_VARIABLE(value);

        ASSERT(value.IsSet() == true);

        if (it != _producers.end()) {

            auto& runtime = (*it).second;
            ASSERT(runtime != nullptr);

            container = runtime->Container(id, searchPaths, logPath, configuration);

            if (container.IsValid() == true) {
               TRACE(Trace::Information, (_T("Container '%s' created successfully (runtime system '%s')"),
                        container->Id().c_str(), value.Data()));
            }
        }
        else {
            TRACE(Trace::Error, (_T("Container runtime system '%s' is not enabled!"), value.Data()));
        }

        _adminLock.Unlock();

        return (container);
    }

    Core::ProxyType<IContainer> ContainerAdministrator::Get(const string& id)
    {
        Core::ProxyType<IContainer> container;

        _containers.Visit([&container, &id](Core::ProxyType<IContainer>& entry) -> bool {
            if (entry->Id() == id) {
                container = entry;
                return (true);
            } else {
                return (false);
            }
        });

        return (container);
    }

    IContainerIterator* ContainerAdministrator::Containers() const
    {
        std::vector<string> containers;

        _containers.Visit([&containers](const Core::ProxyType<IContainer>& entry) -> bool {
            containers.push_back(entry->Id());
            return (false);
        });

        return (new BaseContainerIterator(std::move(containers)));
    }

} // namespace ProcessContainers
}