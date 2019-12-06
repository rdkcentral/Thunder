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
        Container_t* container;
        const char** sp = new const char*[searchpaths.Count()];

        searchpaths.Reset(0);
        for (int i = 0; searchpaths.Next(); i++) {
            sp[i] = searchpaths.Current().c_str();
        }

        ContainerError error = pcontainer_create(&container, id.c_str(), sp, logpath.c_str(), configuration.c_str());

        delete[] sp;

        if (error != ContainerError::ERROR_NONE) {
            TRACE_L1("Failed to create container %s. Error code %d", id.c_str(), error);
            return nullptr;
        } else {
            // make sure framework is initialized
            if (_containers.size() == 0) {
                error = pcontainer_initialize();

                if (error != ContainerError::ERROR_NONE) {
                    TRACE_L1("Failed to initialize container api. Error code %d", error);
                }
            }

            _containers.push_back(new CContainer(container));
            return _containers.back();
        }
    }

    void CContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        pcontainer_logging(logPath.c_str(), loggingOptions.c_str());
    }

    CContainerAdministrator::ContainerIterator CContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void CContainerAdministrator::AddRef() const
    {

    }

    uint32_t CContainerAdministrator::Release()
    {
        return (Core::ERROR_NONE);
    }

    void CContainerAdministrator::RemoveContainer(IContainer* container)
    {
        _containers.remove(container);
        
        if (_containers.size() == 0) {
            pcontainer_deinitialize();
        }
    }

    // Container
    // ------------------------------------
    CContainer::CContainer(Container_t* container)
        : _container(container)
        , _refCount(1)
    {
        _GetNetworkInterfaces();
    }

    CContainer::~CContainer() 
    {
        static_cast<CContainerAdministrator&>(CContainerAdministrator::Instance()).RemoveContainer(this);

        if (_container != nullptr) 
            pcontainer_release(_container);
    }

    const string CContainer::Id() const 
    {
        string result;
        char name[64];
        uint32_t nameLength;
        uint32_t error = pcontainer_getName(_container, name, &nameLength, sizeof(name));

        if (error == Core::ERROR_NONE) {
            result = name;
        } else if (error == ContainerError::ERROR_MORE_DATA_AVAILBALE) {
            char* nameHeap = new char[nameLength+1];
            pcontainer_getName(_container, nameHeap, nullptr, nameLength+1);

            result = nameHeap;
            delete nameHeap;
        } else {
            result = "";
            TRACE_L1("Failed to get name of the container");
        }

        return result;
    }

    pid_t CContainer::Pid() const 
    {
        // TODO: Implement

        return 0;
    }

    CContainer::MemoryInfo CContainer::Memory() const
    {
        MemoryInfo result;
        ContainerMemory memory;

        pcontainer_getMemory(_container, &memory);
        
        result.allocated = memory.allocated;
        result.resident = memory.resident;
        result.shared = memory.shared;

        return result;
    }

    CContainer::CPUInfo CContainer::Cpu() const
    {
        unsigned int cores = std::thread::hardware_concurrency();
        CPUInfo cpuInfo;;

        // Get total usage
        pcontainer_getCpuUsage(_container, -1, &(cpuInfo.total));

        cpuInfo.cores.reserve(cores);

        // get per thread usage
        for (uint32_t i = 0; i < cores; i++) {
            uint64_t usage;
            ContainerError error = pcontainer_getCpuUsage(_container, i, &usage);

            if (error == ContainerError::ERROR_NONE) {
                cpuInfo.cores.push_back(usage);
            } else if (error == ContainerError::ERROR_OUT_OF_BOUNDS) {
                // INTENTIONALLY LEFT WITHOUT WARNING.
                // In some rare situations it could be hit in non-error way

                break;
            } else {
                TRACE_L1("Error occurred while getting cpu usage of thread %d. Error code: %d", i, error);
                break;
            }
        }

        return cpuInfo;
    }
    
    IConstStringIterator CContainer::NetworkInterfaces() const
    {
        return IConstStringIterator(_networkInterfaces);
    }

    std::vector<string> CContainer::IPs(const string& interface) const
    {
        std::vector<string> output;
        uint32_t numIPs;

        ContainerError error = pcontainer_getNumIPs(_container, interface.c_str(), &numIPs);

        if (error == ContainerError::ERROR_NONE)
        {
            output.reserve(numIPs);

            for (uint32_t i = 0; i < numIPs; i++) {
                char ip[48];
                uint32_t addressLength;
                error = pcontainer_getIP(_container, interface.c_str(), i, ip, &addressLength, sizeof(ip));

                if (error == ContainerError::ERROR_NONE) {
                    output.push_back(ip);
                } else if (error == ContainerError::ERROR_MORE_DATA_AVAILBALE) {
                    
                } else {
                    TRACE_L1("Error occurred while getting ip %d of interface %s. Error code: %d", i, interface.c_str(), error);
                }
            }

        } else {
            TRACE_L1("Could not get number of ips assigned to interface %s. Error code: %d", interface.c_str(), error);
        }

        return output;
    }

    bool CContainer::IsRunning() const
    {
        return static_cast<bool>(pcontainer_isRunning(_container));
    }

    bool CContainer::Start(const string& command, IStringIterator& parameters) 
    {
        char const** params = new char const*[parameters.Count()];

        parameters.Reset(0);
        for (int i = 0; parameters.Next(); i++) {
            params[i] = parameters.Current().c_str();
        }
        
        ContainerError error = pcontainer_start(_container, command.c_str(), params, parameters.Count());

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
        if (pcontainer_stop(_container) != ContainerError::ERROR_NONE) {
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

    void CContainer::_GetNetworkInterfaces()
    {
        uint32_t numNetworks;
        pcontainer_getNumNetworkInterfaces(_container, &numNetworks);

        _networkInterfaces.reserve(numNetworks);
        
        for (uint32_t i = 0; i < numNetworks; i++) 
        {
            char interfaceName[16]; // 16 bytes is usual max length of interface name, because of DHCP limitations

            uint32_t nameLength;
            ContainerError error = pcontainer_getNetworkInterfaceName(_container, i, interfaceName, &nameLength, sizeof(interfaceName));

            if (error == ContainerError::ERROR_NONE) {
                _networkInterfaces.emplace_back(interfaceName);
;
            } else if (error == ContainerError::ERROR_MORE_DATA_AVAILBALE) {
                char* nameHeap = new char[nameLength+1];
                pcontainer_getName(_container, nameHeap, nullptr, nameLength+1);

                _networkInterfaces.emplace_back(nameHeap);
                delete nameHeap;
            } else if (error == ContainerError::ERROR_OUT_OF_BOUNDS) {
                break;
            } else {
                TRACE_L1("Failed to get interface name of the container");
            } 
        }
    }


} // namespace ProcessContainers

} // namespace WPEFramework


