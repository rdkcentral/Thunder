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

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {
    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;

    class INetworkInterfaceIterator : public Core::IIterator, public Core::IReferenceCounted {
    public:
        ~INetworkInterfaceIterator() override = default;

        // Return interface name (eg. veth0)
        virtual string Name() const = 0;

        // Return numver of ip addresses assigned to the interface
        virtual uint16_t NumAddresses() const = 0;

        // Get n-th ip assigned to the container. Empty string
        // is returned if n is larger than NumAddresses()
        virtual string Address(const uint16_t n = 0) const = 0;
    };

    class IMemoryInfo : public Core::IReferenceCounted {
    public:
        ~IMemoryInfo() override = default;

        virtual uint64_t Allocated() const = 0; // in bytes, returns UINT64_MAX on error
        virtual uint64_t Resident() const = 0; // in bytes, returns UINT64_MAX on error
        virtual uint64_t Shared() const = 0; // in bytes, returns UINT64_MAX on error
    };

    class IProcessorInfo : public Core::IReferenceCounted {
    public:
        ~IProcessorInfo() override = default;

        // total usage of cpu, in nanoseconds. Returns UINT64_MAX on error
        virtual uint64_t TotalUsage() const = 0;

        // cpu usage per core in nanoseconds. Returns UINT64_MAX on error
        virtual uint64_t CoreUsage(uint32_t coreNum) const = 0;

        // return number of cores
        virtual uint16_t NumberOfCores() const = 0;
    };

    class IContainerIterator : public Core::IIterator, public Core::IReferenceCounted {
    public:
        ~IContainerIterator() override = default;

        // Return Id of container
        virtual const string& Id() const = 0;
    };

    class IContainer : public Core::IReferenceCounted {
    public:
        ~IContainer() override = default;

        // Return the Name of the Container
        virtual const string& Id() const = 0;

        // Get PID of the container in Host namespace
        virtual uint32_t Pid() const = 0;

        // Return memory usage statistics for the whole container
        virtual IMemoryInfo* Memory() const = 0;

        // Return time of CPU spent in whole container
        virtual IProcessorInfo* ProcessorInfo() const = 0;

        // Return information on network status of the container
        virtual INetworkInterfaceIterator* NetworkInterfaces() const = 0;

        // Tells if the container is running or not
        virtual bool IsRunning() const = 0;

        // Start the container with provided process. Returns true if process is started successfully
        virtual bool Start(const string& command, IStringIterator& parameters) = 0;

        // Stops the running containerized process. Returns true when stopped.
        // Note: if timeout == 0, call is asynchronous
        virtual bool Stop(const uint32_t timeout /*ms*/) = 0;
    };

    struct EXTERNAL IContainerAdministrator {
        static IContainerAdministrator& Instance();
        virtual ~IContainerAdministrator() = default;

        /**
            Creates new Container instance

            \param id Name of the created created container.
            \param searchpaths List of locations, which will be checked (in order from first to last) 
                               for presence of the container. Eg. when id="ContainerName", then it will
                               search for <searchpath>/Container.
            \param containerLogPath Path to the folder, where logfile for created container will be created
            \param configuration Implementation-specific configuration provided to the container.
        */
        virtual IContainer* Container(const string& id,
            IStringIterator& searchpaths,
            const string& containerLogPath,
            const string& configuration)
            = 0; //searchpaths will be searched in order in which they are iterated

        // Enable logging for the container framework
        virtual void Logging(const string& globalLogPath, const string& loggingoptions) = 0;

        // Returns ids of all created containers
        virtual IContainerIterator* Containers() const = 0;

        // Return a container by its ID. Returns nullptr if container is not found
        // It needs to be released.
        virtual IContainer* Get(const string& id) = 0;
    };
} // ProcessContainers
} // WPEFramework
