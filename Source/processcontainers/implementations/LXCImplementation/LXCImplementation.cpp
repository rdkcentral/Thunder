#include "LXCImplementation.h"

namespace WPEFramework {
namespace ProcessContainers {
    LXCNetworkInterfaceIterator::LXCNetworkInterfaceIterator(LxcContainerType* lxcContainer)
        : NetworkInterfaceIterator()
    {
        char buf[256];

        for(int netnr = 0; ;netnr++) {
            LXCNetInterface interface;

            sprintf(buf, "lxc.net.%d.type", netnr);

            char* type = lxcContainer->get_running_config_item(lxcContainer, buf);
            if (!type)
                break;

            if (strcmp(type, "veth") == 0) {
                sprintf(buf, "lxc.net.%d.veth.pair", netnr);
            } else {
                sprintf(buf, "lxc.net.%d.link", netnr);
            }
            free(type);

            interface.name = lxcContainer->get_running_config_item(lxcContainer, buf);
            if (interface.name == nullptr)
                break;

            interface.addresses = lxcContainer->get_ips(lxcContainer, interface.name, NULL, 0);
            if (!interface.addresses) {
                TRACE_L1("Failed to get IP addreses inside container. Interface: %s", interface.name);
            }

            interface.numAddresses = 0;
            for (int i = 0; interface.addresses[i] != nullptr; i++) {
                interface.numAddresses++;
            }

            _interfaces.push_back(interface);
        }

        _count = _interfaces.size();
    }

    LXCNetworkInterfaceIterator::~LXCNetworkInterfaceIterator()
    {
        for (auto interface : _interfaces) {
            free(interface.addresses);
            free(interface.name);
        }
    }

    std::string LXCNetworkInterfaceIterator::Name() const 
    {
        return _interfaces.at(_current).name;
    }
    uint32_t LXCNetworkInterfaceIterator::NumIPs() const 
    {
        return _interfaces.at(_current).numAddresses;
    }

    std::string LXCNetworkInterfaceIterator::IP(uint32_t id) const 
    {
        ASSERT(id < _interfaces.at(_current).numAddresses);

        return _interfaces.at(_current).addresses[id];
    }

    LXCContainer::Config::ConfigItem::ConfigItem(const ConfigItem& rhs) 
        : Core::JSON::Container()
        , Key(rhs.Key)
        , Value(rhs.Value)
    {
        Add(_T("key"), &Key);
        Add(_T("value"), &Value); 
    }

    LXCContainer::Config::ConfigItem::ConfigItem()
        : Core::JSON::Container()
        , Key()
        , Value()
    {
        Add(_T("key"), &Key);
        Add(_T("value"), &Value); 
    }
    
    LXCContainer::Config::Config()
        : Core::JSON::Container()
        , ConsoleLogging("0")
        , ConfigItems()
#ifdef __DEBUG__
        , Attach(false)
#endif
    {
        Add(_T("console"), &ConsoleLogging); // should be a power of 2 when converted to bytes. Valid size prefixes are 'KB', 'MB', 'GB', 0 is off, auto is auto determined

        Add(_T("items"), &ConfigItems);

#ifdef __DEBUG__
        Add(_T("attach"), &Attach);
#endif
    }

    LXCContainer::LXCContainer(const string& name, LxcContainerType* lxcContainer, const string& containerLogDir, const string& configuration, const string& lxcPath)
        : _name(name)
        , _pid(0)
        , _lxcPath(lxcPath)
        , _containerLogDir(containerLogDir)
        , _referenceCount(1)
        , _lxcContainer(lxcContainer)
#ifdef __DEBUG__
        , _attach(false)
#endif
    {
        Config config;
        Core::OptionalType<Core::JSON::Error> error;
        config.FromString(configuration, error);
        if (error.IsSet() == true) {
            SYSLOG(Logging::ParsingError, (_T("Parsing container %s configuration failed with %s"), name.c_str(), ErrorDisplayMessage(error.Value()).c_str()));
        }

#ifdef __DEBUG__
            _attach = config.Attach.Value();
#endif
        string pathName;
        Core::SystemInfo::GetEnvironment("PATH", pathName);
        string key("PATH");
        key += "=";
        key += pathName;
        _lxcContainer->set_config_item(_lxcContainer, "lxc.environment", key.c_str());

        if( config.ConsoleLogging.Value() != _T("0") ) {

            Core::Directory logdirectory(_containerLogDir.c_str());
            logdirectory.CreatePath(); //note: lxc API does not create the complate path for logging, it must exist

            string filename(_containerLogDir + "/console.log");

            _lxcContainer->set_config_item(_lxcContainer, "lxc.console.size", config.ConsoleLogging.Value().c_str()); // yes this one is important!!! Otherwise file logging will not work
            _lxcContainer->set_config_item(_lxcContainer, "lxc.console.logfile", filename.c_str());
        }

        Core::JSON::ArrayType<Config::ConfigItem>::Iterator index(config.ConfigItems.Elements());
        while( index.Next() == true ) {
            _lxcContainer->set_config_item(_lxcContainer, index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str());
        };
    }

    const string LXCContainer::Id() const 
    {
        return _name;
    }

    uint32_t LXCContainer::Pid() const
    {
        return _pid;
    }

    LXCContainer::MemoryInfo LXCContainer::Memory() const  
    {
        ASSERT(_lxcContainer != nullptr);

        MemoryInfo result {UINT64_MAX, UINT64_MAX, UINT64_MAX};
        char buffer[2048];
        int32_t read = _lxcContainer->get_cgroup_item(_lxcContainer, "memory.usage_in_bytes", buffer, sizeof(buffer));

        // Not sure if "read < sizeof(buffer)" is really needed, but it is checked in official lxc tools sources
        if ((read > 0) && (read < sizeof(buffer))) {
            int32_t scanned = sscanf(buffer, "%llu", &result.allocated);

            if (scanned != 1) {
                TRACE(Trace::Warning, ("Could not read allocated memory of LXC container"));
            }
        }

        read = _lxcContainer->get_cgroup_item(_lxcContainer, "memory.stat", buffer, sizeof(buffer));

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
        ASSERT(_lxcContainer != nullptr);

        CPUInfo result;
        char buffer[512];
        uint32_t read = _lxcContainer->get_cgroup_item(_lxcContainer, "cpuacct.usage", buffer, sizeof(buffer));

        if (read != 0 && read < sizeof(buffer)) {
            result.total = strtoll(buffer, nullptr, 10);
        } else {
            TRACE(Trace::Warning, ("Could not read total cpu usage of LXC container"));
        } 

        read = _lxcContainer->get_cgroup_item(_lxcContainer, "cpuacct.usage_percpu", buffer, sizeof(buffer));

        if ((read != 0) && (static_cast<uint32_t>(read) < sizeof(buffer))) {
            char* pos = buffer;
            char* end;

            // We might know maximum number of cores in advance
            static const uint32_t numCores = std::thread::hardware_concurrency();
            if (numCores != 0)
                result.cores.reserve(numCores);

            while(true) {
                uint64_t value = strtoull(pos, &end, 10);

                if (pos == end)
                    break;

                result.cores.push_back(value);
                pos = end;
            }
        } else {
            TRACE(Trace::Warning, ("Could not per thread cpu-usage of LXC container"));
        }

        return result;
    }

    NetworkInterfaceIterator* LXCContainer::NetworkInterfaces() const
    {
        return new LXCNetworkInterfaceIterator(_lxcContainer);
    }

    bool LXCContainer::IsRunning() const 
    {
        return _lxcContainer->is_running(_lxcContainer);
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
                result = _lxcContainer->start(_lxcContainer, 0, NULL);
                if( result == true ) {

                    lxc_attach_command_t lxccommand;
                    lxccommand.program = (char *)command.c_str();
                    lxccommand.argv = const_cast<char**>(params.data());

                    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
                    int ret = _lxcContainer->attach(_lxcContainer, lxc_attach_run_command, &lxccommand, &options, reinterpret_cast<pid_t*>(&_pid));
                    if( ret != 0 ) {
                        _lxcContainer->shutdown(_lxcContainer, 0);
                    }
                    result = ret == 0;
                }
            } else
#endif
        {
            result = _lxcContainer->start(_lxcContainer, 0, const_cast<char**>(params.data()));
        }

        if( result == true )  {
            _pid = _lxcContainer->init_pid(_lxcContainer);
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] was started successfully! pid=%u"), _name.c_str(), _pid));
        } else {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] could not be started!"), _name.c_str()));
        }

        return result;
    }

    bool LXCContainer::Stop(const uint32_t timeout /*ms*/) 
    {
        bool result = true;
        if( _lxcContainer->is_running(_lxcContainer)  == true ) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container name [%s] Stop activated"), _name.c_str()));
            int internaltimeout = timeout/1000;
            if( timeout == Core::infinite ) {
                internaltimeout = -1;
            }
            result = _lxcContainer->shutdown(_lxcContainer, internaltimeout);
        }
        return result;
    }

    void LXCContainer::AddRef() const {
        WPEFramework::Core::InterlockedIncrement(_referenceCount);
        lxc_container_get(_lxcContainer);
    }

    uint32_t LXCContainer::Release() {
        uint32_t retval = WPEFramework::Core::ERROR_NONE;
        uint32_t lxcresult = lxc_container_put(_lxcContainer);
        if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
            ASSERT(lxcresult == 1); // if 1 is returned, lxc also released the container
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] released"), _name.c_str()));

            static_cast<LXCContainerAdministrator&>(LXCContainerAdministrator::Instance()).RemoveContainer(this);

            delete this;
            retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;

        }
        return retval;
    }

    LXCContainerAdministrator::LXCContainerAdministrator() 
        : _lock() 
        ,_containers()
        , _globalLogDir()
    {
        TRACE(ProcessContainers::ProcessContainerization, (_T("LXC library initialization, version: %s"), lxc_get_version()));
    }

    LXCContainerAdministrator::~LXCContainerAdministrator() 
    {
            lxc_log_close();
    }

    IContainer* LXCContainerAdministrator::Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration) 
    {
        _lock.Lock();

        ProcessContainers::IContainer* container { nullptr };

        while( ( container == nullptr ) && ( searchpaths.Next() == true ) )  {
            LxcContainerType **clist = nullptr;
            int32_t numberofcontainersfound = list_defined_containers(searchpaths.Current().c_str(), nullptr, &clist);
            int32_t index = 0;
            while( ( container == nullptr) && ( index < numberofcontainersfound ) ) {
                LxcContainerType *c = clist[index];
                if( name == c->name ) {
                    container = new LXCContainer(name, c, containerLogDir, configuration, searchpaths.Current());

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

    void LXCContainerAdministrator::Logging(const string& globalLogDir, const string& loggingOptions) 
    {
        // Valid logging values: NONE and the ones below
        // 0 = trace, 1 = debug, 2 = info, 3 = notice, 4 = warn, 5 = error, 6 = critical, 7 = alert, and 8 = fatal, but also string is allowed:
        // (case insensitive) TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRIT, ALERT, FATAL

        const char* logstring = loggingOptions.c_str();
        _globalLogDir = globalLogDir;

        if( ( loggingOptions.size() != 4 ) || ( std::toupper(logstring[0]) != _T('N') ) 
                                        || ( std::toupper(logstring[1]) != _T('O') ) 
                                        || ( std::toupper(logstring[2]) != _T('N') ) 
                                        || ( std::toupper(logstring[3]) != _T('E') )) 
        {
            // Create logging directory
            Core::Directory logDir(_globalLogDir.c_str());
            logDir.CreatePath();

            string filename(_globalLogDir + "/" + logFileName);

            lxc_log log;
            log.name = "WPE_LXC";
            log.lxcpath = _globalLogDir.c_str(); //we don't have a correct lxcPath were all containers are stored...
            log.file = filename.c_str();
            log.level = logstring;
            log.prefix = "";
            log.quiet = false;
            lxc_log_init(&log);
        }
    }

    LXCContainerAdministrator::ContainerIterator LXCContainerAdministrator::Containers()
    {
        return ContainerIterator(_containers);
    }

    void LXCContainerAdministrator::AddRef() const
    {

    }

    uint32_t LXCContainerAdministrator::Release()
    {
        return (Core::ERROR_NONE);
    }


    IContainerAdministrator& IContainerAdministrator::Instance()
    {
        static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

        return myLXCContainerAdministrator;
    }

    void LXCContainerAdministrator::RemoveContainer(ProcessContainers::IContainer* container)
    {
        this->_containers.remove(container);
    }

    constexpr char const* LXCContainerAdministrator::logFileName;
    constexpr char const* LXCContainerAdministrator::configFileName;
    constexpr uint32_t LXCContainerAdministrator::maxReadSize;

}
} //namespace WPEFramework 



