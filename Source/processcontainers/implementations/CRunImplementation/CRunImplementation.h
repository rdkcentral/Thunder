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

#pragma once

#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseAdministrator.h"
#include "processcontainers/common/BaseRefCount.h"
#include "processcontainers/common/CGroupContainerInfo.h"

extern "C" {
#include <crun/container.h>
#include <crun/error.h>
#include <crun/status.h>
#include <crun/utils.h>
}

namespace WPEFramework {
namespace ProcessContainers {

    class CRunContainer : public BaseRefCount<IContainer> {
    private:
        friend class CRunContainerAdministrator;

        CRunContainer(const string& name, const string& path, const string& logPath);
        CRunContainer(const CRunContainer&) = delete;
        CRunContainer& operator=(const CRunContainer&) = delete;

    public:
        ~CRunContainer() override;

        // IContainerMethods
        const string& Id() const override;
        uint32_t Pid() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;

    private:
        uint32_t ClearLeftovers();
        void OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters);

        mutable Core::CriticalSection _adminLock;
        bool _created; // keeps track if container was created and needs deletion
        string _name;
        string _bundle;
        string _configFile;
        string _logPath;
        libcrun_container_t* _container;
        libcrun_context_t _context;
        mutable Core::OptionalType<uint32_t> _pid;
        libcrun_error_t _error;
    };

    class CRunContainerAdministrator : public BaseContainerAdministrator<CRunContainer> {
    private:
        friend class Core::SingletonType<CRunContainerAdministrator>;

        CRunContainerAdministrator();

    public:
        CRunContainerAdministrator(const CRunContainerAdministrator&) = delete;
        CRunContainerAdministrator& operator=(const CRunContainerAdministrator&) = delete;

        ~CRunContainerAdministrator() override;

        IContainer* Container(const string& id,
            IStringIterator& searchpaths,
            const string& logpath,
            const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
    };
}
}
