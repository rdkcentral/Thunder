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
#include "JSON.h"
#include <thread>

namespace WPEFramework {
namespace ProcessContainers {
    // Container administrator
    // ----------------------------------
    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static CRunContainerAdministrator& cRunContainerAdministrator = Core::SingletonType<CRunContainerAdministrator>::Instance();

        return cRunContainerAdministrator;
    }

    IContainer* CRunContainerAdministrator::Container(const string& id, IStringIterator& searchpaths, const string& logpath, const string& configuration)
    {
        searchpaths.Reset(0);
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            Core::File configFile(path + "/Container/config.json");

            if (configFile.Exists()) {
                this->InternalLock();
                CRunContainer* container = new CRunContainer(id, path + "/Container", logpath);
                InsertContainer(container);
                this->InternalUnlock();

                return container;
            }
        }

        return nullptr;
    }

    CRunContainerAdministrator::CRunContainerAdministrator()
        : BaseContainerAdministrator()
    {
    }

    CRunContainerAdministrator::~CRunContainerAdministrator()
    {
    }

    void CRunContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        // Only container-scope logging
    }

    // Container
    // ------------------------------------
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

        if (logPath.empty() == false) {
            Core::Directory(logPath.c_str()).CreatePath();

            libcrun_error_t error = nullptr;
            int ret = init_logging(&_context.output_handler, &_context.output_handler_arg, _name.c_str(), (logPath + "/container.log").c_str(), &error);
            if (ret != 0) {
                TRACE_L1("Cannot initialize logging of container \"%s\" in directory %s\n. Error %d: %s", _name.c_str(), logPath.c_str(), error->status, error->msg);
            }
        }
    }

    CRunContainer::~CRunContainer()
    {
        if (_created == true) {
            Stop(Core::infinite);
        }

        auto& administrator = static_cast<CRunContainerAdministrator&>(CRunContainerAdministrator::Instance());
        administrator.RemoveContainer(this);
    }

    const string& CRunContainer::Id() const
    {
        return _name;
    }

    uint32_t CRunContainer::Pid() const
    {
        uint32_t result = 0;
        libcrun_error_t error = nullptr;
        libcrun_container_status_t status;
        status.pid = 0;

        result = libcrun_read_container_status(&status, _context.state_root, _name.c_str(), &error);
        if (result != 0) {
            TRACE_L1("Failed to get PID of container %s", _name.c_str());
        }

        return status.pid;
    }

    IMemoryInfo* CRunContainer::Memory() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.Memory();
    }

    IProcessorInfo* CRunContainer::ProcessorInfo() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.ProcessorInfo();
    }

    INetworkInterfaceIterator* CRunContainer::NetworkInterfaces() const
    {
        return nullptr;
    }

    bool CRunContainer::IsRunning() const
    {
        bool result = true;
        libcrun_error_t error;
        libcrun_container_status_t status;

        if (libcrun_read_container_status(&status, _context.state_root, _name.c_str(), &error) != 0) {
            // This most likely occurs when container is not found
            // TODO: Find another way to make sure that something else did not cause this error
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

        _adminLock.Lock();
        // Make sure no leftover container instances are present
        if (ClearLeftovers() != Core::ERROR_NONE) {
            result = false;
        } else {
            _container = libcrun_container_load_from_file(_configFile.c_str(), &error);

            // Add bundle prefix to rootfs location if relative path is provided
            // TODO: Possibly change mount relative path to acomodate for bundle or check if doing chdir() is safe alternative
            string rootfsPath = _bundle + "/";
            if (_container->container_def->root->path != nullptr && _container->container_def->root->path[0] != '/') {
                rootfsPath += _container->container_def->root->path;
                _container->container_def->root->path = reinterpret_cast<char*>(realloc(_container->container_def->root->path, (rootfsPath.length() + 1) * sizeof(char)));

                strcpy(_container->container_def->root->path, rootfsPath.c_str());
            }

            if (_container == NULL) {

                TRACE_L1("Failed to load a configuration file in %s. Error %d: %s", _configFile.c_str(), error->status, error->msg);
                result = false;
            } else {

                OverwriteContainerArgs(_container, command, parameters);

                ret = libcrun_container_run(&_context, _container, LIBCRUN_RUN_OPTIONS_PREFORK, &error);
                if (ret != 0) {
                    TRACE_L1("Failed to run a container \"%s\". Error %d: %s", _name.c_str(), error->status, error->msg);
                    result = false;
                } else {
                    _created = true;
                }
            }
        }
        _adminLock.Unlock();

        return result;
    }

    bool CRunContainer::Stop(const uint32_t timeout /*ms*/)
    {
        bool result = true;
        libcrun_error_t error = nullptr;

        _adminLock.Lock();
        if (libcrun_container_delete(&_context, NULL, _name.c_str(), true, &error) != 0) {
            TRACE_L1("Failed to destroy a container \"%s\". Error: %s", _name.c_str(), error->msg);
            result = false;
        } else {
            _created = false;
        }
       _adminLock.Unlock();

        return result;
    }

    void CRunContainer::OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters)
    {
        // Clear args that were set by runtime
        if (container->container_def->process->args) {
            for (size_t i = 0; i < container->container_def->process->args_len; i++) {
                if (container->container_def->process->args[i] != NULL) {
                    free(container->container_def->process->args[i]);
                }
            }
            free(container->container_def->process->args);
        }

        char** argv = reinterpret_cast<char**>(malloc((newParameters.Count() + 2) * sizeof(char*)));
        int argc = 0;

        // Set custom starting command
        argv[argc] = reinterpret_cast<char*>(malloc(sizeof(char) * newComand.length() + 1));
        strcpy(argv[argc], newComand.c_str());
        argc++;

        // Set command arguments
        while (newParameters.Next()) {
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
        uint32_t result = Core::ERROR_NONE;
        libcrun_error_t error = nullptr;
        int ret = 0;

        // Find containers
        libcrun_container_list_t* list;
        ret = libcrun_get_containers_list(&list, "/run/crun", &error);
        if (ret < 0) {
            TRACE_L1("Failed to get containers list. Error %d: %s", error->status, error->msg);
            result = Core::ERROR_UNAVAILABLE;
        } else {
            for (libcrun_container_list_t* it = list; it != nullptr; it = it->next) {
                if (_name == it->name) {
                    TRACE_L1("Found container %s already created (maybe leftover from another Thunder launch). Killing it!", _name.c_str());
                    ret = libcrun_container_delete(&_context, nullptr, it->name, true, &error);

                    if (ret < 0) {
                        TRACE_L1("Failed to destroy a container %s. Error %d: %s", _name.c_str(), error->status, error->msg);
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                    // its only possible to find one leftover. No sense looking for more...
                    break;
                }
            }
        }

        return result;
    }

} // namespace ProcessContainers

} // namespace WPEFramework
