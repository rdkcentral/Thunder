
/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "processcontainers/process_containers.h"
#include "processcontainers/ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    class CNetworkInterfaceIterator : public NetworkInterfaceIterator 
    {
    public:
        CNetworkInterfaceIterator(const ProcessContainer* container);
        ~CNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;

        std::string IP(uint32_t id) const override;
    private:
        ProcessContainerNetworkStatus _networkStatus;
    };
    
    class CContainer : public IContainer 
    {
    private:
        friend class CContainerAdministrator;

        CContainer(ProcessContainer* container);
        CContainer(const CContainer&) = delete;
        CContainer& operator=(const CContainer&) = delete;
    public:
        virtual ~CContainer();

        // IContainerMethods
        const string Id() const override;
        uint32_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        NetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;

    private:
        ProcessContainer* _container;
        mutable uint32_t _refCount;
        std::vector<string> _networkInterfaces;
    };

    class CContainerAdministrator : public IContainerAdministrator 
    {
    private:
        friend class CContainer;
        friend class Core::SingletonType<CContainerAdministrator>;

        CContainerAdministrator();
    public:
        CContainerAdministrator(const CContainerAdministrator&) = delete;
        CContainerAdministrator& operator=(const CContainerAdministrator&) = delete;

        ~CContainerAdministrator();

        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

    protected:
        void RemoveContainer(IContainer*);

    private:
        std::list<IContainer*> _containers;
        Core::CriticalSection _adminLock;
    };
}
}