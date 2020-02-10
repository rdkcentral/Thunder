#include "CImplementation.h"
#include <thread>

namespace WPEFramework
{
    
namespace ProcessContainers
{
    // Container administrator
    // ----------------------------------
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

        ContainerError error = process_container_create(&container, id.c_str(), sp, logpath.c_str(), configuration.c_str());

        delete[] sp;

        if (error != ContainerError::ERROR_NONE) {
            TRACE_L1("Failed to create container %s. Error code %d", id.c_str(), error);
            return nullptr;
        } else {

            _containers.push_back(new CContainer(container));
            return _containers.back();
        }
    }

    void CContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        process_container_logging(logPath.c_str(), loggingOptions.c_str());
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
        if (process_container_pid(_container, &result) == ERROR_NONE) {
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

            if (error == ContainerError::ERROR_NONE) {
                cpuInfo.cores.push_back(usage);
            } else if (error == ContainerError::ERROR_OUT_OF_BOUNDS) {
                // Out of bounds, lets finish
                break;
            } else {
                TRACE_L1("Error occurred while getting cpu usage of thread %d. Error code: %d", i, error);
                break;
            }
        }

        return cpuInfo;
    }
    
    std::vector<CContainer::NetworkInterface> CContainer::NetworkInterfaces() const
    {
        std::vector<NetworkInterface> result;

        ProcessContainerNetworkStatus networkStatus;
        if (process_container_network_status_create(_container, &networkStatus) == ERROR_NONE) {

            result.reserve(networkStatus.numInterfaces);

            for (uint32_t i = 0; i < networkStatus.numInterfaces; i++) {
                NetworkInterface interface;
                interface.name = networkStatus.interfaces[i].interfaceName;
                interface.IPs.reserve(networkStatus.interfaces[i].numIp);

                for (uint32_t ipId = 0; ipId < networkStatus.interfaces[i].numIp; ++ipId) {
                    interface.IPs.emplace_back(networkStatus.interfaces[i].ips[ipId]);
                }
            }
        } else {
            TRACE_L1("Failed to get network status for container \"%s\"", Id().c_str());
        }

        return result;
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
        
        ContainerError error = process_container_start(_container, command.c_str(), params);

        delete[] params;

        if (error != ContainerError::ERROR_NONE) {
            TRACE_L1("Failed to start command %s. Error code: %d", command.c_str(), error);
            return false;
        } else {
            return true;
        }
        
    }
    bool CContainer::Stop(const uint32_t timeout /*ms*/)
    {
        if (process_container_stop(_container) != ContainerError::ERROR_NONE) {
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
    };

} // namespace ProcessContainers

} // namespace WPEFramework


