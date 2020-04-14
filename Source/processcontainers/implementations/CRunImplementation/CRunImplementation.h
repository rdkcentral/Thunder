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

#include "processcontainers/ProcessContainer.h"
#include "processcontainers/implementations/common/CGroupContainerInfo.h"

// TODO: Find more elegant alternative
extern "C" {
    #include <crun/error.h>
    #include <crun/container.h>
    #include <crun/utils.h>
    #include <crun/status.h>
}

namespace WPEFramework {
namespace ProcessContainers {

    class CRunNetworkInterfaceIterator : public NetworkInterfaceIterator
    {
    public:
        CRunNetworkInterfaceIterator();
        ~CRunNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;
        std::string IP(uint32_t id) const override;
    };

    class CRunContainer : public CGroupContainerInfo, virtual public IContainer 
    {
    public:
        CRunContainer(string name, string path, string logPath);
        virtual ~CRunContainer();

        // IContainerMethods
        const string Id() const override;
        uint32_t Pid() const override;
        NetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;

    private:
        uint32_t ClearLeftovers();
        void OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters);

        mutable uint32_t _refCount;
        bool _created; // keeps track if container was created and needs deletion
        string _name;
        string _bundle;
        string _configFile;
        string _logPath;
        libcrun_container_t *_container;
        libcrun_context_t _context;
        mutable Core::OptionalType<uint32_t> _pid;
        libcrun_error_t _error;
    };

    class CRunContainerAdministrator : public IContainerAdministrator 
    {
        friend class CRunContainer;
    public:
        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        CRunContainerAdministrator();
        ~CRunContainerAdministrator();

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;

    protected:
        void RemoveContainer(CRunContainer* container);

    private:
        std::list<IContainer*> _containers;
        mutable uint32_t _refCount;
    };
}
}
