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

namespace Thunder {
namespace ProcessContainers {

    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;

    struct INetworkInterfaceIterator : public Core::IIterator,  public Core::IReferenceCounted  {

        ~INetworkInterfaceIterator() override = default;

        // Return interface name (eg. eth0)
        virtual string Name() const = 0;

        // Return numver of IP addresses assigned to the interface
        virtual uint16_t NumAddresses() const = 0;

        // Get n-th IP assigned to the container. Empty string is returned if n is larger than NumAddresses()
        virtual string Address(const uint16_t n = 0) const = 0;
    };

    struct IMemoryInfo : public Core::IReferenceCounted {

        ~IMemoryInfo() override = default;

        // Memory size in bytes, returns UINT64_MAX on error
        virtual uint64_t Allocated() const = 0;
        virtual uint64_t Resident() const = 0;
        virtual uint64_t Shared() const = 0;
    };

    struct IProcessorInfo : public Core::IReferenceCounted {

        ~IProcessorInfo() override = default;

        // Total usage of CPU, in nanoseconds. Returns UINT64_MAX on error
        virtual uint64_t TotalUsage() const = 0;

        // CPU usage per core in nanoseconds. Returns UINT64_MAX on error
        virtual uint64_t CoreUsage(uint32_t coreNum) const = 0;

        virtual uint16_t NumberOfCores() const = 0;
    };

    struct IContainerIterator : public Core::IIterator, public Core::IReferenceCounted {

        ~IContainerIterator() override = default;

        // Return Id of container
        virtual const string& Id() const = 0;
    };

    struct IContainer {

        enum containertype : uint8_t {
            DEFAULT,
            LXC,
            RUNC,
            CRUN,
            DOBBY,
            AWC
        };

        virtual ~IContainer() = default;

        virtual containertype Type() const = 0;

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

        // Start the container with provided process
        virtual bool Start(const string& command, IStringIterator& parameters) = 0;

        // Stops the running containerized process
        virtual bool Stop(const uint32_t timeout /*ms*/) = 0;
    };

    struct IContainerProducer {

        virtual ~IContainerProducer() = default;

        // Initializes the producer
        virtual uint32_t Initialize(const string& /* configuration */) = 0;

        // Shuts down the producer
        virtual void Deinitialize()= 0;

        // Creates a new container
        virtual Core::ProxyType<IContainer> Container(const string& id, IStringIterator& searchPaths, const string& logPath, const string& configuration) = 0;
    };

} // ProcessContainers
}
