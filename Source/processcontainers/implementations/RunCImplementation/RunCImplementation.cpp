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

#include "RunCImplementation.h"
#include "JSON.h"
#include <thread>

namespace WPEFramework {

uint32_t callRunC(Core::Process::Options& options, string* output, uint32_t timeout = Core::infinite)
{

    uint32_t result = Core::ERROR_NONE;
    bool capture = (output != nullptr);
    Core::Process process(capture);
    uint32_t pid;

    if (process.Launch(options, &pid) != Core::ERROR_NONE) {
        TRACE_L1("[RunC] Failed to launch runc");
        result = Core::ERROR_UNAVAILABLE;
    } else {
        if (process.WaitProcessCompleted(timeout) != Core::ERROR_NONE) {
            TRACE_L1("[RunC] Call to runc timed out (%u ms)", timeout);
            result = Core::ERROR_TIMEDOUT;
        } else {
            if (process.ExitCode() != 0) {
                TRACE_L1("[RunC] Call to runc resulted in non-zero exit code: %d", process.ExitCode());
                result = Core::ERROR_GENERAL;

            } else if (capture) {
                char buffer[2048];
                while (process.Output(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer)) > 0) {
                    *output += buffer;
                }
            }
        }
    }

    return result;
}

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
    ~RunCStatus() override = default;

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
    ~RunCListEntry() override = default;

public:
    Core::JSON::String Id;
};

namespace ProcessContainers {
    // Container administrator
    // ----------------------------------
    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static RunCContainerAdministrator& runCContainerAdministrator = Core::SingletonType<RunCContainerAdministrator>::Instance();

        return runCContainerAdministrator;
    }

    IContainer* RunCContainerAdministrator::Container(const string& id, IStringIterator& searchpaths, const string& logpath, const string& configuration)
    {
        searchpaths.Reset(0);
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            Core::File configFile(path + "/Container/config.json");

            if (configFile.Exists()) {
                // Make sure no leftover will interfere...
                if (ContainerNameTaken(id)) {
                    DestroyContainer(id);
                }
                this->InternalLock();
                RunCContainer* container = new RunCContainer(id, path + "/Container", logpath);
                InsertContainer(container);
                this->InternalUnlock();

                return container;
            }
        }

        return nullptr;
    }

    RunCContainerAdministrator::RunCContainerAdministrator()
        : BaseContainerAdministrator()
    {
    }

    RunCContainerAdministrator::~RunCContainerAdministrator()
    {
    }

    void RunCContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        // Only container-scope logging
    }

    void RunCContainerAdministrator::DestroyContainer(const string& name)
    {
        if (callRunC(Core::Process::Options("/usr/bin/runc").Add("delete").Add("-f").Add(name), nullptr) != Core::ERROR_NONE) {
            TRACE_L1("Failed do destroy a container named %s", name.c_str());
        }
    }

    bool RunCContainerAdministrator::ContainerNameTaken(const string& name)
    {
        bool result = false;

        string output = "";
        if (callRunC(Core::Process::Options("/usr/bin/runc").Add("list").Add("-q"), &output) != Core::ERROR_NONE) {
            result = false;
        } else {
            result = (output.find(name) != std::string::npos);
        }

        return result;
    }

    // Container
    // ------------------------------------
    RunCContainer::RunCContainer(const string& name, const string& path, const string& logPath)
        : _adminLock()
        , _name(name)
        , _path(path)
        , _logPath(logPath)
        , _pid()
    {
    }

    RunCContainer::~RunCContainer()
    {
        auto& admin = static_cast<RunCContainerAdministrator&>(RunCContainerAdministrator::Instance());

        if (admin.ContainerNameTaken(_name) == true) {
            Stop(Core::infinite);
        }

        admin.RemoveContainer(this);
    }

    const string& RunCContainer::Id() const
    {
        return _name;
    }

    uint32_t RunCContainer::Pid() const
    {
        uint32_t returnedPid = 0;

        if (_pid.IsSet() == false) {
            Core::Process::Options options("/usr/bin/runc");

            options.Add("state").Add(_name);

            Core::Process process(true);

            uint32_t tmp;
            if (process.Launch(options, &tmp) != Core::ERROR_NONE) {
                TRACE_L1("Failed to create RunC container with name: %s", _name.c_str());
                returnedPid = 0;
            } else {

                process.WaitProcessCompleted(Core::infinite);

                if (process.ExitCode() != 0) {
                    returnedPid = 0;
                } else {

                    char data[1024];
                    process.Output((uint8_t*)data, 2048);

                    RunCStatus info;
                    info.FromString(string(data));

                    _pid = info.Pid.Value();
                    returnedPid = _pid;
                }
            }
        }

        return returnedPid;
    }

    IMemoryInfo* RunCContainer::Memory() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.Memory();
    }

    IProcessorInfo* RunCContainer::ProcessorInfo() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.ProcessorInfo();
    }

    INetworkInterfaceIterator* RunCContainer::NetworkInterfaces() const
    {
        return nullptr;
    }

    bool RunCContainer::IsRunning() const
    {
        bool result = false;
        string output = "";
        if (callRunC(Core::Process::Options("/usr/bin/runc").Add("state").Add(_name), &output) != Core::ERROR_NONE) {
            result = false;
        } else {
            RunCStatus info;
            info.FromString(string(output));

            result = info.Status.Value() == "running";
        }

        return result;
    }

    bool RunCContainer::Start(const string& command, IStringIterator& parameters)
    {
        Core::JSON::ArrayType<Core::JSON::String> paramsJSON;
        Core::JSON::String tmp;
        bool result = false;

        _adminLock.Lock();
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
            Core::Directory(_logPath.c_str()).CreatePath();

            options.Add("-log").Add(_logPath + "container.log");
        }
        options.Add("run")
            .Add("-d")
            .Add("--args")
            .Add(paramsFormated)
            .Add("-b")
            .Add(_path)
            .Add("--no-new-keyring")
            .Add(_name)
            .Add(command);

        if (callRunC(options, nullptr) != Core::ERROR_NONE) {
            TRACE_L1("Failed to create RunC container with name: %s", _name.c_str());
        } else {
            result = true;
        }
        _adminLock.Unlock();

        return result;
    }

    bool RunCContainer::Stop(const uint32_t timeout /*ms*/)
    {
        bool result = false;

        _adminLock.Lock();
        if (callRunC(Core::Process::Options("/usr/bin/runc").Add("delete").Add("-f").Add(_name), nullptr, timeout) != Core::ERROR_NONE) {
            TRACE_L1("Failed to destroy RunC container named: %s", _name.c_str());
        } else {
            result = true;
        }
        _adminLock.Unlock();

        return result;
    }

} // namespace ProcessContainers

} // namespace WPEFramework
