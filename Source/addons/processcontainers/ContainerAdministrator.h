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

#pragma once

#include "Module.h"

#include "IProcessContainers.h"

namespace Thunder {

namespace ProcessContainers {

    class EXTERNAL ContainerAdministrator {
    private:
        template<typename T>
        friend class Core::SingletonType;

        ContainerAdministrator()
            : _adminLock()
            , _producers()
            , _containers()
            , _default(IContainer::DEFAULT)
        {
        }

    public:
        ContainerAdministrator(const ContainerAdministrator&) = delete;
        ContainerAdministrator(ContainerAdministrator&&) = delete;
        ContainerAdministrator& operator=(const ContainerAdministrator&) = delete;
        ContainerAdministrator& operator=(ContainerAdministrator&&) = delete;

        ~ContainerAdministrator()
        {
            ASSERT(_containers.Count() == 0);
            ASSERT(_producers.empty() == true);
        }

    public:
        static ContainerAdministrator& Instance() {
            static ContainerAdministrator& instance = Core::SingletonType<ContainerAdministrator>::Instance();
            return (instance);
        }

    public:
        uint32_t Initialize(const string& configuration);
        void Deinitialize();

    public:
        template<typename T, typename... Args>
        Core::ProxyType<T> Create(Args&&... args) {
            return (_containers.Instance<T>(std::forward<Args>(args)...));
        }

    public:
        Core::ProxyType<IContainer> Container(const IContainer::containertype type, const string& id, 
                            IStringIterator& searchPaths, const string& logPath, const string& configuration);

        Core::ProxyType<IContainer> Get(const string& id);
        IContainerIterator* Containers() const;

    private:
        template <typename PRODUCER, const IContainer::containertype CONTAINERTYPE>
        friend class ContainerProducerRegistrationType;

    private:
        void Announce(const IContainer::containertype type, IContainerProducer* producer)
        {
            ASSERT(producer != nullptr);

            _adminLock.Lock();

            ASSERT(_producers.find(type) == _producers.end());

            // Announce another container runtime...
            _producers.emplace(type, producer);

            _adminLock.Unlock();
        }

        void Revoke(const IContainer::containertype type)
        {
            _adminLock.Lock();

            ASSERT(_producers.find(type) != _producers.end());

            // No longer available
            _producers.erase(type);

            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        std::map<IContainer::containertype, IContainerProducer*> _producers;
        Core::ProxyListType<IContainer> _containers;
        IContainer::containertype _default;
    };

} // namespace ProcessContainers

}
