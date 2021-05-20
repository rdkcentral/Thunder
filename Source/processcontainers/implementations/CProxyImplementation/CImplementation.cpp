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

#include "CImplementation.h"
#include <thread>

namespace WPEFramework {

namespace ProcessContainers {
    // NetworkInfo
    // ----------------------------------
    CNetworkInterfaceIterator::CNetworkInterfaceIterator(const ProcessContainer* container)
        : NetworkInterfaceIterator()
    {
        if (process_container_network_status_create(const_cast<ProcessContainer*>(container), &_networkStatus) != PC_ERROR_NONE) {
            TRACE_L1("Failed to get network status for container \"%s\"", container->id);
        }

        _count = _networkStatus.numInterfaces;
    }

    CNetworkInterfaceIterator::~CNetworkInterfaceIterator()
    {
        if (process_container_network_status_destroy(&_networkStatus) != PC_ERROR_NONE) {
            TRACE_L1("Failed to release a network status");
        }
    }

    string CNetworkInterfaceIterator::Name() const
    {
        return _networkStatus.interfaces[_current].interfaceName;
    }

    uint32_t CNetworkInterfaceIterator::NumAddresses() const
    {
        return _networkStatus.interfaces[_current].numIp;
    }

    string CNetworkInterfaceIterator::IP(uint32_t id) const
    {
        ASSERT(id < _networkStatus.interfaces[_current].numIp);

        return _networkStatus.interfaces[_current].ips[id];
    }

    // Container administrator
    // ----------------------------------
    CContainerAdministrator::CContainerAdministrator()
    {
        // make sure framework is initialized
        ContainerError error = process_container_initialize();

        if (error != ContainerError::PC_ERROR_NONE) {
            TRACE_L1("Failed to initialize container api. Error code %d", error);
        } else {
            _refCount = 1;
        }
    }

    CCoontainerAdministrator::~CContainerAdministrator()
    {
        process_container_deinitialize();
    }

    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static CContainerAdministrator& cContainerAdministrator = Core::SingletonType<CContainerAdministrator>::Instance();

        return cContainerAdministrator;
    }

    IContainer* CContainerAdministrator::Container(const string& id, IStringIterator& searchpaths, const string& logpath, const string& configuration)
    {
        IContainer* result = nullptr;
        ProcessContainer* container;
        const char** sp = new const char*[searchpaths.Count() + 1];

        searchpaths.Reset(0);
        for (int i = 0; searchpaths.Next(); i++) {
            sp[i] = searchpaths.Current().c_str();
        }
        sp[searchpaths.Count()] = nullptr;

        _adminLock.Lock();
        ContainerError error = process_container_create(&container, const_cast<char*>(id.c_str()), const_cast<char**>(sp), const_cast<char*>(logpath.c_str()), const_cast<char*>(configuration.c_str()));

        delete[] sp;

        if (error != ContainerError::PC_ERROR_NONE) {
            TRACE_L1("Failed to create container %s. Error code %d", id.c_str(), error);
        } else {
            _containers.push_back(new CContainer(container));

            result = _containers.back();
        }
        _adminLock.Unlock();

        return result;
    }

    void CContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        _adminLock.Lock();
        process_container_logging(const_cast<char*>(logPath.c_str()), const_cast<char*>(loggingOptions.c_str()));
        _adminLock.Unlock();
    }

    CContainerAdministrator::ContainerIterator CContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void CContainerAdministrator::RemoveContainer(IContainer* container)
    {
        _adminLock.Lock();
        _containers.remove(container);

        if (_containers.size() == 0) {
            process_container_deinitialize();
        }
        _adminLock.Unlock();
    }

    // Container
    // ------------------------------------
    CContainer::CContainer(ProcessContainer* container)
        : _container(container)
        , _refCount(1)
    {
    }

    CContainer::~CContainer()
    {
        static_cast<CContainerAdministrator&>(CContainerAdministrator::Instance()).RemoveContainer(this);

        if (_container != nullptr)
            process_container_destroy(_container);
    }

    const string CContainer::Id() const
    {
        return string(_container->id);
    }

    uint32_t CContainer::Pid() const
    {
        uint32_t pid;
        if (process_container_pid(_container, &pid) != PC_ERROR_NONE) {
            pid = 0;
        }

        return pid;
    }

    CContainer::MemoryInfo CContainer::Memory() const
    {
        MemoryInfo result;
        ProcessContainerMemory memory;

        process_container_memory_status(_container, &memory);

        result.allocated = memory.allocated;
        result.resident = memory.resident;
        result.shared = memory.shared;

        return result;
    }

    CContainer::CPUInfo CContainer::Cpu() const
    {
        const unsigned int cores = std::thread::hardware_concurrency();
        CPUInfo cpuInfo;
        ;

        // Get total usage
        process_container_cpu_usage(_container, -1, &(cpuInfo.total));

        cpuInfo.cores.reserve(cores);

        // get per core usage
        for (uint32_t i = 0; i < cores; i++) {
            uint64_t usage;
            ContainerError error = process_container_cpu_usage(_container, i, &usage);

            if (error == ContainerError::PC_ERROR_NONE) {
                cpuInfo.cores.push_back(usage);
            } else if (error == ContainerError::PC_ERROR_OUT_OF_BOUNDS) {
                // Out of bounds, lets finish
                break;
            } else {
                TRACE_L1("Error occurred while getting cpu usage of thread %d. Error code: %d", i, error);
                break;
            }
        }

        return cpuInfo;
    }

    NetworkInterfaceIterator* CContainer::NetworkInterfaces() const
    {
        return new CNetworkInterfaceIterator(_container);
    }

    bool CContainer::IsRunning() const
    {
        return static_cast<bool>(process_container_running(_container));
    }

    bool CContainer::Start(const string& command, IStringIterator& parameters)
    {
        char const** params = new char const*[parameters.Count() + 1];
        bool result = false;

        parameters.Reset(0);
        for (int i = 0; parameters.Next(); i++) {
            params[i] = parameters.Current().c_str();
        }

        params[parameters.Count()] = nullptr;

        ContainerError error = process_container_start(_container, const_cast<char*>(command.c_str()), const_cast<char**>(params));

        delete[] params;

        if (error != ContainerError::PC_ERROR_NONE) {
            TRACE_L1("Failed to start command %s. Error code: %d", command.c_str(), error);
            else
            {
                result = true;
            }

            return result;
        }
        bool CContainer::Stop(const uint32_t timeout /*ms*/)
        {
            return (process_container_stop(_container) == ContainerError::PC_ERROR_NONE);
        }

        void CContainer::AddRef() const
        {
            Core::InterlockedIncrement(_refCount);
        };

        uint32_t CContainer::Release()
        {
            uint32_t result = Core::ERROR_NONE;
            if (Core::InterlockedDecrement(_refCount) == 0) {
                delete this;

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            return result;
        };

    } // namespace ProcessContainers

} // namespace WPEFramework
