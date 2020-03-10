#include "RunCImplementation.h"
#include "JSON.h"
#include <thread>

namespace WPEFramework
{

class RunCStatus : public Core::JSON::Container {
private:
    RunCStatus(const RunCStatus&);
    RunCStatus& operator=(const RunCStatus&);

public:
    RunCStatus()
        : Core::JSON::Container()
        , Pid(0)
        , Status()
    {
        Add(_T("pid"), &Pid);
        Add(_T("status"), &Status);
    }
    ~RunCStatus()
    {
    }

public:
    Core::JSON::DecUInt32 Pid;
    Core::JSON::String Status;
};
    
namespace ProcessContainers
{
    // NetworkInterfaceIterator
    // ----------------------------------
    RunCNetworkInterfaceIterator::RunCNetworkInterfaceIterator()
        : NetworkInterfaceIterator()
    {
        TRACE_L1("RunCNetworkInterfaceIterator::RunCNetworkInterfaceIterator() not implemented")
    }

    RunCNetworkInterfaceIterator::~RunCNetworkInterfaceIterator()
    {
        TRACE_L1("RunCNetworkInterfaceIterator::~RunCNetworkInterfaceIterator() not implemented")
    }

    std::string RunCNetworkInterfaceIterator::Name() const 
    {
        TRACE_L1("RunCNetworkInterfaceIterator::Name() not implemented")

        return "";
    }

    uint32_t RunCNetworkInterfaceIterator::NumIPs() const 
    {
        TRACE_L1("RunCNetworkInterfaceIterator::NumIPs() not implemented")

        return 0;
    }

    std::string RunCNetworkInterfaceIterator::IP(uint32_t id) const 
    {
        TRACE_L1("RunCNetworkInterfaceIterator::IP() not implemented")

        return "";
    }

    // Container administrator
    // ----------------------------------
    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static RunCContainerAdministrator& runCContainerAdministrator = Core::SingletonType<RunCContainerAdministrator>::Instance();

        return runCContainerAdministrator;
    }

    IContainer* RunCContainerAdministrator::Container(const string& id, IStringIterator& searchpaths,  const string& logpath, const string& configuration) 
    {
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            Core::File configFile(path + "/" + id + "/config.json");

            if (configFile.Exists()) {
                RunCContainer* container = new RunCContainer(id, path + "/" + id);
                _containers.push_back(container);
                AddRef();

                return container;
            }
        }

        return nullptr;
    }

    RunCContainerAdministrator::RunCContainerAdministrator()
        : _refCount(1)
    {

    }

    RunCContainerAdministrator::~RunCContainerAdministrator()
    {
        if (_containers.size() > 0) {
            TRACE_L1("There are still active containers when shutting down administrator!");
            
            while (_containers.size() > 0) {
                _containers.back()->Release();
                _containers.pop_back();
            }
        }
    }

    void RunCContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        // Only container-scope logging
    }

    RunCContainerAdministrator::ContainerIterator RunCContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void RunCContainerAdministrator::AddRef() const
    {
        _refCount++;
    }

    uint32_t RunCContainerAdministrator::Release()
    {
        --_refCount;

        return (Core::ERROR_NONE);
    }

    void RunCContainerAdministrator::RemoveContainer(IContainer* container)
    {
        _containers.remove(container);
        delete container;

        Release();
    }

    // Container
    // ------------------------------------
    RunCContainer::RunCContainer(string name, string path)
        : _refCount(1)
        , _name(name)        
        , _path(path)
        , _pid()
    {

    }

    RunCContainer::~RunCContainer() 
    {

    }  

    const string RunCContainer::Id() const 
    {
        return _name;
    }

    uint32_t RunCContainer::Pid() const 
    {
        if (_pid.IsSet() == false) {
            Core::Process::Options options("/usr/bin/runc");

            options.Add("state").Add(_name);

            Core::Process process(true);
        
            uint32_t pid;

            if (process.Launch(options, &pid) != Core::ERROR_NONE) {
                TRACE_L1("Failed to create RunC container with name: %s", _name.c_str());

                return 0;
            }

            process.WaitProcessCompleted(Core::infinite);
            
            if (process.ExitCode() != 0) {
                return 0;
            }

            char data[1024];
            process.Output((uint8_t*)data, 2048);
            
            RunCStatus info;
            info.FromString(string(data));

            _pid = info.Pid.Value();
        }
        
        return _pid;
    }

    RunCContainer::MemoryInfo RunCContainer::Memory() const
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

        return result;
    }

    RunCContainer::CPUInfo RunCContainer::Cpu() const
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
    
    NetworkInterfaceIterator* RunCContainer::NetworkInterfaces() const
    {
        return new RunCNetworkInterfaceIterator();
    }

    bool RunCContainer::IsRunning() const
    {
        bool result = false;

        Core::Process::Options options("/usr/bin/runc");
        options.Add("state").Add(_name);

        Core::Process process(true);
        
        uint32_t pid;

        if (process.Launch(options, &pid) != Core::ERROR_NONE) {
            TRACE_L1("Failed to create RunC container with name: %s", _name.c_str());
        } else {
            process.WaitProcessCompleted(Core::infinite);
        
            if (process.ExitCode() == 0) {
                
                char data[1024];
                process.Output((uint8_t*)data, 2048);
                
                RunCStatus info;
                info.FromString(string(data));

                result = info.Status.Value() == "running";       
            }
        }

        return result;
    }

    bool RunCContainer::Start(const string& command, IStringIterator& parameters) 
    {
        Core::JSON::ArrayType<Core::JSON::String> paramsJSON;
        Core::JSON::String tmp;
        bool result = false;

        tmp = command;
        paramsJSON.Add(tmp);

        while (parameters.Next()) {
            tmp = parameters.Current();
            paramsJSON.Add(tmp);
        }

        string paramsFormated;
        paramsJSON.ToString(paramsFormated);

        Core::Process::Options options("/usr/bin/runc");
        options.Add("run").Add("-d").Add("--args").Add(paramsFormated).Add("-b").Add(_path).Add("--no-new-keyring")
            .Add(_name).Add(command);

        Core::Process process(true);
        
        uint32_t pid;

        if (process.Launch(options, &pid) != Core::ERROR_NONE) {
            TRACE_L1("Failed to create RunC container with name: %s", _name.c_str());
        } else {
            process.WaitProcessCompleted(Core::infinite);

            if (process.ExitCode() != 0) {
                // Force kill
                Stop(Core::infinite);
            } else {
                result = true;
            }
        }

        return result;
    }

    bool RunCContainer::Stop(const uint32_t timeout /*ms*/)
    {
        bool result = false;

        Core::Process::Options options("/usr/bin/runc");
        options.Add("delete").Add(_name);

        Core::Process process(true);
        
        uint32_t pid;

        if (process.Launch(options, &pid) != Core::ERROR_NONE) {
            TRACE_L1("Failed to send a stop request to RunC container named: %s", _name.c_str());
        } else {
            // Prepare for eventual force kill
            options.Clear();
            options.Add("delete").Add("-f").Add(_name);

            uint32_t deleteSuccess = process.WaitProcessCompleted(timeout);

            if (deleteSuccess != Core::ERROR_NONE || process.ExitCode() != 0) {
                if (process.Launch(options, &pid) != Core::ERROR_NONE) {
                    TRACE_L1("Failed to send a forced kill request to RunC container named: %s", _name.c_str());
                } else {
                    process.WaitProcessCompleted(timeout);

                    result = (process.ExitCode() == 0);
                }

            } else {
                result = true;
            }
        }

        return result;
    }

    void RunCContainer::AddRef() const 
    {
        _refCount++;
    };

    uint32_t RunCContainer::Release()
    {
        if (--_refCount == 0) {
            static_cast<RunCContainerAdministrator&>(RunCContainerAdministrator::Instance()).RemoveContainer(this);
        }

        return Core::ERROR_NONE;
    };

} // namespace ProcessContainers

} // namespace WPEFramework


