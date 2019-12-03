#include "LXCImplementation.h"

namespace WPEFramework {
namespace ProcessContainers {

    LXCContainer::LXCContainer(const string& name, LxcContainerType* lxccontainer, const string& logpath, const string& configuration, const string& lxcpath)
        : _name(name)
        , _pid(0)
        , _lxcpath(lxcpath)
        , _logpath(logpath)
        , _referenceCount(1)
        , _lxccontainer(lxccontainer)
        #ifdef __DEBUG__
            , _attach(false)
        #endif
    {
        Config config;
        Core::OptionalType<Core::JSON::Error> error;
        config.FromString(configuration, error);
        if (error.IsSet() == true) {
            SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
        }

        #ifdef __DEBUG__
            _attach = config.Attach.Value();
        #endif
        string pathName;
        Core::SystemInfo::GetEnvironment("PATH", pathName);
        string key("PATH");
        key += "=";
        key += pathName;
        _lxccontainer->set_config_item(_lxccontainer, "lxc.environment", key.c_str());

        if( config.ConsoleLogging.Value() != _T("0") ) {

            Core::Directory logdirectory(logpath.c_str());
            logdirectory.CreatePath(); //note: lxc API does not create the complate path for logging, it must exist

            string filename(logpath + "console.log");

            _lxccontainer->set_config_item(_lxccontainer, "lxc.console.size", config.ConsoleLogging.Value().c_str()); // yes this one is important!!! Otherwise file logging will not work
            _lxccontainer->set_config_item(_lxccontainer, "lxc.console.logfile", filename.c_str());
        }

        Core::JSON::ArrayType<Config::ConfigItem>::Iterator index(config.ConfigItems.Elements());
        while( index.Next() == true ) {
            _lxccontainer->set_config_item(_lxccontainer, index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str());
        };

        _networkInterfaces = GetNetworkInterfaces();
    }

    const string LXCContainer::Id() const 
    {
        return _name;
    }

    pid_t LXCContainer::Pid() const
    {
        return _pid;
    }

    LXCContainer::MemoryInfo LXCContainer::Memory() const  
    {
        ASSERT(_lxccontainer != nullptr);

        MemoryInfo result {UINT64_MAX, UINT64_MAX, UINT64_MAX};
        char buffer[2048];
        int32_t read = _lxccontainer->get_cgroup_item(_lxccontainer, "memory.usage_in_bytes", buffer, sizeof(buffer));

        // Not sure if "read < sizeof(buffer)" is realy needed, but it is checked in offical lxc tools sources
        if ((read > 0) && (read < sizeof(buffer))) {
            int32_t scanned = sscanf(buffer, "%llu", &result.allocated);

            if (scanned != 1) {
                TRACE(Trace::Warning, ("Could not read allocated memory of LXC container"));
            }
        }

        read = _lxccontainer->get_cgroup_item(_lxccontainer, "memory.stat", buffer, sizeof(buffer));

        if ((read > 0) && (static_cast<uint32_t>(read) < sizeof(buffer))) {
            char name[128];
            uint64_t value;
            uint32_t position = 0;

            while (position < read) {
                int32_t charsRead = 0;
                int32_t scanned = sscanf(buffer + position, "%s %llu%n", name, &value, &charsRead);

                if (scanned != 2) {
                    break;
                }

                if (strcmp(name, "rss") == 0) {
                    result.resident = value;
                } else if (strcmp(name, "shmem") == 0) {
                    result.shared = value;
                }

                position += charsRead;
            }
        } else {
            TRACE(Trace::Warning, ("Could not read memory usage of LXC container"));
        } 

        return result;
    }

    LXCContainer::CPUInfo LXCContainer::Cpu() const
    {
        ASSERT(_lxccontainer != nullptr);

        CPUInfo result;
        char buffer[512];
        uint32_t read = _lxccontainer->get_cgroup_item(_lxccontainer, "cpuacct.usage", buffer, sizeof(buffer));

        if (read != 0 && read < sizeof(buffer)) {
            result.total = strtoll(buffer, nullptr, 10);
        } else {
            TRACE(Trace::Warning, ("Could not read total cpu usage of LXC container"));
        } 

        read = _lxccontainer->get_cgroup_item(_lxccontainer, "cpuacct.usage_percpu", buffer, sizeof(buffer));

        if ((read != 0) && (static_cast<uint32_t>(read) < sizeof(buffer))) {
            char* pos = buffer;
            char* end;

            // We might now maximum number of threads in advance
            static const uint32_t numThreads = std::thread::hardware_concurrency();
            if (numThreads != 0)
                result.threads.reserve(numThreads);

            while(true) {
                uint64_t value = strtoull(pos, &end, 10);

                if (pos == end)
                    break;

                result.threads.push_back(value);
                pos = end;
            }
        } else {
            TRACE(Trace::Warning, ("Could not per thread cpu-usage of LXC container"));
        }

        return result;
    }

    string LXCContainer::ConfigPath() const 
    {
        return (_lxcpath + Id() + "/" + LXCContainerAdministrator::configFileName);
    }

    string LXCContainer::LogPath() const 
    {
        return (_logpath + LXCContainerAdministrator::logFileName);
    }

    IConstStringIterator LXCContainer::NetworkInterfaces() const
    {
        return ProcessContainers::IConstStringIterator(_networkInterfaces);
    }

    std::vector<Core::NodeId> LXCContainer::IPs(const string& interface) const 
    {
        std::vector<Core::NodeId> result;

        // if no interface name is specified, all addresess will be returned
        const char* interfaceName = interface.empty() ? nullptr : interface.c_str(); 

        char **addresses = _lxccontainer->get_ips(_lxccontainer, interfaceName, NULL, 0);
        if (addresses) {
            for (int i = 0; addresses[i] != nullptr; i++) {
                result.emplace_back(addresses[i]);
            }
        }

        return result;
    }

    bool LXCContainer::IsRunning() const 
    {
        return _lxccontainer->is_running(_lxccontainer);
    }

    bool LXCContainer::Start(const string& command, IStringIterator& parameters) 
    {
        bool result = false;

        std::vector<const char*> params(parameters.Count()+2);
        parameters.Reset(0);
        uint16_t pos = 0;
        params[pos++] = command.c_str();

        while( parameters.Next() == true ) {
            params[pos++] = parameters.Current().c_str();
        }
        params[pos++] = nullptr;
        ASSERT(pos == parameters.Count()+2);

        #ifdef __DEBUG__
            if( _attach == true ) {
                result = _lxccontainer->start(_lxccontainer, 0, NULL);
                if( result == true ) {

                    lxc_attach_command_t lxccommand;
                    lxccommand.program = (char *)command.c_str();
                    lxccommand.argv = const_cast<char**>(params.data());

                    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
                    int ret = _lxccontainer->attach(_lxccontainer, lxc_attach_run_command, &lxccommand, &options, &_pid);
                    if( ret != 0 ) {
                        _lxccontainer->shutdown(_lxccontainer, 0);
                    }
                    result = ret == 0;
                }
            } else
        #endif
        {
            result = _lxccontainer->start(_lxccontainer, 0, const_cast<char**>(params.data()));
        }

        if( result == true )  {
            _pid = _lxccontainer->init_pid(_lxccontainer);
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] was started successfully! pid=%u"), _name.c_str(), _pid));
        } else {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] could not be started!"), _name.c_str()));
        }

        return result;
    }

    bool LXCContainer::Stop(const uint32_t timeout /*ms*/) 
    {
        bool result = true;
        if( _lxccontainer->is_running(_lxccontainer)  == true ) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container name [%s] Stop activated"), _name.c_str()));
            int internaltimeout = timeout/1000;
            if( timeout == Core::infinite ) {
                internaltimeout = -1;
            }
            result = _lxccontainer->shutdown(_lxccontainer, internaltimeout);
        }
        return result;
    }

    void LXCContainer::AddRef() const {
        WPEFramework::Core::InterlockedIncrement(_referenceCount);
        lxc_container_get(_lxccontainer);
    }

    uint32_t LXCContainer::Release() const {
        uint32_t retval = WPEFramework::Core::ERROR_NONE;
        uint32_t lxcresult = lxc_container_put(_lxccontainer);
        if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
            ASSERT(lxcresult == 1); // if 1 is returned, lxc also released the container
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] released"), _name.c_str()));

            delete this;
            retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;

        }
        return retval;
    }

    LXCContainerAdministrator::LXCContainerAdministrator() 
        : _lock() 
    {
        TRACE(ProcessContainers::ProcessContainerization, (_T("LXC library initialization, version: %s"), lxc_get_version()));
    }

    LXCContainerAdministrator::~LXCContainerAdministrator() 
    {
            lxc_log_close();
    }

    void LXCContainerAdministrator::AddRef() const 
    {

    }
    uint32_t LXCContainerAdministrator::Release() const 
    {
        return Core::ERROR_NONE;
    }

    IContainer* LXCContainerAdministrator::Container(const string& name, ProcessContainers::IStringIterator& searchpaths, const string& logpath, const string& configuration) 
    {
        searchpaths.Reset(0);

        _lock.Lock();

        ProcessContainers::IContainer* container { nullptr };

        while( ( container == nullptr ) && ( searchpaths.Next() == true ) )  {
            LxcContainerType **clist = nullptr;
            int32_t numberofcontainersfound = list_defined_containers(searchpaths.Current().c_str(), nullptr, &clist);
            int32_t index = 0;
            while( ( container == nullptr) && ( index < numberofcontainersfound ) ) {
                LxcContainerType *c = clist[index];
                if( name == c->name ) {
                    container = new LXCContainer(name, c, logpath, configuration, searchpaths.Current());

                    _containers.push_back(container);
                }
                else {
                    lxc_container_put(c);
                }
                ++index;
            };
            if( numberofcontainersfound > 0 ) {
                free(clist);
            }
        };

        _lock.Unlock();

        if( container == nullptr ) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition for name [%s] could not be found!"), name.c_str()));
        }

        return container;
    }

    void LXCContainerAdministrator::Logging(const string& logpath, const string& logid, const string& loggingoptions) 
    {
        // Valid logging values: NONE and the ones below
        // 0 = trace, 1 = debug, 2 = info, 3 = notice, 4 = warn, 5 = error, 6 = critical, 7 = alert, and 8 = fatal, but also string is allowed:
        // (case insensitive) TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRIT, ALERT, FATAL

        const char* logstring = loggingoptions.c_str();

        if( ( loggingoptions.size() != 4 ) || ( std::toupper(logstring[0]) != _T('N') ) 
                                        || ( std::toupper(logstring[1]) != _T('O') ) 
                                        || ( std::toupper(logstring[2]) != _T('N') ) 
                                        || ( std::toupper(logstring[3]) != _T('E') ) 
            ) {
            string filename(logpath + logFileName);

            lxc_log log;
            log.name = logid.c_str();
            log.lxcpath = logpath.c_str(); //we don't have a correct lxcpath were all containers are stored...
            log.file = filename.c_str();
            log.level = logstring;
            log.prefix = logid.c_str();
            log.quiet = false;
            lxc_log_init(&log);
        }
    }

    LXCContainerAdministrator::ContainerIterator LXCContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    IContainerAdministrator& IContainerAdministrator::Instance()
    {
        static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

        return myLXCContainerAdministrator;
    }

    std::vector<string> LXCContainer::GetNetworkInterfaces() {
        char buf[256];
        std::vector<string> result;

        ASSERT(_lxccontainer != nullptr);

        for(int netnr = 0; ;netnr++) {
            sprintf(buf, "lxc.net.%d.type", netnr);

            char* type = _lxccontainer->get_running_config_item(_lxccontainer, buf);
            if (!type)
                break;

            if (strcmp(type, "veth") == 0) {
                sprintf(buf, "lxc.net.%d.veth.pair", netnr);
            } else {
                sprintf(buf, "lxc.net.%d.link", netnr);
            }
            free(type);

            char* ifname = _lxccontainer->get_running_config_item(_lxccontainer, buf);
            if (!ifname)
                break;

            result.emplace_back(ifname);
            free(ifname);
        }

        return result;
    }

    constexpr char* LXCContainerAdministrator::logFileName;
    constexpr char* LXCContainerAdministrator::configFileName;
    constexpr uint32_t LXCContainerAdministrator::maxReadSize;

}
} //namespace WPEFramework 



