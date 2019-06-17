#include "Module.h"

#include "ProcessContainer.h"

#include "Tracing.h"
#include <lxc/lxccontainer.h>
#include <vector>
#include <utility>

namespace WPEFramework {

class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
private:

    using LxcContainerType = struct lxc_container;

public:
    LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
    LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;

    LXCContainerAdministrator() 
        : _lock() {
            TRACE(ProcessContainers::ProcessContainerization, (_T("LXC library initialization, version: %s"), lxc_get_version()));
        }

    virtual ~LXCContainerAdministrator() {
            lxc_log_close();
    }

    class LCXContainer : public ProcessContainers::IContainerAdministrator::IContainer {
    private:

        class Config : public Core::JSON::Container {
        public:
            class ConfigItem : public Core::JSON::Container {
            public:
                ConfigItem& operator=(const ConfigItem&) = delete;

                ConfigItem(const ConfigItem& rhs) 
                    : Core::JSON::Container()
                    , Key(rhs.Key)
                    , Value(rhs.Value)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value); 
                }

                ConfigItem()
                    : Core::JSON::Container()
                    , Key()
                    , Value()
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value); 
                }
                
                ~ConfigItem() = default;

                Core::JSON::String Key;
                Core::JSON::String Value;
            };
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , LXCLogging(false)
                , LXCLogLevel("ERROR")
                , ConsoleLogging(false)
                , ConsoleLogFileSize("auto")
                , ConsoleBufferSize("0")
#ifdef __DEBUG__
                , DebugMode(false)
#endif
            {
                Add(_T("lxclogging"), &LXCLogging);
                Add(_T("lxcloglevel"), &LXCLogLevel); // 0 = trace, 1 = debug, 2 = info, 3 = notice, 4 = warn, 5 = error, 6 = critical, 7 = alert, and 8 = fatal, but also string is allowed:
                                                      // (case insensitive) TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRIT, ALERT, FATAL

                Add(_T("consolelogging"), &ConsoleLogging);
                Add(_T("consolelogfilesize"), &ConsoleLogFileSize); // should be a power of 2 when converted to bytes. Valid size prefixes are 'KB', 'MB', 'GB'
                Add(_T("consolelogbuffersize"), &ConsoleBufferSize); // NOt used yet, to later on enable getting the console using an interface, should be a power of 2 when converted to bytes. Valid size prefixes are 'KB', 'MB', 'GB'

                Add(_T("configuration_items"), &ConfigItems);

#ifdef __DEBUG__
                Add(_T("debugmode"), &DebugMode);
#endif
            }
            
            ~Config() = default;

            Core::JSON::Boolean LXCLogging;
            Core::JSON::String LXCLogLevel;
            Core::JSON::Boolean ConsoleLogging;
            Core::JSON::String ConsoleLogFileSize;
            Core::JSON::String ConsoleBufferSize;
            Core::JSON::ArrayType<ConfigItem> ConfigItems;
#ifdef __DEBUG__
            Core::JSON::Boolean DebugMode;
#endif
        };

    public:

        LCXContainer(const LCXContainer&) = delete;
        LCXContainer& operator=(const LCXContainer&) = delete;

        LCXContainer(const string& name, LxcContainerType* lxccontainer, const string& logpath, const string& configuration, const string& lxcpath)
            : _name(name)
            , _pid(0)
            , _lxcpath(lxcpath)
            , _referenceCount(1)
            , _lxccontainer(lxccontainer)
#ifdef __DEBUG__
            , _debugmode(false)
#endif
        {
            Config config;
            config.FromString(configuration);

#ifdef __DEBUG__
            _debugmode = config.DebugMode.Value();
#endif
            string pathName;
            Core::SystemInfo::GetEnvironment("PATH", pathName);
            string key("PATH");
            key += "=";
            key += pathName;
            _lxccontainer->set_config_item(_lxccontainer, "lxc.environment", key.c_str());

            // const char* test = _lxccontainer->get_config_path(_lxccontainer); Note: this does not give you the correct path!!

            if( config.LXCLogging.Value() == true ) {
                // we need to move this probably to the admin, no possibility to set this per container, hippyware!!
                string filename(logpath + name + "/lxclogging.log");
                lxc_log log;
                log.name = name.c_str();
                log.lxcpath = _lxcpath.c_str();
                log.file = filename.c_str();
                log.level = config.LXCLogLevel.Value().c_str();
                log.prefix = name.c_str();
                log.quiet = false;
                lxc_log_init(&log);
            }

            if( config.ConsoleLogging.Value() == true ) {
                string filename(logpath + name + "/console.log");
                _lxccontainer->set_config_item(_lxccontainer, "lxc.console.buffer.size", config.ConsoleBufferSize.Value().c_str());
                _lxccontainer->set_config_item(_lxccontainer, "lxc.console.size", config.ConsoleLogFileSize.Value().c_str()); // yes this one is important!!! Otherwise file logging will not work
                _lxccontainer->set_config_item(_lxccontainer, "lxc.console.logfile", filename.c_str());
            }

            Core::JSON::ArrayType<Config::ConfigItem>::Iterator index(config.ConfigItems.Elements());
            while( index.Next() == true ) {
                _lxccontainer->set_config_item(_lxccontainer, index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str());
            };
        }

        const string& Id() const override {
            return _name;
        }

        pid_t Pid() const override {
            return _pid;
        }

        bool IsRunning() const override;

        bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override {
            WPEFramework::Core::InterlockedIncrement(_referenceCount);
            lxc_container_get(_lxccontainer);
        }

        uint32_t Release() const override {
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

        private:
            const string _name;
            pid_t _pid;
            string _lxcpath;
            mutable uint32_t _referenceCount;
            LxcContainerType* _lxccontainer;
#ifdef __DEBUG__
            bool _debugmode;
#endif
    };

    // Lifetime management
     void AddRef() const override {
    }
    uint32_t Release() const override {
        return Core::ERROR_NONE;
    }

    ProcessContainers::IContainerAdministrator::IContainer* Container(const string& name, 
                                                                      ProcessContainers::IStringIterator& searchpaths, 
                                                                      const string& logpath,
                                                                      const string& configuration) override;

private:
    mutable Core::CriticalSection _lock;
};

ProcessContainers::IContainerAdministrator::IContainer* LXCContainerAdministrator::Container(const string& name, 
                                                                                             ProcessContainers::IStringIterator& searchpaths,
                                                                                             const string& logpath,
                                                                                             const string& configuration) {
    searchpaths.Reset(0);

    _lock.Lock();

    ProcessContainers::IContainerAdministrator::IContainer* container { nullptr };

    while( ( container == nullptr ) && ( searchpaths.Next() == true ) )  {
        LxcContainerType **clist = nullptr;
        int32_t numberofcontainersfound = list_defined_containers(searchpaths.Current().c_str(), nullptr, &clist);
        int32_t index = 0;
        while( ( container == nullptr) && ( index < numberofcontainersfound ) ) {
            LxcContainerType *c = clist[index];
            if( name == c->name ) {
                container = new LXCContainerAdministrator::LCXContainer(name, c, logpath, configuration, searchpaths.Current());
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

ProcessContainers::IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
{
    static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

    return myLXCContainerAdministrator;
}

bool LXCContainerAdministrator::LCXContainer::Start(const string& command, ProcessContainers::IStringIterator& parameters) {

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
    if( _debugmode == true ) {
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
        result = _lxccontainer->start(_lxccontainer, 1, const_cast<char**>(params.data()));
    }

    if( result == true )  {
        _pid = _lxccontainer->init_pid(_lxccontainer);
        TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] was started successfully! pid=%u"), _name.c_str(), _pid));
    } else {
        TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] could not be started!"), _name.c_str()));
    }

    return result;
}

bool LXCContainerAdministrator::LCXContainer::Stop(const uint32_t timeout /*ms*/) {
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

bool LXCContainerAdministrator::LCXContainer::IsRunning() const {
    return _lxccontainer->is_running(_lxccontainer);
}


} //namespace WPEFramework 



