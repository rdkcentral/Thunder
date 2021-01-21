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

#include "Module.h"
#include "json/JsonData_Controller.h"
#include "Controller.h"


namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Controller;

    // Registration
    //

    void Controller::RegisterAll()
    {
        Register<ActivateParamsInfo,void>(_T("activate"), &Controller::endpoint_activate, this);
        Register<ActivateParamsInfo,void>(_T("deactivate"), &Controller::endpoint_deactivate, this);
        Register<StartdiscoveryParamsData,void>(_T("startdiscovery"), &Controller::endpoint_startdiscovery, this);
        Register<void,void>(_T("storeconfig"), &Controller::endpoint_storeconfig, this);
        Register<DeleteParamsData,void>(_T("delete"), &Controller::endpoint_delete, this);
        Register<void,void>(_T("harakiri"), &Controller::endpoint_harakiri, this);
        Property<Core::JSON::ArrayType<PluginHost::MetaData::Service>>(_T("status"), &Controller::get_status, nullptr, this);
        Property<Core::JSON::ArrayType<PluginHost::MetaData::Channel>>(_T("links"), &Controller::get_links, nullptr, this);
        Property<PluginHost::MetaData::Server>(_T("processinfo"), &Controller::get_processinfo, nullptr, this);
        Property<Core::JSON::ArrayType<SubsystemsParamsData>>(_T("subsystems"), &Controller::get_subsystems, nullptr, this);
        Property<Core::JSON::ArrayType<PluginHost::MetaData::Bridge>>(_T("discoveryresults"), &Controller::get_discoveryresults, nullptr, this);
        Property<Core::JSON::String>(_T("environment"), &Controller::get_environment, nullptr, this);
        Property<Core::JSON::String>(_T("configuration"), &Controller::get_configuration, &Controller::set_configuration, this);
        Register<CloneParamsInfo,Core::JSON::String>(_T("clone"), &Controller::endpoint_clone, this);
    }

    void Controller::UnregisterAll()
    {
        Unregister(_T("harakiri"));
        Unregister(_T("delete"));
        Unregister(_T("storeconfig"));
        Unregister(_T("startdiscovery"));
        Unregister(_T("deactivate"));
        Unregister(_T("activate"));
        Unregister(_T("configuration"));
        Unregister(_T("environment"));
        Unregister(_T("discoveryresults"));
        Unregister(_T("subsystems"));
        Unregister(_T("processinfo"));
        Unregister(_T("links"));
        Unregister(_T("status"));
        Unregister(_T("clone"));
    }

    // API implementation
    //

    // Method: activate - Activates a plugin
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_PENDING_CONDITIONS: The plugin will be activated once its activation preconditions are met
    //  - ERROR_INPROGRESS: The plugin is currently being activated
    //  - ERROR_UNKNOWN_KEY: The plugin does not exist
    //  - ERROR_OPENING_FAILED: Failed to activate the plugin
    //  - ERROR_ILLEGAL_STATE: Current state of the plugin does not allow activation
    //  - ERROR_PRIVILEGED_REQUEST: Activation of the plugin is not allowed (e.g. Controller)
    uint32_t Controller::endpoint_activate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;
        const string& callsign = params.Callsign.Value();

        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::Server::Service> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                result = service->Activate(PluginHost::IShell::REQUESTED);

                // Normalise return code
                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ILLEGAL_STATE) && (result !=  Core::ERROR_INPROGRESS) && (result != Core::ERROR_PENDING_CONDITIONS)) {
                    result = Core::ERROR_OPENING_FAILED;
                }
            }
            else {
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else {
            result = Core::ERROR_PRIVILIGED_REQUEST;
        }

        return result;
    }

    // Method: deactivate - Deactivates a plugin
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: The plugin is currently being deactivated
    //  - ERROR_UNKNOWN_KEY: The plugin does not exist
    //  - ERROR_ILLEGAL_STATE: Current state of the plugin does not allow deactivation
    //  - ERROR_CLOSING_FAILED: Failed to activate the plugin
    //  - ERROR_PRIVILEGED_REQUEST: Deactivation of the plugin is not allowed (e.g. Controller)
    uint32_t Controller::endpoint_deactivate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;
        const string& callsign = params.Callsign.Value();

        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::Server::Service> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                result = service->Deactivate(PluginHost::IShell::REQUESTED);

                // Normalise return code
                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ILLEGAL_STATE) && (result !=  Core::ERROR_INPROGRESS)) {
                    result = Core::ERROR_CLOSING_FAILED;
                }
            }
            else {
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }
        else {
            result = Core::ERROR_PRIVILIGED_REQUEST;
        }

        return result;
    }

    // Starts the network discovery.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::endpoint_startdiscovery(const JsonData::Controller::StartdiscoveryParamsData& params)
    {
        const uint8_t& ttl = params.Ttl.Value();

        ASSERT(_probe != nullptr);
        _probe->Ping(ttl);

        return Core::ERROR_NONE;
    }

    // Method: storeconfig - Stores the configuration
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to store the configuration
    uint32_t Controller::endpoint_storeconfig()
    {
        ASSERT(_pluginServer != nullptr);

        uint32_t result = _pluginServer->Services().Persist();

        // Normalise return code
        if (result != Core::ERROR_NONE) {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    // Method: delete - Removes contents of a directory from the persistent storage
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The given path was incorrect
    //  - ERROR_DESTRUCTION_FAILED : Failed to delete given path
    //  - ERROR_PRIVILEGED_REQUEST: The path points outside of persistent directory or some files/directories couldn't have been deleted
    uint32_t Controller::endpoint_delete(const DeleteParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& path = params.Path.Value();

        if (path.empty() == false) {
            if (path.find("..") == string::npos) {
                ASSERT(_service != nullptr);
                Core::File file(_service->PersistentPath() +  path);

                if (file.Exists() == true) {
                    if (file.IsDirectory() == true) {
                        result = (Core::Directory((_service->PersistentPath() +  path).c_str()).Destroy(true) == true) ? Core::ERROR_NONE : Core::ERROR_DESTRUCTION_FAILED;
                    } else {
                        result = (file.Destroy() == true) ? Core::ERROR_NONE : Core::ERROR_DESTRUCTION_FAILED;
                    }
                } else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }
            }
            else {
                result = Core::ERROR_PRIVILIGED_REQUEST;
            }
        }

        return result;
    }

    // Method: harakiri - Reboots the device
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Rebooting procedure is not available on the device
    //  - ERROR_PRIVILEGED_REQUEST: Insufficient privileges to reboot the device
    //  - ERROR_GENERAL: Failed to reboot the device
    uint32_t Controller::endpoint_harakiri()
    {
        uint32_t result =  Core::System::Reboot();

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_UNAVAILABLE) && (result != Core::ERROR_PRIVILIGED_REQUEST) && (result != Core::ERROR_GENERAL)) {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }


    // Method: clone - Clone a plugin
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The original plugin does not exist
    //  - ERROR_PRIVILEGED_REQUEST: Cloning of the plugin is not allowed (e.g. Controller)
    //  - ERROR_UNAVAILABLE: Failed to clone plugin configuration
    uint32_t Controller::endpoint_clone(const CloneParamsInfo& params, Core::JSON::String& response)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& baseCallsign = params.Callsign.Value();
        const string& newCallsign = params.NewCallsign.Value();
        const string controllerName = _pluginServer->Controller()->Callsign();

        ASSERT(_pluginServer != nullptr);

        if ((baseCallsign.empty() == false) && (baseCallsign.empty() == false) && (baseCallsign != controllerName) && (newCallsign != controllerName)) {
            Core::ProxyType<PluginHost::Server::Service> baseService, newService;

            if (_pluginServer->Services().FromIdentifier(baseCallsign, baseService) != Core::ERROR_NONE) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else if (_pluginServer->Services().FromIdentifier(newCallsign, newService) != Core::ERROR_NONE) {
                result = _pluginServer->Services().Clone(baseService, newCallsign, newService);
            }
            else if (baseService->ClassName() != newService->ClassName()) {
                result = Core::ERROR_GENERAL;
            }

            if (result == Core::ERROR_NONE) {
                response = newService->Callsign();
            }
        }
        else {
            result = Core::ERROR_PRIVILIGED_REQUEST;
        }

        return result;
    }

    // Property: status - Information about plugins, including their configurations
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The service does not exist
    uint32_t Controller::get_status(const string& index, Core::JSON::ArrayType<PluginHost::MetaData::Service>& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (index.empty() == true) {
            _pluginServer->Services().GetMetaData(response);
            result = Core::ERROR_NONE;
        }
        else {
            if (_pluginServer->Services().FromIdentifier(index, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());

                PluginHost::MetaData::Service status;
                service->GetMetaData(status);

                response.Add(status);

                result = Core::ERROR_NONE;
            }
        }

        return result;
    }

    // Property: links - Information about active connections
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::get_links(Core::JSON::ArrayType<PluginHost::MetaData::Channel>& response) const
    {
        ASSERT(_pluginServer != nullptr);

        _pluginServer->Dispatcher().GetMetaData(response);

        return Core::ERROR_NONE;
    }

    // Property: processinfo - Information about the framework process
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::get_processinfo(PluginHost::MetaData::Server& response) const
    {
        WorkerPoolMetaData(response);

        return Core::ERROR_NONE;
    }

    // Property: subsystems - Status of subsystems
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::get_subsystems(Core::JSON::ArrayType<SubsystemsParamsData>& response) const
    {
        ASSERT(_service != nullptr);
        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        if (subSystem != nullptr) {
            uint8_t i = 0;
            while (i < PluginHost::ISubSystem::END_LIST) {
                PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(i));
                SubsystemsParamsData status;
                status.Subsystem = current;
                status.Active = subSystem->IsActive(current);
                response.Add(status);
                ++i;
            }
            subSystem->Release();
        }

        return Core::ERROR_NONE;
    }

    // Property: discoveryresults - SSDP network discovery results
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::get_discoveryresults(Core::JSON::ArrayType<PluginHost::MetaData::Bridge>& response) const
    {
        ASSERT(_probe != nullptr);
        Probe::Iterator index(_probe->Instances());

        while (index.Next() == true) {
            PluginHost::MetaData::Bridge element((*index).URL().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure());
            response.Add(element);
        }

        return Core::ERROR_NONE;
    }

    // Property: environment - Value of an environment variable
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The variable does not exist
    uint32_t Controller::get_environment(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        string value;

        if (Core::SystemInfo::GetEnvironment(index, value) == true) {
            response = value;
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Property: configuration - Configuration object of a service
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The service does not exist
    uint32_t Controller::get_configuration(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(index, service) == Core::ERROR_NONE) {
            response.SetQuoted(false);
            response = service->ConfigLine();
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Property: configuration - Configuration object of a service
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The service does not exist
    //  - ERROR_GENERAL: Failed to update the configuration
    uint32_t Controller::set_configuration(const string& index, const Core::JSON::String& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(index, service) == Core::ERROR_NONE) {
            const string& configuration = params.Value();

            result = service->ConfigLine(configuration);

            // Normalise return code
            if (result != Core::ERROR_NONE) {
                result = Core::ERROR_GENERAL;
            }
        }

        return result;
    }

    // Event: statechange - Signals a plugin state change
    void Controller::event_statechange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        StatechangeParamsData params;
        params.Callsign = callsign;
        params.State = state;
        params.Reason = reason;

        Notify(_T("statechange"), params);
    }

    // Note: event_all and event_subsytemchange are handled internally within the Controller

} // namespace Plugin

}

