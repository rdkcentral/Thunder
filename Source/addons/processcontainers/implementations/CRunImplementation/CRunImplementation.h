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

#include "processcontainers/IProcessContainers.h"
#include "processcontainers/ContainerAdministrator.h"
#include "processcontainers/common/CGroupContainerInfo.h"


PUSH_WARNING(DISABLE_WARNING_PEDANTIC)

extern "C" {
#include <crun/container.h>
#include <crun/error.h>
#include <crun/status.h>
#include <crun/utils.h>
}

POP_WARNING()

namespace Thunder {
namespace ProcessContainers {

    // PUBLIC_INTERFACE
    /** Libcrun container managed by ContainerAdministrator's proxy list. */
    class CRunContainer : public IContainer {
    public:
        // PUBLIC_INTERFACE
        /** Bind a container ID to its OCI bundle and optional logging directory. */
        CRunContainer(const string& name, const string& path, const string& logPath);
        CRunContainer(const CRunContainer&) = delete;
        CRunContainer& operator=(const CRunContainer&) = delete;

    public:
        ~CRunContainer() override;

        // IContainerMethods
        // PUBLIC_INTERFACE
        /** Return the producer type used by the current administrator. */
        containertype Type() const override { return IContainer::CRUN; }
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
        bool OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters);

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

    // PUBLIC_INTERFACE
    /** Producer registered with the shared ContainerAdministrator. */
    class CRunContainerAdministrator : public IContainerProducer {
    public:
        // PUBLIC_INTERFACE
        /** Construct the stateless libcrun producer. */
        CRunContainerAdministrator() = default;
        CRunContainerAdministrator(const CRunContainerAdministrator&) = delete;
        CRunContainerAdministrator& operator=(const CRunContainerAdministrator&) = delete;

        ~CRunContainerAdministrator() override = default;

        // PUBLIC_INTERFACE
        /** Search OCI bundles and return an administrator-owned container proxy. */
        Core::ProxyType<IContainer> Container(const string& id,
            IStringIterator& searchpaths,
            const string& logpath,
            const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        // PUBLIC_INTERFACE
        /** Accept producer configuration; logging is configured per container. */
        uint32_t Initialize(const string& configuration) override;
        // PUBLIC_INTERFACE
        /** Release producer resources (none are retained by this producer). */
        void Deinitialize() override { }
    };
}
}
