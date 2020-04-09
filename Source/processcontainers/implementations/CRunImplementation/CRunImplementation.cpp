#include "CRunImplementation.h"
#include "JSON.h"
#include <thread>

namespace WPEFramework
{    
namespace ProcessContainers
{
    // NetworkInterfaceIterator
    // ----------------------------------
    CRunNetworkInterfaceIterator::CRunNetworkInterfaceIterator()
        : NetworkInterfaceIterator()
    {
        TRACE_L1("CRunNetworkInterfaceIterator::CRunNetworkInterfaceIterator() not implemented")
    }

    CRunNetworkInterfaceIterator::~CRunNetworkInterfaceIterator()
    {
        TRACE_L1("CRunNetworkInterfaceIterator::~CRunNetworkInterfaceIterator() not implemented")
    }

    std::string CRunNetworkInterfaceIterator::Name() const 
    {
        TRACE_L1("CRunNetworkInterfaceIterator::Name() not implemented")

        return "";
    }

    uint32_t CRunNetworkInterfaceIterator::NumIPs() const 
    {
        TRACE_L1("CRunNetworkInterfaceIterator::NumIPs() not implemented")

        return 0;
    }

    std::string CRunNetworkInterfaceIterator::IP(uint32_t id) const 
    {
        TRACE_L1("CRunNetworkInterfaceIterator::IP() not implemented")

        return "";
    }

    // Container administrator
    // ----------------------------------
    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static CRunContainerAdministrator& runCContainerAdministrator = Core::SingletonType<CRunContainerAdministrator>::Instance();

        return runCContainerAdministrator;
    }

    IContainer* CRunContainerAdministrator::Container(const string& id, IStringIterator& searchpaths,  const string& logpath, const string& configuration) 
    {
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            Core::File configFile(path + "/Container/config.json");

            if (configFile.Exists()) {
                CRunContainer* container = new CRunContainer(id, path + "/Container", logpath);
                _containers.push_back(container);
                AddRef();

                return container;
            }
        }

        return nullptr;
    }

    CRunContainerAdministrator::CRunContainerAdministrator()
        : _refCount(1)
    {

    }

    CRunContainerAdministrator::~CRunContainerAdministrator()
    {
        if (_containers.size() > 0) {
            TRACE_L1("There are still active containers when shutting down administrator!");
            
            while (_containers.size() > 0) {
                _containers.back()->Release();
                _containers.pop_back();
            }
        }
    }

    void CRunContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        // Only container-scope logging
    }

    CRunContainerAdministrator::ContainerIterator CRunContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void CRunContainerAdministrator::AddRef() const
    {
        _refCount++;
    }

    uint32_t CRunContainerAdministrator::Release()
    {
        --_refCount;

        return (Core::ERROR_NONE);
    }
    
    // Container
    // ------------------------------------
    CRunContainer::CRunContainer(string name, string path, string logPath)
        : _refCount(1)
        , _created(false)
        , _name(name)        
        , _bundle(path)
        , _configFile(path + "/config.json")
        , _logPath(logPath)
        , _container(nullptr)
        , _context()
        , _pid()
    {
        // create a context
        _context.bundle = _bundle.c_str();
        _context.console_socket = nullptr;
        _context.detach = true;
        _context.fifo_exec_wait_fd = -1;
        _context.force_no_cgroup = false;
        _context.id = _name.c_str();
        _context.no_new_keyring = true;
        _context.no_pivot = false;
        _context.no_subreaper = true;
        _context.notify_socket = nullptr;
        _context.output_handler = nullptr;
        _context.output_handler_arg = nullptr;
        _context.pid_file = nullptr;
        _context.preserve_fds = 0;
        _context.state_root = "/run/crun";
        _context.systemd_cgroup = 0;

        return;
        if (logPath.empty() == false) { 
            Core::Directory(logPath.c_str()).CreatePath();

            libcrun_error_t error = nullptr;
            int ret = init_logging (&_context.output_handler, &_context.output_handler_arg, _name.c_str(), (logPath + "/container.log").c_str(), &error);
            if (ret != 0) {
                TRACE_L1("Cannot initialize logging of container \"%s\" in directory %s\n. Error %d: %s", _name.c_str(), logPath.c_str(), error->status, error->msg);
            }
        }
    }

    CRunContainer::~CRunContainer() 
    {

    }  

    const string CRunContainer::Id() const 
    {
        return _name;
    }

    uint32_t CRunContainer::Pid() const 
    {
        uint32_t result = 0;
        libcrun_error_t error = nullptr;
        libcrun_container_status_t status;
        status.pid = 0;

        result = libcrun_read_container_status (&status, _context.state_root, _name.c_str(), &error);
        if (result != 0) {
            TRACE_L1("Failed to get PID of container %s", _name.c_str());
        }

        return status.pid;
    }

    CRunContainer::MemoryInfo CRunContainer::Memory() const
    {
      MemoryInfo result {UINT64_MAX, UINT64_MAX, UINT64_MAX};

        // Load total allocated memory
        string _memoryInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.usage_in_bytes";      

        char buffer[2048];
        auto fd = open(_memoryInfoPath.c_str(), O_RDONLY);

        if (fd != 0) {
            size_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                result.allocated = std::stoll(buffer);
            }
            
            close(fd);
        } else {
            TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
        }

        // Load details about memory
        string memoryFullInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.stat";

        fd = open(memoryFullInfoPath.c_str(), O_RDONLY);
        if (fd != 0) {
            size_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                char* tmp;
                char* token = strtok_r(buffer, " \n", &tmp);

                while (token != nullptr) {
                    if (token == nullptr) 
                        break;

                    char* label = token;

                    token = strtok_r(NULL, " \n", &tmp);
                    if (token == nullptr) 
                        break;

                    uint64_t value = std::stoll(token);

                    if (strcmp(label, "rss") == 0) 
                        result.resident = value;
                    else if (strcmp(label, "mapped_file") == 0) 
                        result.shared = value;

                    token = strtok_r(NULL, " \n", &tmp);
                }
            }
            
            close(fd);
        } else {
            TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
        }

        return result;    }

    CRunContainer::CPUInfo CRunContainer::Cpu() const
    {
        CPUInfo output {UINT64_MAX, std::vector<uint64_t>()};

        // Load total cpu time
        string cpuUsagePath = "/sys/fs/cgroup/cpuacct/" + _name + "/cpuacct.usage";

        char buffer[2048];
        auto fd = open(cpuUsagePath.c_str(), O_RDONLY);

        if (fd != 0) {
            uint32_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                output.total = atoi((char*)buffer);
            }            

            close(fd);
        }

        // Load per-core cpu time
        string cpuPerCoreUsagePath = "/sys/fs/cgroup/cpuacct/" + _name + "/cpuacct.usage_percpu";
        fd = open(cpuPerCoreUsagePath.c_str(), O_RDONLY);

        if (fd != 0) {
            uint32_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                char* tmp;
                char* token = strtok_r((char*)buffer, " \n", &tmp);

                while (token != nullptr) {
                    // Sometimes (but not always for some reason?) a nonprintable character is caught as a separate token.
                    if (isdigit(token[0])) {
                        output.cores.push_back(atoi(token));
                    }
                    token = strtok_r(NULL, " \n", &tmp);
                }
            }

            close(fd);
        }

        return output;
    }
    
    NetworkInterfaceIterator* CRunContainer::NetworkInterfaces() const
    {
        return new CRunNetworkInterfaceIterator();
    }

    bool CRunContainer::IsRunning() const
    {
        bool result = true;
        libcrun_error_t error;
        libcrun_container_status_t status;

        if (libcrun_read_container_status (&status, _context.state_root, _name.c_str(), &error) != 0) {
            // This most likely occurs when container is not found
            // TODO: Find another way to make sure that this is not result of other cause
            result = false;
        } else {
            int ret = libcrun_is_container_running(&status, &error);
            if (ret < 0) {
                TRACE_L1("Failed to acquire container \"%s\" state string", _name.c_str());
                result = false;
            } else {
                result = (ret == 1);
            }

            libcrun_free_container_status(&status);
        }

        return result;
    }

    bool CRunContainer::Start(const string& command, IStringIterator& parameters) 
    {
        libcrun_error_t error = nullptr;
        int ret = 0;
        bool result = true;

        // Make sure no leftover container instances are present
        if (ClearLeftovers() != Core::ERROR_NONE) {
            result = false;
        }
        else {
            _container = libcrun_container_load_from_file (_configFile.c_str(), &error);

            // Add bundle prefix to rootfs location if relative path is provided
            // TODO: Possibly change mount relative path to acomodate for bundle or check if doing chdir() is safe alternative
            string rootfsPath = _bundle + "/";
            if (_container->container_def->root->path != nullptr && _container->container_def->root->path[0] != '/') {
                rootfsPath += _container->container_def->root->path;
                _container->container_def->root->path = 
                    reinterpret_cast<char*>(realloc (_container->container_def->root->path, (rootfsPath.length() + 1) * sizeof(char)));
                
                strcpy(_container->container_def->root->path, rootfsPath.c_str());
            }
            
            if (_container == NULL) {

                TRACE_L1("Failed to load a configuration file in %s. Error %d: %s", _configFile.c_str(), error->status, error->msg);
                result = false;
            } else {

                OverwriteContainerArgs(_container, command, parameters);

                ret = libcrun_container_run (&_context, _container, LIBCRUN_RUN_OPTIONS_PREFORK, &error);
                if (ret != 0) {
                    TRACE_L1("Failed to run a container \"%s\". Error %d: %s", _name.c_str(), error->status, error->msg);
                    result = false;
                } else {
                    _created = true;
                }
            }
        }

        return result;
    }

    bool CRunContainer::Stop(const uint32_t timeout /*ms*/)
    {
        bool result = true;
        libcrun_error_t error = nullptr;

        if (libcrun_container_delete (&_context, NULL, _name.c_str(), true, &error) != 0) {
            TRACE_L1("Failed to stop a container \"%s\". Error: %s", _name.c_str(), error->msg);
            result = false;
        }
        _created = false;

        return result;
    }

    void CRunContainer::AddRef() const 
    {
        _refCount++;
    };

    uint32_t CRunContainer::Release()
    {
        if (--_refCount == 0) {
            if (_created == true) {
                Stop(Core::infinite);
            }        
        }

        return Core::ERROR_NONE;
    };

    void CRunContainer::OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters)
    {
        // Clear args that were set by runtime
        if (container->container_def->process->args) {
            for (size_t i = 0; i < container->container_def->process->args_len; i++)
            {
                if (container->container_def->process->args[i] != NULL)
                {
                    free (container->container_def->process->args[i]);
                }
            }
            free (container->container_def->process->args);
        }

        char** argv = reinterpret_cast<char**>(malloc((newParameters.Count() + 2) * sizeof(char*)));
        int argc = 0;

        // Set custom starting command
        argv[argc] = reinterpret_cast<char*>(malloc(sizeof(char) * newComand.length() + 1));
        strcpy(argv[argc], newComand.c_str());
        argc++;

        // Set command arguments
        while (newParameters.Next())
        {
            argv[argc] = reinterpret_cast<char*>(malloc(sizeof(char) * newParameters.Current().length() + 1));
            strcpy(argv[argc], newParameters.Current().c_str());
            argc++;
        }
        argv[argc] = nullptr;
        container->container_def->process->args = argv;
        container->container_def->process->args_len = argc;
    }

    uint32_t CRunContainer::ClearLeftovers() 
    {
        libcrun_error_t error = nullptr;
        int ret = 0;

        // Find containers 
        libcrun_container_list_t *list;
        ret = libcrun_get_containers_list (&list, "/run/crun", &error);
        if (ret < 0) {
            TRACE_L1("Failed to get containers list. Error %d: %s", error->status, error->msg);
            return Core::ERROR_UNAVAILABLE;
        }

        for (libcrun_container_list_t* it = list; it != nullptr; it = it->next)
        {
            if (_name == it->name) {
                TRACE_L1("Found container %s already created (maybe leftover from another Thunder launch). Killing it!", _name.c_str());
                ret = libcrun_container_delete (&_context, nullptr, it->name, true, &error);

                if (ret < 0) {
                    TRACE_L1("Failed to destroy a container %s. Error %d: %s", _name.c_str(), error->status, error->msg);
                    return Core::ERROR_UNKNOWN_KEY;
                } 
                // its only possible to find one leftover. No sense looking for more...
                break;
            }
        }

        return Core::ERROR_NONE;
    }

} // namespace ProcessContainers

} // namespace WPEFramework


