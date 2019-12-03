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
        const char** sp = new char*[searchpaths.Count()];

        searchpaths.Reset(0);
        for (int i = 0; searchpaths.Next(); i++) {
            sp[i] = searchpaths.Current().c_str();
        }

        ContainerError error = container_create(&container, id.c_str(), sp, logpath.c_str(), configuration.c_str());

        delete[] sp;

        if (error != ContainerError::ERROR_NONE) {
            TRACE_L1("Failed to create container %s. Error code %d", id.c_str(), error);
            return nullptr;
        } else {
            _containers.push_back(new CContainer(container));
            return _containers.back();
        }
    }

    void CContainerAdministrator::Logging(const string& logpath, const string& logid, const string& loggingoptions)
    {
        container_enableLogging(logpath.c_str(), logid.c_str(), loggingoptions.c_str());
    }

    IContainerAdministrator::ContainerIterator CContainerAdministrator::Containers() 
    {
        return ContainerIterator(_containers);
    }

    // Container
    // ------------------------------------
    CContainer::CContainer(Container_t* container)
        : _container(container)
        , _refCount(1)
    {
        
    }

    CContainer::~CContainer() 
    {
        container_release(_container);
    }

    const string CContainer::Id() const 
    {
        char name[64];
        container_getName(_container, name, 64);

        return string(name);
    }

    pid_t CContainer::Pid() const 
    {
        return 0;
    }

    CContainer::MemoryInfo CContainer::Memory() const
    {
        MemoryInfo result;
        ContainerMemory memory;

        container_getMemory(_container, &memory);
        
        result.allocated = memory.allocated;
        result.resident = memory.resident;
        result.shared = memory.shared;

        return result;
    }

    CContainer::CPUInfo CContainer::Cpu() const
    {
        unsigned int threads = std::thread::hardware_concurrency();
        ContainerError result = ContainerError::ERROR_NONE;
        CPUInfo cpuInfo;;

        // Get total usage
        container_getCpuUsage(_container, -1, &(cpuInfo.total));

        cpuInfo.threads.reserve(threads);

        // get per thread usage
        for (int i = 0; i < threads; i++) {
            uint64_t usage;
            ContainerError error = container_getCpuUsage(_container, i, &usage);

            if (error == ContainerError::ERROR_NONE) {
                cpuInfo.threads.push_back(usage);
            } else if (error == ContainerError::ERROR_OUT_OF_BOUNDS) {
                break;
            } else {
                TRACE_L1("Error occurred while getting cpu usage of thread %d. Error code: %d", i, error);
                break;
            }
        }
    }

    string CContainer::ConfigPath() const
    {
        char configPath[256];

        container_getConfigPath(_container, configPath, 256);

        return string(configPath);
    }

    string CContainer::LogPath() const
    {
        return "";
    }

    IConstStringIterator CContainer::NetworkInterfaces() const
    {
        uint32_t numNetworks;
        container_getNumNetworkInterfaces(_container, &numNetworks);

        std::vector<string> networkInterfaces;
        networkInterfaces.reserve(numNetworks);
        
        for (int i = 0; i < numNetworks; i++) 
        {
            char interfaceName[16];

            ContainerError error = container_getNetworkInterfaceName(_container, i, interfaceName, 16);

            if (error != ContainerError::ERROR_NONE) {
                TRACE_L1("Error occurred while getting network interface %d name. Error code: %d", i, error);
                break;

            } else {
                networkInterfaces.emplace_back(interfaceName);
            }
        }

        return IConstStringIterator(networkInterfaces);
    }

    std::vector<Core::NodeId> CContainer::IPs(const string& interface) const
    {
        std::vector<Core::NodeId> output;
        uint32_t numIPs;

        ContainerError error = container_getNumIPs(_container, interface.c_str(), &numIPs);

        if (error == ContainerError::ERROR_NONE)
        {
            output.reserve(numIPs);

            for (int i = 0; i < numIPs; i++) {
                char ip[256];
                error = container_getIP(_container, interface.c_str(), i, ip, 256);

                if (error != ContainerError::ERROR_NONE) {
                    TRACE_L1("Error occurred while getting ip %d of interface %s. Error code: %d", i, interface.c_str(), error);
                    break;
                } else {
                    output.emplace_back(ip);
                }
            }
        } else {
            TRACE_L1("Could not get number of ips assigned to interface %s. Error code: %d", interface.c_str(), error);
        }

        return output;
    }

    bool CContainer::IsRunning() const
    {
        return static_cast<bool>(container_isRunning(_container));
    }

    bool CContainer::Start(const string& command, IStringIterator& parameters) 
    {
        const char** params = new char*[parameters.Count()];

        parameters.Reset(0);
        for (int i = 0; parameters.Next(); i++) {
            params[i] = parameters.Current().c_str();
        }
        
        ContainerError error = container_start(_container, command.c_str(), params, parameters.Count());

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
        container_stop(_container);
    }

    void CContainer::AddRef() const 
    {
        ++_refCount;
    };

    uint32_t CContainer::Release() const
    {
        --_refCount;

        if (_refCount <= 0) {
            delete this;
        }
    };


} // namespace ProcessContainers

} // namespace WPEFramework


