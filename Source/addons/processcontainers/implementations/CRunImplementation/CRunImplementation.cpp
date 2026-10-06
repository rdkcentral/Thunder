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

#include "CRunImplementation.h"
#include "CRunConfig.h"
#include "processcontainers/ContainerProducer.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace Thunder {
namespace ProcessContainers {

    // PUBLIC_INTERFACE
    /** Initialize the stateless producer; logging remains container-scoped. */
    uint32_t CRunContainerAdministrator::Initialize(const string& configuration VARIABLE_IS_NOT_USED)
    {
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * Search the supplied paths for id's OCI bundle.
     * logpath controls container logging; configuration is currently unused.
     * Return an administrator-owned proxy or an invalid proxy when absent.
     */
    Core::ProxyType<IContainer> CRunContainerAdministrator::Container(const string& id,
        IStringIterator& searchpaths, const string& logpath, const string& configuration VARIABLE_IS_NOT_USED)
    {
        searchpaths.Reset(0);
        while (searchpaths.Next()) {
            const string path = searchpaths.Current();
            Core::File configFile(path + "/Container/config.json");
            if (configFile.Exists()) {
                // Cross-type proxy constructors are explicit; use the supported
                // assignment path used by RunC while retaining list ownership.
                Core::ProxyType<IContainer> container;
                container = ContainerAdministrator::Instance().Create<CRunContainer>(id, path + "/Container", logpath);
                return container;
            }
        }
        return Core::ProxyType<IContainer>();
    }

    // PUBLIC_INTERFACE
    /** Bind name to its bundle path and optional container log directory. */
    CRunContainer::CRunContainer(const string& name, const string& path, const string& logPath)
        : _adminLock()
        , _created(false)
        , _name(name)
        , _bundle(path)
        , _configFile(path + "/config.json")
        , _logPath(logPath)
        , _container(nullptr)
        , _context()
        , _pid()
    {
        _context.bundle = _bundle.c_str();
        _context.console_socket = nullptr;
        _context.detach = true;
        _context.fifo_exec_wait_fd = -1;
        _context.force_no_cgroup = false;
        _context.id = _name.c_str();
        _context.no_new_keyring = true;
        _context.no_pivot = false;
        // libcrun 1.21 exposes no no_subreaper context switch; leave
        // subreaper handling to libcrun rather than assigning an absent member.
        _context.notify_socket = nullptr;
        _context.output_handler = nullptr;
        _context.output_handler_arg = nullptr;
        _context.pid_file = nullptr;
        _context.preserve_fds = 0;
        _context.state_root = "/run/crun";
        _context.systemd_cgroup = 0;

        if (logPath.empty() == false) {
            Core::Directory(logPath.c_str()).CreatePath();
            libcrun_error_t error = nullptr;
            const int ret = libcrun_init_logging(&_context.output_handler, &_context.output_handler_arg,
                _name.c_str(), (logPath + "/container.log").c_str(), &error);
            if (ret != 0) {
                // A failed external call need not provide an error object.
                TRACE_L1("Cannot initialize logging of container %s in %s. Error %d: %s",
                    _name.c_str(), logPath.c_str(), error != nullptr ? error->status : ret,
                    error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
            }
        }
    }

    // PUBLIC_INTERFACE
    /** Stop a created container before its administrator-owned proxy expires. */
    CRunContainer::~CRunContainer()
    {
        if (_created == true) {
            Stop(Core::infinite);
        }
        // The shared proxy list replaces obsolete explicit RemoveContainer.
    }

    // PUBLIC_INTERFACE
    /** Return the container identifier. */
    const string& CRunContainer::Id() const
    {
        return _name;
    }

    // PUBLIC_INTERFACE
    /** Query the host PID, returning zero if status cannot be read. */
    uint32_t CRunContainer::Pid() const
    {
        libcrun_error_t error = nullptr;
        libcrun_container_status_t status {};
        const int result = libcrun_read_container_status(&status, _context.state_root, _name.c_str(), &error);
        if (result != 0) {
            TRACE_L1("Failed to get PID of container %s", _name.c_str());
            return 0;
        }
        const uint32_t pid = status.pid;
        libcrun_free_container_status(&status);
        return pid;
    }

    // PUBLIC_INTERFACE
    /** Return reference-counted cgroup memory metrics. */
    IMemoryInfo* CRunContainer::Memory() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.Memory();
    }

    // PUBLIC_INTERFACE
    /** Return reference-counted cgroup processor metrics. */
    IProcessorInfo* CRunContainer::ProcessorInfo() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.ProcessorInfo();
    }

    // PUBLIC_INTERFACE
    /** Return null because network-interface enumeration is not implemented. */
    INetworkInterfaceIterator* CRunContainer::NetworkInterfaces() const
    {
        return nullptr;
    }

    // PUBLIC_INTERFACE
    /** Query running state; failed status queries currently return false. */
    bool CRunContainer::IsRunning() const
    {
        bool result = false;
        // Initialize external output holders before a failed status query.
        libcrun_error_t error = nullptr;
        libcrun_container_status_t status {};
        if (libcrun_read_container_status(&status, _context.state_root, _name.c_str(), &error) != 0) {
            // TODO: Distinguish a missing container from other libcrun failures.
            result = false;
        } else {
            const int ret = libcrun_is_container_running(&status, &error);
            if (ret < 0) {
                TRACE_L1("Failed to acquire container %s state", _name.c_str());
            } else {
                result = (ret == 1);
            }
            libcrun_free_container_status(&status);
        }
        return result;
    }

    // PUBLIC_INTERFACE
    /**
     * Start command with parameters after validating the loaded OCI configuration.
     * Return false for load, validation, allocation, cleanup or runtime failures;
     * no runtime launch occurs with an invalid rootfs or incomplete argv.
     */
    bool CRunContainer::Start(const string& command, IStringIterator& parameters)
    {
        libcrun_error_t error = nullptr;
        bool result = false;
        // RAII releases the lock on every failure path, including C++ exceptions.
        Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
        if (ClearLeftovers() == Core::ERROR_NONE) {
            _container = libcrun_container_load_from_file(_configFile.c_str(), &error);

            // Check every loader-owned pointer before use, and preserve the
            // original rootfs allocation if realloc cannot expand it.
            // TODO: Relative mount paths still need bundle-aware treatment.
            if (Detail::PrepareRootfs(_container, _bundle) == false) {
                TRACE_L1("Invalid configuration or rootfs allocation failure in %s: %s",
                    _configFile.c_str(), error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
            } else if (OverwriteContainerArgs(_container, command, parameters) == false) {
                TRACE_L1("Failed to allocate command arguments for container %s", _name.c_str());
            } else {
                const int ret = libcrun_container_run(&_context, _container, LIBCRUN_RUN_OPTIONS_PREFORK, &error);
                if (ret != 0) {
                    TRACE_L1("Failed to run container %s. Error %d: %s", _name.c_str(),
                        error != nullptr ? error->status : ret,
                        error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
                } else {
                    _created = true;
                    result = true;
                }
            }
        }
        return result;
    }

    // PUBLIC_INTERFACE
    /** Force-delete the container; timeout is not implemented by this backend. */
    bool CRunContainer::Stop(const uint32_t timeout /*ms*/ VARIABLE_IS_NOT_USED)
    {
        bool result = true;
        libcrun_error_t error = nullptr;
        Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
        if (libcrun_container_delete(&_context, nullptr, _name.c_str(), true, &error) != 0) {
            // Failure reporting must not itself dereference absent metadata.
            TRACE_L1("Failed to destroy container %s: %s", _name.c_str(),
                error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
            result = false;
        } else {
            _created = false;
        }
        return result;
    }

    bool CRunContainer::OverwriteContainerArgs(libcrun_container_t* container,
        const string& newComand, IStringIterator& newParameters)
    {
        // Build the replacement before freeing any loader-owned arguments.
        const size_t count = newParameters.Count();
        if (newComand.empty() || (count > (SIZE_MAX / sizeof(char*)) - 2)) {
            return false;
        }
        char** argv = reinterpret_cast<char**>(calloc(count + 2, sizeof(char*)));
        if (argv == nullptr) {
            return false;
        }

        size_t argc = 0;
        argv[argc] = strdup(newComand.c_str());
        bool complete = (argv[argc] != nullptr);
        if (complete) {
            ++argc;
        }
        newParameters.Reset(0);
        while (complete && newParameters.Next()) {
            // Guard against an iterator yielding more entries than Count().
            if (argc >= count + 1) {
                complete = false;
            } else {
                const string parameter = newParameters.Current();
                argv[argc] = strdup(parameter.c_str());
                complete = (argv[argc] != nullptr);
                if (complete) {
                    ++argc;
                }
            }
        }
        if (complete == false) {
            for (size_t i = 0; i < argc; ++i) {
                free(argv[i]);
            }
            free(argv);
            return false;
        }

        // Commit only after every allocation succeeded; calloc supplies the
        // final null entry required by libcrun.
        if (container->container_def->process->args != nullptr) {
            for (size_t i = 0; i < container->container_def->process->args_len; ++i) {
                free(container->container_def->process->args[i]);
            }
            free(container->container_def->process->args);
        }
        container->container_def->process->args = argv;
        container->container_def->process->args_len = argc;
        return true;
    }

    uint32_t CRunContainer::ClearLeftovers()
    {
        uint32_t result = Core::ERROR_NONE;
        libcrun_error_t error = nullptr;
        libcrun_container_list_t* list = nullptr;
        const int ret = libcrun_get_containers_list(&list, _context.state_root, &error);
        if (ret < 0) {
            // Cleanup runs before the loader; guard its diagnostics as well.
            TRACE_L1("Failed to get containers list. Error %d: %s",
                error != nullptr ? error->status : ret,
                error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
            result = Core::ERROR_UNAVAILABLE;
        } else {
            for (libcrun_container_list_t* it = list; it != nullptr; it = it->next) {
                if ((it->name != nullptr) && (_name == it->name)) {
                    TRACE_L1("Found leftover container %s. Killing it", _name.c_str());
                    const int deleted = libcrun_container_delete(&_context, nullptr, it->name, true, &error);
                    if (deleted < 0) {
                        TRACE_L1("Failed to destroy container %s. Error %d: %s", _name.c_str(),
                            error != nullptr ? error->status : deleted,
                            error != nullptr && error->msg != nullptr ? error->msg : "No diagnostic");
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                    // Only one container can have this identifier.
                    break;
                }
            }
        }
        return result;
    }

    // Use the shared registration path so enabling CRUN actually selects it.
    static ContainerProducerRegistrationType<CRunContainerAdministrator, IContainer::containertype::CRUN> registration;

} // namespace ProcessContainers
} // namespace Thunder
