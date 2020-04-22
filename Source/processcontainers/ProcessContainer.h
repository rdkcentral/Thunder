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

#include "Module.h"

namespace WPEFramework {
namespace ProcessContainers {
    
    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;
    using IConstStringIterator = Core::IteratorType<const std::vector<string>, const string, std::vector<string>::const_iterator>;

    struct NetworkInterfaceIterator {
        NetworkInterfaceIterator();
        virtual ~NetworkInterfaceIterator() {};
        
        // Common iterator functions:
        // -----------------------------------
        virtual bool Next();
        virtual void Reset();
        virtual bool IsValid() const;
        virtual uint32_t Count() const;

        // Lifetime management:
        // ------------------------------------
        void AddRef();
        void Release();
        
        // Implementation-dependent
        // -----------------------------------

        // Return interface name (eg. veth0)
        virtual std::string Name() const = 0;

        // Return numver of ip addresses assigned to the interface
        virtual uint32_t NumIPs() const = 0;

        // Get n-th ip assigned to the container. Empty string
        // is returned if n is larger than NumIPs()
        virtual std::string IP(uint32_t n) const = 0;
    protected:
        uint32_t _current;
        uint32_t _count;

    private:
        uint32_t _refCount;
    };

    struct IContainer {
        struct MemoryInfo {
            uint64_t allocated; // in bytes
            uint64_t resident; // in bytes
            uint64_t shared; // in bytes
        };

        struct CPUInfo {
            uint64_t total; // total usage of cpu, in nanoseconds;
            std::vector<uint64_t> cores; // cpu usage per core in nanoseconds;
        };

        IContainer() = default;
        virtual ~IContainer() = default;

        // Return the Name of the Container
        virtual const string Id() const = 0;

        // Get PID of the container in Host namespace
        virtual uint32_t Pid() const = 0;

        // Return memory usage statistics for the whole container
        virtual MemoryInfo Memory() const = 0;

        // Return time of CPU spent in whole container
        virtual CPUInfo Cpu() const = 0;

        // Return information on network status of the container
        virtual NetworkInterfaceIterator* NetworkInterfaces() const = 0;

        // Tells if the container is running or not
        virtual bool IsRunning() const = 0;

        // Start the container with provided process. Returns true if process is started successfully
        virtual bool Start(const string& command, IStringIterator& parameters) = 0; 

        // Stops the running containerized process. Returns true when stopped.
        // Note: if timeout == 0, call is asynchronous
        virtual bool Stop(const uint32_t timeout /*ms*/) = 0; 

        virtual void AddRef() const = 0;
        virtual uint32_t Release() = 0;
    };

    struct IContainerAdministrator {
        static IContainerAdministrator& Instance();

        IContainerAdministrator() = default;
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
                                      const string& configuration) = 0; //searchpaths will be searched in order in which they are iterated

        // Enable logging for the container framework
        virtual void Logging(const string& globalLogPath, const string& loggingoptions) = 0;

        // Returns ids of all created containers
        virtual std::vector<string> Containers() = 0;
        
        // Return a container by its ID. Returns nullptr if container is not found
        // It needs to be released.
        virtual IContainer* Get(const string& id) = 0;
    };
} // ProcessContainers
} // WPEFramework

