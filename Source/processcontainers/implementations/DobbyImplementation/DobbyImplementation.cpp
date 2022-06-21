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

#include "DobbyImplementation.h"
#include <Dobby/DobbyProxy.h>
#include <Dobby/IpcService/IpcFactory.h>
#include <fstream>
#include <thread>
#include <json/value.h>

namespace WPEFramework {

namespace ProcessContainers {
    // Container administrator
    // ----------------------------------
    IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
    {
        static DobbyContainerAdministrator& dobbyContainerAdministrator = Core::SingletonType<DobbyContainerAdministrator>::Instance();

        return dobbyContainerAdministrator;
    }

    IContainer* DobbyContainerAdministrator::Container(const string& id, IStringIterator& searchpaths, const string& logpath, const string& configuration)
    {
        searchpaths.Reset(0);
        while (searchpaths.Next()) {
            auto path = searchpaths.Current();

            auto createContainer = [&](bool useSpecFile, const string &path) -> IContainer* {
                // Make sure no leftover will interfere...
                if (ContainerNameTaken(id)) {
                    DestroyContainer(id);
                }
                this->InternalLock();
                DobbyContainer* container = new DobbyContainer(id, path, logpath, useSpecFile);
                InsertContainer(container);
                this->InternalUnlock();

                return container;
            };

            Core::File configBundleFile(path + "/Container" + CONFIG_NAME);
            TRACE(ProcessContainers::ProcessContainerization, (_T("searching %s container at %s, %s"), id.c_str(), configBundleFile.Name().c_str(), configBundleFile.PathName().c_str()));
            if (configBundleFile.Exists()) {
                TRACE(ProcessContainers::ProcessContainerization, (_T("Found %s container!"), id.c_str()));
                return createContainer(false, configBundleFile.PathName());
            }

            Core::File configSpecFile(path + CONFIG_NAME_SPEC);
            TRACE(ProcessContainers::ProcessContainerization, (_T("searching %s container at %s"), id.c_str(), configSpecFile.Name().c_str(), configSpecFile.Name().c_str()));
            if (configSpecFile.Exists()) {
                TRACE(ProcessContainers::ProcessContainerization, (_T("Found %s container!"), id.c_str()));
                return createContainer(true, configSpecFile.Name());
            }
        }

        TRACE(Trace::Error, (_T("Could not find suitable container config for %s in any search path"), id.c_str()));

        return nullptr;
    }

    DobbyContainerAdministrator::DobbyContainerAdministrator()
        : BaseContainerAdministrator()
    {
        mIpcService = AI_IPC::createIpcService("unix:path=/var/run/dbus/system_bus_socket", "org.rdk.dobby.processcontainers");

        if (!mIpcService) {
            TRACE(Trace::Error, (_T("Failed to create Dobby IPC service")));
            return;
        } else {
            // Start the IPCService which kicks off the event dispatcher thread
            mIpcService->start();
        }

        // Create a DobbyProxy remote service that wraps up the dbus API
        // calls to the Dobby daemon
        mDobbyProxy = std::make_shared<DobbyProxy>(mIpcService, DOBBY_SERVICE, DOBBY_OBJECT);
    }

    DobbyContainerAdministrator::~DobbyContainerAdministrator()
    {
    }

    void DobbyContainerAdministrator::Logging(const string& logPath, const string& loggingOptions)
    {
        // Only container-scope logging
    }

    void DobbyContainerAdministrator::DestroyContainer(const string& name)
    {
        // Get the running containers from Dobby
        std::list<std::pair<int32_t, std::string>> runningContainers;
        runningContainers = mDobbyProxy->listContainers();

        if (!runningContainers.empty()) {
            for (const std::pair<int32_t, std::string>& c : runningContainers) {
                if (c.second == name) {
                    // found the container, now try stopping it...
                    TRACE(ProcessContainers::ProcessContainerization, (_T("destroying container: %s "), name.c_str()));

                    // Dobby stop is async - block until we get the notification the container
                    // has actually stopped
                    this->InternalLock();

                    _stopPromise = std::promise<void>();
                    const void* vp = static_cast<void*>(new std::string(name));
                    int listenerId = mDobbyProxy->registerListener(std::bind(&DobbyContainerAdministrator::containerStopCallback, this,
                                                                       std::placeholders::_1,
                                                                       std::placeholders::_2,
                                                                       std::placeholders::_3,
                                                                       std::placeholders::_4), vp);

                    std::future<void> future = _stopPromise.get_future();
                    bool stoppedSuccessfully = mDobbyProxy->stopContainer(c.first, true);
                    if (!stoppedSuccessfully) {
                        TRACE(Trace::Warning, (_T("Failed to destroy container, internal Dobby error. id: %s descriptor: %d"), name.c_str(), c.first));
                    } else {
                        // Block here until container has stopped (max 5 seconds)
                        std::future_status status = future.wait_for(std::chrono::seconds(5));
                        if (status == std::future_status::ready) {
                            TRACE(ProcessContainers::ProcessContainerization, (_T("Container %s has stopped"), name.c_str()));
                        } else if (status == std::future_status::timeout) {
                            TRACE(Trace::Warning, (_T("Timeout waiting for container %s to stop"), name.c_str()));
                        }
                    }

                    this->InternalUnlock();

                    // Always make sure we unregister our callback
                    mDobbyProxy->unregisterListener(listenerId);

                    break;
                }
            }
        }
    }

    bool DobbyContainerAdministrator::ContainerNameTaken(const string& name)
    {
        bool result = false;

        // Get the running containers from Dobby
        std::list<std::pair<int32_t, std::string>> runningContainers;
        runningContainers = mDobbyProxy->listContainers();

        // Build the response if containers were found
        if (!runningContainers.empty()) {
            for (const std::pair<int32_t, std::string>& c : runningContainers) {
                if (c.second == name) {
                    TRACE(ProcessContainers::ProcessContainerization, (_T("container %s already running..."), name.c_str()));
                    result = true;
                    break;
                }
            }
        }

        return result;
    }

    void DobbyContainerAdministrator::containerStopCallback(int32_t cd, const std::string& containerId,
        IDobbyProxyEvents::ContainerState state,
        const void* params)
    {
        const std::string* id = static_cast<const std::string*>(params);

        // Interested in stop events only
        if (state == IDobbyProxyEvents::ContainerState::Stopped && containerId == *id) {
            _stopPromise.set_value();
        }
    }

    // Container
    // ------------------------------------
    DobbyContainer::DobbyContainer(const string& name, const string& path, const string& logPath, bool useSpecFile)
        : _adminLock()
        , _name(name)
        , _path(path)
        , _logPath(logPath)
        , _pid()
        , _useSpecFile(useSpecFile)
    {
    }

    DobbyContainer::~DobbyContainer()
    {
        auto& admin = static_cast<DobbyContainerAdministrator&>(DobbyContainerAdministrator::Instance());

        if (admin.ContainerNameTaken(_name) == true) {
            Stop(Core::infinite);
        }

        admin.RemoveContainer(this);
    }

    const string& DobbyContainer::Id() const
    {
        return _name;
    }

    uint32_t DobbyContainer::Pid() const
    {
        uint32_t returnedPid = 0;
        auto& admin = static_cast<DobbyContainerAdministrator&>(DobbyContainerAdministrator::Instance());

        if (_pid.IsSet() == false) {
            std::string containerInfoString = admin.mDobbyProxy->getContainerInfo(_descriptor);

            if (containerInfoString.empty()) {
                TRACE(Trace::Warning, (_T("Failed to get info for container %s"), _name.c_str()));
            } else {
                // Dobby returns the container info as JSON, so parse it
                JsonObject containerInfoJson;
                WPEFramework::Core::OptionalType<WPEFramework::Core::JSON::Error> error;
                if (!WPEFramework::Core::JSON::IElement::FromString(containerInfoString, containerInfoJson, error)) {
                    TRACE(Trace::Warning, (_T("Failed to parse Dobby container info JSON due to: %s"), WPEFramework::Core::JSON::ErrorDisplayMessage(error).c_str()));
                } else {
                    JsonArray pids = containerInfoJson["pids"].Array();

                    // In Dobby containers, DobbyInit is always the parent process of
                    // the container

                    // If we're running a plugin in container, the plugin should run
                    // under WPEProcess. Return the WPEProcess PID if available to
                    // ensure consistency with non-containerised oop plugins
                    if (pids.Length() > 0) {
                        if (pids.Length() == 1) {
                            returnedPid = pids[0].Number();
                            _pid = returnedPid;
                        } else {
                            JsonArray processes = containerInfoJson["processes"].Array();
                            JsonArray::Iterator index(processes.Elements());

                            uint32_t dobbyInitPid = 0;
                            uint32_t wpeProcessPid = 0;

                            while (index.Next()) {
                                if (Core::JSON::Variant::type::OBJECT == index.Current().Content()) {
                                    JsonObject process = index.Current().Object();

                                    uint32_t pid = process["pid"].Number();
                                    uint32_t nsPid = process["nspid"].Number();

                                    if (nsPid == 1) {
                                        dobbyInitPid = pid;
                                        continue;
                                    }

                                    string executable = process["executable"].String();
                                    if (executable.find("WPEProcess") != std::string::npos) {
                                        wpeProcessPid = pid;
                                        break;
                                    }
                                }
                            }

                            if (wpeProcessPid == 0 && dobbyInitPid > 0) {
                                // We didn't find WPEProcess, just return DobbyInit
                                returnedPid = dobbyInitPid;
                                _pid = returnedPid;
                            } else if (wpeProcessPid > 0) {
                                // Found WPEProcess, return its PID
                                returnedPid = wpeProcessPid;
                                _pid = returnedPid;
                            } else {
                                // Unable to determine the PID for some reason
                                // Just return the first PID in the pids array
                                // which 99% of the time is DobbyInit
                                returnedPid = pids[0].Number();
                                _pid = returnedPid;
                            }
                        }
                    }
                }
            }
        } else {
            // Value was already read before, use this value
            returnedPid = _pid;
        }
        return returnedPid;
    }

    IMemoryInfo* DobbyContainer::Memory() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.Memory();
    }

    IProcessorInfo* DobbyContainer::ProcessorInfo() const
    {
        CGroupMetrics containerMetrics(_name);
        return containerMetrics.ProcessorInfo();
    }

    INetworkInterfaceIterator* DobbyContainer::NetworkInterfaces() const
    {
        return nullptr;
    }

    bool DobbyContainer::IsRunning() const
    {
        bool result = false;
        auto& admin = static_cast<DobbyContainerAdministrator&>(DobbyContainerAdministrator::Instance());

        // We got a state back successfully, work out what that means in English
        switch (static_cast<IDobbyProxyEvents::ContainerState>(admin.mDobbyProxy->getContainerState(_descriptor))) {
        case IDobbyProxyEvents::ContainerState::Invalid:
            result = false;
            break;
        case IDobbyProxyEvents::ContainerState::Starting:
            result = false;
            break;
        case IDobbyProxyEvents::ContainerState::Running:
            result = true;
            break;
        case IDobbyProxyEvents::ContainerState::Stopping:
            result = true;
            break;
        case IDobbyProxyEvents::ContainerState::Paused:
            result = true;
            break;
        case IDobbyProxyEvents::ContainerState::Stopped:
            result = false;
            break;
        default:
            result = false;
            break;
        }

        return result;
    }

    bool DobbyContainer::Start(const string& command, IStringIterator& parameters)
    {
        bool result = false;

        _adminLock.Lock();
        auto& admin = static_cast<DobbyContainerAdministrator&>(DobbyContainerAdministrator::Instance());

        std::list<int> emptyList;

        // construct the full command to run with all the arguments
        std::string fullCommand = "/usr/bin/" + command;
        while (parameters.Next()) {
            fullCommand += " " + parameters.Current();
        }

        if (_useSpecFile) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("path set to %s for name %s"), _path.c_str(), _name.c_str()));

            std::ifstream jsonSpecFile(_path);

            std::ostringstream specFileStream;
            specFileStream << jsonSpecFile.rdbuf();

            // convert ostringstream to std::string
            std::string containerSpecString = specFileStream.str();
            TRACE(ProcessContainers::ProcessContainerization, (_T("container spec string: %s"), containerSpecString.c_str()));

            _descriptor = admin.mDobbyProxy->startContainerFromSpec(_name, containerSpecString , emptyList, fullCommand);
        } else {
            _descriptor = admin.mDobbyProxy->startContainerFromBundle(_name, _path, emptyList, fullCommand);
        }
        // startContainer returns -1 on failure
        if (_descriptor <= 0) {
            TRACE(Trace::Error, (_T("Failed to start container %s - internal Dobby error."), _name.c_str()));
            result = false;
        } else {
            TRACE(ProcessContainers::ProcessContainerization, (_T("started %s container! descriptor: %d"), _name.c_str(), _descriptor));
            result = true;
        }
        _adminLock.UnLock();

        return result;
    }

    bool DobbyContainer::Stop(const uint32_t timeout /*ms*/)
    {
        // TODO: add timeout support
        bool result = false;

        _adminLock.Lock();
        auto& admin = static_cast<DobbyContainerAdministrator&>(DobbyContainerAdministrator::Instance());

        bool stoppedSuccessfully = admin.mDobbyProxy->stopContainer(_descriptor, false);

        if (!stoppedSuccessfully) {
            TRACE(Trace::Error, (_T("Failed to stop container, internal Dobby error. id: %s descriptor: %d"), _name.c_str(), _descriptor));
        } else {
            result = true;
        }
        _adminLock.Unlock();

        return result;
    }

} // namespace ProcessContainers

} // namespace WPEFramework
