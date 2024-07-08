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
#include <core/JSON.h>

namespace Thunder {

namespace ProcessContainers {

    static constexpr uint32_t DEFAULT_TIMEOUT = 5000 /* ms */;

    namespace runc {

        static constexpr const char* RUNC_EXECUTABLE = "/usr/bin/runc";
        static constexpr const char* CONFIG_FILE = "config.json";
        static constexpr const char* LOG_FILE = "runc.container.log";

        class Runner {
        public:
            class Info : public Core::JSON::Container {
            public:
                Info(Info&&) = delete;
                Info(const Info&) = delete;
                Info& operator=(Info&&) = delete;
                Info& operator=(const Info&) = delete;

            public:
                Info()
                    : Core::JSON::Container()
                    , Pid(0)
                    , Status()
                {
                    Add(_T("pid"), &Pid);
                    Add(_T("status"), &Status);
                }
                ~Info() override = default;

            public:
                Core::JSON::DecUInt32 Pid;
                Core::JSON::String Status;
            };

            class Config : public Core::JSON::Container {
            public:
                class ProcessData : public Core::JSON::Container {
                public:
                    ProcessData(ProcessData&&) = delete;
                    ProcessData(const ProcessData&) = delete;
                    ProcessData& operator=(ProcessData&&) = delete;
                    ProcessData& operator=(const ProcessData&) = delete;

                public:
                    ProcessData()
                        : Core::JSON::Container()
                        , Env()
                    {
                        Add(_T("env"), &Env);
                    }
                    ~ProcessData() override = default;

                public:
                    Core::JSON::ArrayType<Core::JSON::String> Env;
                };

            public:
                Config(Config&&) = delete;
                Config(const Config&) = delete;
                Config& operator=(Config&&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , Process()
                {
                    Add(_T("process"), &Process);
                }
                ~Config() override = default;

            public:
                ProcessData Process;
            };

        public:
            explicit Runner(const uint32_t timeout = Core::infinite)
                : _log()
                , _timeout(timeout)
            {
            }
            explicit Runner(const string& logPath, const uint32_t timeout = Core::infinite)
                : Runner(timeout)
            {
                if (logPath.empty() == false) {
                    Core::Directory path(logPath.c_str());

                    if (path.IsDirectory() == false) {
                        path.CreatePath();
                    }

                    _log = (logPath + LOG_FILE);
                }
            }

        public:
            uint32_t Create(const string& id, const string& bundlePath) const
            {
                ASSERT(id.empty() == false);
                ASSERT(bundlePath.empty() == false);

                Options options;

                if (_log.empty() == false) {
                    options.Add(_T("--log")).Add(_log);
                }

                options.Add(_T("create"));
                options.Add(_T("--no-new-keyring"));
                options.Add(_T("--bundle")).Add(bundlePath);
                options.Add(id);

                // runc --log <log-file> create --no-new-keyring --bundle <bundle-path> <container-id>

                const uint32_t result = Execute(options);

                if (result == Core::ERROR_NONE) {
                    TRACE_L2("Successfuly created container '%s'", id.c_str());
                }
                else {
                    TRACE_L1("Failed to create container '%s'", id.c_str());
                }

                return (result);
            }
            uint32_t Exec(const string& id, const string& bundlePath, const string& command, IStringIterator* parameters = nullptr) const
            {
                ASSERT(id.empty() == false);
                ASSERT(bundlePath.empty() == false);
                ASSERT(command.empty() == false);

                Options options;

                if (_log.empty() == false) {
                    options.Add(_T("--log")).Add(_log);
                }

                options.Add(_T("exec"));
                options.Add(_T("--detach"));

                InheritHostEnvironmentVariables(bundlePath, options);

                options.Add(id);

                options.Add(command);

                if (parameters != nullptr) {
                    while (parameters->Next() == true) {
                        options.Add(parameters->Current());
                    }
                }

                // runc --log <log-file> exec --detach [--env <variable=value>...] <container-id> <command> [command-parameters...]

                const uint32_t result = Execute(options);

                if (result == Core::ERROR_NONE) {
                    TRACE_L2("Successfuly executed command '%s' on container '%s'", command.c_str(), id.c_str());
                }
                else {
                    TRACE_L1("Failed to execute command '%s' on container '%s'", command.c_str(), id.c_str());
                }

                return (result);
            }
            uint32_t Delete(const string& id, const uint32_t timeout = 0) const
            {
                ASSERT(id.empty() == false);

                Options options;

                if (_log.empty() == false) {
                    options.Add(_T("--log")).Add(_log);
                }

                options.Add(_T("delete"));
                options.Add(_T("--force"));
                options.Add(id);

                // runc --log <log-file> delete --force <container-id>

                const uint32_t result = Execute(options, nullptr, timeout);

                if (result == Core::ERROR_NONE) {
                    TRACE_L2("Successfuly deleted container '%s'", id.c_str());
                }
                else {
                    TRACE_L1("Failed to delete container '%s'", id.c_str());
                }

                return (result);
            }
            uint32_t State(const string& id, Info& info) const
            {
                ASSERT(id.empty() == false);

                Options options;
                options.Add(_T("state"));
                options.Add(id);

                string output;

                // runc state <container-id>

                const uint32_t result = Execute(options, &output);

                if (result ==  Core::ERROR_NONE) {
                    info.FromString(output);

                    TRACE_L2("Successfully retrieved state from container '%s'", id.c_str());
                }
                else {
                    TRACE_L1("Failed to retrieve state from container '%s'", id.c_str());
                }

                return (result);
            }
            uint32_t List(string& list) const
            {
                Options options;
                options.Add(_T("list"));
                options.Add(_T("--quiet"));

                // runc list --quiet

                const uint32_t result = Execute(options, &list);

                if (result ==  Core::ERROR_NONE) {
                    TRACE_L2("Successfully retrieved container list (%s)", list.c_str());
                }
                else {
                    TRACE_L1("Failed to retrieve container list");
                }

                return (result);
            }

        private:
            class Options : public Core::Process::Options {
            public:
                Options()
                    : Core::Process::Options(RUNC_EXECUTABLE)
                {
                }
                ~Options() = default;

                using Core::Process::Options::Options;
            };

        private:
            uint32_t Execute(Options& options, string* output = nullptr, const uint32_t timeout = 0) const
            {
                uint32_t result = Core::ERROR_NONE;

#ifdef __DEBUG__
                char cmdLine[2048];
                options.Line(cmdLine, sizeof(cmdLine));
                TRACE_L2("Executing '%s'",cmdLine);
#endif // __DEBUG__

                Core::Process process(output != nullptr);
                uint32_t pid = 0;

                if (process.Launch(options, &pid) != Core::ERROR_NONE) {
                    TRACE_L1("Failed to launch runc");
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    if (process.WaitProcessCompleted(timeout != 0? timeout : _timeout) != Core::ERROR_NONE) {
                        TRACE_L1("Call to runc timed out");
                        result = Core::ERROR_TIMEDOUT;
                    }
                    else {
                        if (process.ExitCode() != 0) {
                            TRACE_L2("Call to runc resulted in non-zero exit code: %d", process.ExitCode());
                            result = Core::ERROR_GENERAL;

                        }
                        else if (output != nullptr) {
                            char buffer[2048];

                            for (;;) {
                                const uint16_t bytes = process.Output(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer) - 1);

                                if (bytes > 0) {
                                    buffer[bytes] = '\0';
                                    (*output) += buffer;
                                }
                                else {
                                    break;
                                }
                            }
                        }
                    }
                }

                return (result);
            }

            static void InheritHostEnvironmentVariables(const string& bundlePath, Core::Process::Options& options)
            {
                // This goes through process/env entries in the config file. All entries that are setting
                // the variable to $VAR will be set here to the same value as on host.

                Core::File file(bundlePath + CONFIG_FILE);
                if (file.Open(true) == true) {
                    Config config;

                    if (config.IElement::FromFile(file) == true) {
                        auto element = config.Process.Env.Elements();

                        while (element.Next() == true) {
                            Core::TextSegmentIterator it(Core::TextFragment(element.Current().Value()), true, _T("="));

                            if (it.Next() == true) {
                                const string& var = it.Current().Text();

                                if ((it.Next() == true) && (it.Current().Length() > 1) && (it.Current().Text()[0] == '$')) {
                                    const string hostVar = &it.Current().Text()[1];
                                    string value;

                                    if ((Core::SystemInfo::GetEnvironment(hostVar, value) == true) && (value.size() != 0)) {
                                        TRACE_L1("Using host environment variable: %s", hostVar.c_str());
                                        options.Add(_T("--env")).Add(var + _T("=") + value);
                                    }
                                }
                            }
                        }
                    }

                    file.Close();
                }
                else {
                    TRACE_L1("Failed to open config.json file");
                }
            }

        private:
            string _log;
            uint32_t _timeout;
        };

    } // namespace runc


    // ContainerAdministrator

    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static RunCContainerAdministrator& runCContainerAdministrator = Core::SingletonType<RunCContainerAdministrator>::Instance();
        return (runCContainerAdministrator);
    }

    IContainer* RunCContainerAdministrator::Container(const string& id, IStringIterator& searchPaths, const string& logPath,
                    const string& configuration VARIABLE_IS_NOT_USED) /* override */
    {
        ASSERT(id.empty() == false);

        RunCContainer* container = nullptr;

        searchPaths.Reset(0);

        while (searchPaths.Next() == true) {

            const string containerPath(Core::Directory::Normalize(searchPaths.Current() + _T("Container")));
            Core::File configFile(containerPath + runc::CONFIG_FILE);

            if (configFile.Exists() == true) {
                container = new RunCContainer(id, containerPath, logPath);
                ASSERT(container != nullptr);

                InternalLock();
                InsertContainer(container);
                InternalUnlock();

                TRACE(Trace::Information, (_T("Container '%s' created from '%s'"), id.c_str(), configFile.Name().c_str()));

                break;
            }
        }

        if (container == nullptr) {
            TRACE(Trace::Error, (_T("Runc container configuration not found!")));
        }

        return (container);
    }

    void RunCContainerAdministrator::Logging(const string& logDir VARIABLE_IS_NOT_USED, const string& loggingOptions VARIABLE_IS_NOT_USED) /* override */
    {
        TRACE_L1("runc logging only on container level");
    }


    // Container

    RunCContainer::RunCContainer(const string& id, const string& path, const string& logPath)
        : _adminLock()
        , _id(id)
        , _path(path)
        , _pid(0)
        , _runner(new runc::Runner(logPath, DEFAULT_TIMEOUT))
    {
        ASSERT(_id.empty() == false);
        ASSERT(_path.empty() == false);

        // Make sure it's not exisiting.
        InternalPurge();
    }

    RunCContainer::~RunCContainer()
    {
        _adminLock.Lock();

        static_cast<RunCContainerAdministrator&>(ProcessContainers::IContainerAdministrator::Instance()).RemoveContainer(this);

        InternalPurge(DEFAULT_TIMEOUT);

        _adminLock.Unlock();
    }

    const string& RunCContainer::Id() const /* override */
    {
        return (_id);
    }

    uint32_t RunCContainer::Pid() const /* override */
    {
        uint32_t result = 0;

        _adminLock.Lock();

        if (_pid == 0) {
            runc::Runner::Info info;

            if (_runner->State(_id, info) == Core::ERROR_NONE) {
                _pid = info.Pid;

                TRACE_L1("Container '%s' PID is %u", _id.c_str(), _pid);
            }
        }

        result = _pid;

        _adminLock.Unlock();

        return (result);
    }

    bool RunCContainer::IsRunning() const /* override */
    {
        bool result = false;

        string containers;

        if (_runner->List(containers) == Core::ERROR_NONE) {

            if (containers.find(_id) != string::npos) {
                runc::Runner::Info info;

                if (_runner->State(_id, info) == Core::ERROR_NONE) {
                    result = info.Status.IsSet();

                    TRACE_L1("Container '%s' is %s", _id.c_str(), info.Status.Value().c_str());
                }
            }
            else {
                TRACE_L1("Container '%s' is not available", _id.c_str());
            }
        }

        return (result);
    }

    bool RunCContainer::Start(const string& command, IStringIterator& parameters) /* override */
    {
        ASSERT(command.empty() == false);

        TRACE(Trace::Information, (_T("Starting container '%s'..."), _id.c_str()));

        _adminLock.Lock();

        ASSERT(IsRunning() == false);

        uint32_t result = _runner->Create(_id, _path);

        if (result == Core::ERROR_NONE) {
            result = _runner->Exec(_id, _path, command, &parameters);
        }

        _adminLock.Unlock();

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T("Failed to start container '%s'"), _id.c_str()));
        }

        return (result == Core::ERROR_NONE);
    }

    bool RunCContainer::Stop(const uint32_t timeout /* ms */) /* override */
    {
        bool result = false;

        TRACE_L1("Stopping container '%s'...", _id.c_str());

        _adminLock.Lock();

        const uint32_t purgeResult = InternalPurge(timeout);

        _adminLock.Unlock();

        if ((purgeResult == Core::ERROR_NONE) || (purgeResult == Core::ERROR_ALREADY_RELEASED)) {

            if (IsRunning() == false) {
                TRACE(Trace::Information, (_T("Container '%s' has been stopped"), _id.c_str()));
                result = true;
            }
            else {
                ASSERT(!"Container is still running");
            }
        }

        if (result == false) {
            TRACE(Trace::Error, (_T("Container '%s' failed to stop"), _id.c_str()));
        }

        return (result);
    }

    IMemoryInfo* RunCContainer::Memory() const /* override */
    {
        CGroupMetrics containerMetrics(_id);

        return (containerMetrics.Memory());
    }

    IProcessorInfo* RunCContainer::ProcessorInfo() const /* override */
    {
        CGroupMetrics containerMetrics(_id);

        return (containerMetrics.ProcessorInfo());
    }

    INetworkInterfaceIterator* RunCContainer::NetworkInterfaces() const /* override */
    {
        return (nullptr);
    }

    uint32_t RunCContainer::InternalPurge(const uint32_t timeout /* ms */)
    {
        return (IsRunning()? _runner->Delete(_id, timeout) : static_cast<uint32_t>(Core::ERROR_ALREADY_RELEASED));
    }

} // namespace ProcessContainers

}