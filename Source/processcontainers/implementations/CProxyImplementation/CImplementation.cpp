#include "CImplementation.h"
#include <thread>

namespace WPEFramework
{
    
namespace ProcessContainers
{
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

    uint32_t CNetworkInterfaceIterator::NumIPs() const
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

    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static CContainerAdministrator& cContainerAdministrator = Core::SingletonType<CContainerAdministrator>::Instance();

        return cContainerAdministrator;
    }

    IContainer* CContainerAdministrator::Container(const string& id, IStringIterator& searchpaths,  const string& logpath, const string& configuration) 
    {
        ProcessContainer* container;
        const char** sp = new const char*[searchpaths.Count() + 1];

        searchpaths.Reset(0);
        for (int i = 0; searchpaths.Next(); i++) {
            sp[i] = searchpaths.Current().c_str();
        }
        sp[searchpaths.Count()] = nullptr;

        ContainerError error = process_container_create(&container, const_cast<char*>(id.c_str())
            , const_cast<char**>(sp), const_cast<char*>(logpath.c_str()), const_cast<char*>(configuration.c_str()));

        delete[] sp;

        if (error != ContainerError::PC_ERROR_NONE) {
            TRACE_L1("Failed to create container %s. Error code %d", id.c_str(), error);
            return nullptr;
        } else {
            _containers.push_back(new CContainer(container));

            return _containers.back();
        }
    }

    void CContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        process_container_logging(const_cast<char*>(logPath.c_str()), const_cast<char*>(loggingOptions.c_str()));
    }

    CContainerAdministrator::ContainerIterator CContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void CContainerAdministrator::AddRef() const
    {
        _refCount++;
    }

    uint32_t CContainerAdministrator::Release()
    {
        if (--_refCount == 0) {
            process_container_deinitialize();
        }

        return (Core::ERROR_NONE);
    }

    void CContainerAdministrator::RemoveContainer(IContainer* container)
    {
        _containers.remove(container);
        
        if (_containers.size() == 0) {
            process_container_deinitialize();
        }
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
        uint32_t result;
        if (process_container_pid(_container, &result) == PC_ERROR_NONE) {
            return result;
        } else {
            return 0;
        }
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
        CPUInfo cpuInfo;;

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

        parameters.Reset(0);
        for (int i = 0; parameters.Next(); i++) {
            params[i] = parameters.Current().c_str();
        }

        params[parameters.Count()] = nullptr;
        
        ContainerError error = process_container_start(_container, const_cast<char*>(command.c_str()), const_cast<char**>(params));

        delete[] params;

        if (error != ContainerError::PC_ERROR_NONE) {
            TRACE_L1("Failed to start command %s. Error code: %d", command.c_str(), error);
            return false;
        } else {
            return true;
        }
        
    }
    bool CContainer::Stop(const uint32_t timeout /*ms*/)
    {
        if (process_container_stop(_container) != ContainerError::PC_ERROR_NONE) {
            return false;
        } else {
            return true;
        }
    }

    void CContainer::AddRef() const 
    {
        ++_refCount;
    };

    uint32_t CContainer::Release()
    {
        --_refCount;

        if (_refCount == 0) {
            delete this;
        }

        return Core::ERROR_DESTRUCTION_SUCCEEDED;
    };

} // namespace ProcessContainers

} // namespace WPEFramework


