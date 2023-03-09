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

#include "LXCImplementation.h"
#include "processcontainers/common/CGroupContainerInfo.h"

namespace WPEFramework {
namespace ProcessContainers {
    LXCNetworkInterfaceIterator::LXCNetworkInterfaceIterator(LxcContainerType* lxcContainer)
            : _current(UINT32_MAX)
            , _interfaces()

    {
        char buf[256];

        for (int netnr = 0;; netnr++) {
            LXCNetInterface interface;

            sprintf(buf, "lxc.net.%d.type", netnr);

            char* type = lxcContainer->get_running_config_item(lxcContainer, buf);
            if (!type)
                break;

            if (strcmp(type, "veth") == 0) {
                sprintf(buf, "lxc.net.%d.name", netnr);
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
    }

    bool LXCNetworkInterfaceIterator::Next()
    {
        if (_current == UINT32_MAX) {
            _current = 0;
        } else {
            ++_current;
        }

        return IsValid();
    }

    bool LXCNetworkInterfaceIterator::Previous()
    {
        if (_current == 0) {
            _current = UINT32_MAX;
        } else if (_current == UINT32_MAX) {
            _current = _interfaces.size() - 1;
        } else {
            --_current;
        }

        return IsValid();
    }

    void LXCNetworkInterfaceIterator::Reset(const uint32_t position)
    {
        (void) position;
        _current = UINT32_MAX;
    }

    bool LXCNetworkInterfaceIterator::IsValid() const
    {
        return _current < _interfaces.size();
    }

    uint32_t LXCNetworkInterfaceIterator::Index() const
    {
        return _current;
    }

    uint32_t LXCNetworkInterfaceIterator::Count() const
    {
        return _interfaces.size();
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
    uint16_t LXCNetworkInterfaceIterator::NumAddresses() const
    {
        return _interfaces.at(_current).numAddresses;
    }

    std::string LXCNetworkInterfaceIterator::Address(const uint16_t id) const
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
        , _adminLock()
        , _referenceCount(1)
        , _lxcContainer(lxcContainer)
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
        InheritRequestedEnvironment();

        if (config.ConsoleLogging.Value() != _T("0")) {

            Core::Directory logdirectory(_containerLogDir.c_str());
            logdirectory.CreatePath(); //note: lxc API does not create the complate path for logging, it must exist

            string filename(_containerLogDir + "/console.log");

            _lxcContainer->set_config_item(_lxcContainer, "lxc.console.size", config.ConsoleLogging.Value().c_str()); // yes this one is important!!! Otherwise file logging will not work
            _lxcContainer->set_config_item(_lxcContainer, "lxc.console.logfile", filename.c_str());
        }

        Core::JSON::ArrayType<Config::ConfigItem>::Iterator index(config.ConfigItems.Elements());
        while (index.Next() == true) {
            _lxcContainer->set_config_item(_lxcContainer, index.Current().Key.Value().c_str(), index.Current().Value.Value().c_str());
        };
    }

    LXCContainer::~LXCContainer()
    {
        if (IsRunning()) {
            Stop(2000);
        }

        TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] released"), _name.c_str()));

        static_cast<LXCContainerAdministrator&>(LXCContainerAdministrator::Instance()).RemoveContainer(this);
    }

    const string& LXCContainer::Id() const
    {
        return _name;
    }

    uint32_t LXCContainer::Pid() const
    {
        return _pid;
    }

    IMemoryInfo* LXCContainer::Memory() const
    {
        ASSERT(_lxcContainer != nullptr);

        CGroupMemoryInfo* result = new CGroupMemoryInfo;

        char buffer[2048];
        uint32_t read = _lxcContainer->get_cgroup_item(_lxcContainer, "memory.usage_in_bytes", buffer, sizeof(buffer));

        // Not sure if "read < sizeof(buffer)" is really needed, but it is checked in official lxc tools sources
        if ((read > 0) && (read < sizeof(buffer))) {
            uint64_t allocated;
            int32_t scanned = sscanf(buffer, "%llu", &allocated);

            result->Allocated(allocated);

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
                    result->Resident(value);
                } else if (strcmp(name, "mapped_file") == 0) {
                    result->Shared(value);
                }

                position += charsRead;
            }
        } else {
            TRACE(Trace::Warning, ("Could not read memory usage of LXC container"));
        }

        return result;
    }

    IProcessorInfo* LXCContainer::ProcessorInfo() const
    {
        ASSERT(_lxcContainer != nullptr);

        std::vector<uint64_t> cores;

        char buffer[512];
        uint32_t read = _lxcContainer->get_cgroup_item(_lxcContainer, "cpuacct.usage_percpu", buffer, sizeof(buffer));

        if ((read != 0) && (static_cast<uint32_t>(read) < sizeof(buffer))) {
            char* pos = buffer;
            char* end;

            // We might know maximum number of cores in advance
            static const uint32_t numCores = std::thread::hardware_concurrency();
            if (numCores != 0)
                cores.reserve(numCores);

            while (true) {
                uint64_t value = strtoull(pos, &end, 10);

                if (pos == end)
                    break;

                cores.push_back(value);
                pos = end;
            }
        } else {
            TRACE(Trace::Warning, ("Could not per thread cpu-usage of LXC container"));
        }

        return new CGroupProcessorInfo(std::move(cores));
    }

    INetworkInterfaceIterator* LXCContainer::NetworkInterfaces() const
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

        std::vector<const char*> params;
        params.reserve(parameters.Count() + 2);
        parameters.Reset(0);

        _adminLock.Lock();
        params.push_back(command.c_str());

        while (parameters.Next() == true) {
            params.push_back(parameters.Current().c_str());
        }
        params.push_back(nullptr);

#ifdef __DEBUG__
        if (_attach == true) {
            result = _lxcContainer->start(_lxcContainer, 0, NULL);
            if (result == true) {

                lxc_attach_command_t lxccommand;
                lxccommand.program = (char*)command.c_str();
                lxccommand.argv = const_cast<char**>(params.data());

                lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
                int ret = _lxcContainer->attach(_lxcContainer, lxc_attach_run_command, &lxccommand, &options, reinterpret_cast<pid_t*>(&_pid));
                if (ret != 0) {
                    _lxcContainer->shutdown(_lxcContainer, 0);
                }
                result = (ret == 0);
            }
        } else
#endif
        {
            result = _lxcContainer->start(_lxcContainer, 0, const_cast<char**>(params.data()));
        }

        if (result == true) {
            _pid = _lxcContainer->init_pid(_lxcContainer);
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] was started successfully! pid=%u"), _name.c_str(), _pid));
        } else {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] could not be started!"), _name.c_str()));
        }
        _adminLock.Unlock();

        return result;
    }

    bool LXCContainer::Stop(const uint32_t timeout /*ms*/)
    {
        bool result = true;

        _adminLock.Lock();
        if (_lxcContainer->is_running(_lxcContainer) == true) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container name [%s] Stop activated"), _name.c_str()));
            int internaltimeout = defaultTimeOutInMSec / 1000;
#if 0
            if (timeout == Core::infinite) {
                internaltimeout = -1;
            }

            if (internaltimeout != -1) {
                result = _lxcContainer->shutdown(_lxcContainer, internaltimeout);
            }

            if (internaltimeout == -1 || result == false) {
                result = _lxcContainer->stop(_lxcContainer);
            }
#endif
            result = _lxcContainer->shutdown(_lxcContainer, internaltimeout);
            if (!result){
                result = _lxcContainer->stop(_lxcContainer);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    void LXCContainer::AddRef() const
    {
        WPEFramework::Core::InterlockedIncrement(_referenceCount);
        lxc_container_get(_lxcContainer);
    }

    uint32_t LXCContainer::Release() const
    {
        uint32_t retval = WPEFramework::Core::ERROR_NONE;

        uint32_t lxcresult = lxc_container_put(_lxcContainer);
        if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
            ASSERT(lxcresult == 1); // if 1 is returned, lxc also released the container

            delete this;
            retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return retval;
    }

    void LXCContainer::InheritRequestedEnvironment()
    {
        // According to https://linuxcontainers.org/lxc/manpages/man5/lxc.container.conf.5.html#lbBM we
        // should be able to inherit env variables from host by using config syntax lxc.environment = ENV_NAME.
        // For some reason this doesn't work with current build of lxc, so we have to provide this functionality
        // from by ourselves

        uint32_t len = _lxcContainer->get_config_item(_lxcContainer, "lxc.environment", nullptr, 0);
        if (len > 0) {
            char* buffer = new char[len];

            uint32_t read = _lxcContainer->get_config_item(_lxcContainer, "lxc.environment", buffer, len);

            if (read > 0) {
                char* tmp;
                char* token = strtok_r(buffer, "\n", &tmp);

                while (token != nullptr) {
                    if (strchr(token, '=') == nullptr) {
                        string envVar;
                        Core::SystemInfo::GetEnvironment(token, envVar);
                        string key = string(token) + "=" + envVar;
                        _lxcContainer->set_config_item(_lxcContainer, "lxc.environment", key.c_str());
                    }

                    token = strtok_r(nullptr, "\n", &tmp);
                }
            }

            delete[] buffer;
        }
    }

    LXCContainerAdministrator::LXCContainerAdministrator()
        : BaseContainerAdministrator()
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
        LXCContainer* container = nullptr;

        searchpaths.Reset(0);
        while ((container == nullptr) && (searchpaths.Next() == true)) {
            LxcContainerType** clist = nullptr;
            int32_t numberofcontainersfound = list_defined_containers(searchpaths.Current().c_str(), nullptr, &clist);
            int32_t index = 0;

            while ((container == nullptr) && (index < numberofcontainersfound)) {
                LxcContainerType* c = clist[index];
                if (strcmp(c->name, "Container") == 0) {

                    this->InternalLock();
                    container = new LXCContainer(name, c, containerLogDir, configuration, searchpaths.Current());
                    InsertContainer(container);
                    this->InternalUnlock();
                } else {
                    lxc_container_put(c);
                }
                ++index;
            };
            if (numberofcontainersfound > 0) {
                free(clist);
            }
        };

        if (container == nullptr) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition for name [%s] could not be found!"), name.c_str()));
        }

        return static_cast<IContainer*>(container);
    }

    void LXCContainerAdministrator::Logging(const string& globalLogDir, const string& loggingOptions)
    {
        // Valid logging values: NONE and the ones below
        // 0 = trace, 1 = debug, 2 = info, 3 = notice, 4 = warn, 5 = error, 6 = critical, 7 = alert, and 8 = fatal, but also string is allowed:
        // (case insensitive) TRACE, DEBUG, INFO, NOTICE, WARN, ERROR, CRIT, ALERT, FATAL

        const char* logstring = loggingOptions.c_str();
        _globalLogDir = globalLogDir;

        if ((loggingOptions.size() != 4) || (std::toupper(logstring[0]) != _T('N'))
            || (std::toupper(logstring[1]) != _T('O'))
            || (std::toupper(logstring[2]) != _T('N'))
            || (std::toupper(logstring[3]) != _T('E'))) {
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

    IContainerAdministrator& IContainerAdministrator::Instance()
    {
        static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

        return myLXCContainerAdministrator;
    }

    constexpr char const* LXCContainerAdministrator::logFileName;
    constexpr char const* LXCContainerAdministrator::configFileName;
    constexpr uint32_t LXCContainerAdministrator::maxReadSize;

}
} //namespace WPEFramework
