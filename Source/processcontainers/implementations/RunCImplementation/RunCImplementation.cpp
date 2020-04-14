/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

class RunCListEntry : public Core::JSON::Container {
private:
    RunCListEntry(const RunCListEntry&);
    RunCListEntry& operator=(const RunCListEntry&);

public:
    RunCListEntry()
        : Core::JSON::Container()
        , Id()
    {
        Add(_T("id"), &Id);
    }
    ~RunCListEntry()
    {
    }

public:
    Core::JSON::String Id;
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
        searchpaths.Reset(0);
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            Core::File configFile(path + "/Container/config.json");

            if (configFile.Exists()) {
                // Make sure no leftover will interfere...
                DestroyContainer(id);

                RunCContainer* container = new RunCContainer(id, path + "/Container", logpath);
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

    void RunCContainerAdministrator::DestroyContainer(const string& name)
    {
        Core::Process::Options options("/usr/bin/runc");
        options.Add("delete").Add("-f").Add(name);

        Core::Process process(false);
        
        uint32_t pid;

        // TODO: Get rid of annoying "container Container does not exist" message
        if (process.Launch(options, &pid) != Core::ERROR_NONE) {
            TRACE_L1("[RunC] Failed to get a destroy a container");
        } else {
            process.WaitProcessCompleted(Core::infinite);
        
            if (process.ExitCode() == 0) {
                TRACE_L1("[RunC] Container named %s was already existent when trying to create it. Destroying previous instance...", name.c_str());
            }
        }
    }

    void RunCContainerAdministrator::RemoveContainer(IContainer* container)
    {
        _containers.remove(container);
        delete container;

        Release();
    }

    // Container
    // ------------------------------------
    RunCContainer::RunCContainer(string name, string path, string logPath)
        : CGroupContainerInfo(name)
        , _refCount(1)
        , _name(name)        
        , _path(path)
        , _logPath(logPath)
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

        if (_logPath.empty() == false) {
            // Create logging directory
            Core::Directory (_logPath.c_str()).CreatePath();

            options.Add("-log").Add(_logPath + "container.log");
        }

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


