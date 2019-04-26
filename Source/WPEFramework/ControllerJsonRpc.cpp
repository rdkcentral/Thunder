
#include "json/JsonData_Controller.h"
#include "Controller.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Controller;

    // Registration
    //

    void Controller::RegisterAll()
    {
        Register<ActivateParamsInfo,void>(_T("activate"), &Controller::endpoint_activate, this);
        Register<ActivateParamsInfo,void>(_T("deactivate"), &Controller::endpoint_deactivate, this);
        Register<ExistsParamsData,Core::JSON::DecUInt32>(_T("exists"), &Controller::endpoint_exists, this);
        Register<ActivateParamsInfo,Core::JSON::ArrayType<PluginHost::MetaData::Service>>(_T("status"), &Controller::endpoint_status, this);
        Register<void,Core::JSON::ArrayType<PluginHost::MetaData::Channel>>(_T("links"), &Controller::endpoint_links, this);
        Register<void,PluginHost::MetaData::Server>(_T("process"), &Controller::endpoint_process, this);
        Register<void,Core::JSON::ArrayType<SubsystemsResultData>>(_T("subsystems"), &Controller::endpoint_subsystems, this);
        Register<StartdiscoveryParamsData,void>(_T("startdiscovery"), &Controller::endpoint_startdiscovery, this);
        Register<void,Core::JSON::ArrayType<PluginHost::MetaData::Bridge>>(_T("discovery"), &Controller::endpoint_discovery, this);
        Register<GetenvParamsData,Core::JSON::String>(_T("getenv"), &Controller::endpoint_getenv, this);
        Register<GetconfigParamsData,Core::JSON::String>(_T("getconfig"), &Controller::endpoint_getconfig, this);
        Register<SetconfigParamsData,void>(_T("setconfig"), &Controller::endpoint_setconfig, this);
        Register<void,void>(_T("storeconfig"), &Controller::endpoint_storeconfig, this);
        Register<Download,void>(_T("download"), &Controller::endpoint_download, this);
        Register<DeleteParamsData,void>(_T("delete"), &Controller::endpoint_delete, this);
        Register<void,void>(_T("harakiri"), &Controller::endpoint_harakiri, this);
    }

    void Controller::UnregisterAll()
    {
        Unregister(_T("harakiri"));
        Unregister(_T("delete"));
        Unregister(_T("download"));
        Unregister(_T("storeconfig"));
        Unregister(_T("setconfig"));
        Unregister(_T("getconfig"));
        Unregister(_T("getenv"));
        Unregister(_T("discovery"));
        Unregister(_T("startdiscovery"));
        Unregister(_T("subsystems"));
        Unregister(_T("process"));
        Unregister(_T("links"));
        Unregister(_T("status"));
        Unregister(_T("exists"));
        Unregister(_T("deactivate"));
        Unregister(_T("activate"));
    }

    // API implementation
    //

    // Activates a plugin.
    // Return codes:
    //  - ERROR_NONE: Success
    //    ERROR_PENDING_CONDITIONS: The plugin will activate once its preconditions are met
    //  - ERROR_INPROGRESS: The plugin is currently being activated
    //  - ERROR_UNKNOWN_KEY: The plugin does not exist
    //  - ERROR_OPENING_FAILED: Failed to activate the plugin
    //  - ERROR_ILLEGAL_STATE: Current state of the plugin does not allow activation
    //  - ERROR_PRIVILEGED_REQUEST: Activation of the plugin is not allowed
    uint32_t Controller::endpoint_activate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;
        const string& callsign = params.Callsign.Value();

        ASSERT(_pluginServer != nullptr);

        if (callsign != _pluginServer->ControllerName()) {
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

    // Deactivates a plugin.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: The plugin is currently being deactivated
    //  - ERROR_UNKNOWN_KEY: The plugin does not exist
    //  - ERROR_ILLEGAL_STATE: Current state of the plugin does not allow deactivation
    //    ERROR_CLOSING_FAILED: Failed to deactivate the plugin
    //  - ERROR_PRIVILEGED_REQUEST: Deactivation of the plugin is not allowed
    uint32_t Controller::endpoint_deactivate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_OPENING_FAILED;
        const string& callsign = params.Callsign.Value();

        ASSERT(_pluginServer != nullptr);

        if (callsign != _pluginServer->ControllerName()) {
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

    uint32_t Controller::endpoint_exists(const JsonData::Controller::ExistsParamsData& params, Core::JSON::DecUInt32& response)
    {
        const string& designator = params.Designator.Value();
        Core::ProxyType<PluginHost::Server::Service> service;
        string callsign = Core::JSONRPC::Message::Callsign(designator);
        response = Core::ERROR_UNKNOWN_KEY;

        if (callsign.empty() == true) {
            if (Exists(designator, Core::JSONRPC::Message::Version(designator)) == Core::ERROR_NONE) {
                response = Core::ERROR_NONE;
            }
        }
        else {
            ASSERT(_pluginServer != nullptr);

            uint32_t result = _pluginServer->Services().FromIdentifier(callsign, service);

            if (result == Core::ERROR_NONE) {
                if (service->State() != PluginHost::IShell::ACTIVATED) {
                    response = Core::ERROR_UNAVAILABLE;
                }
                else {
                    ASSERT(service.IsValid());
                    PluginHost::IDispatcher* plugin = service->Dispatcher();

                    if (plugin != nullptr) {
                        if (plugin->Exists(Core::JSONRPC::Message::Method(designator), Core::JSONRPC::Message::Version(designator)) == Core::ERROR_NONE) {
                            response = Core::ERROR_NONE;
                        }
                    }
                }
            }
            else {
                response = result;
            }
        }

        return Core::ERROR_NONE;
    }

    // Retrieves information about plugins.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The plugin does not exist
    uint32_t Controller::endpoint_status(const ActivateParamsInfo& params, Core::JSON::ArrayType<PluginHost::MetaData::Service>& response)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& callsign = params.Callsign.Value();
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (callsign.empty() == true) {
            _pluginServer->Services().GetMetaData(response);
        }
        else {
            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());

                PluginHost::MetaData::Service status;
                service->GetMetaData(status);

                response.Add(status);

                result = Core::ERROR_NONE;
            }
        }

        return result;
    }

    // Retrieves information about active connections.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::endpoint_links(Core::JSON::ArrayType<PluginHost::MetaData::Channel>& response)
    {
        ASSERT(_pluginServer != nullptr);

        _pluginServer->Dispatcher().GetMetaData(response);

        return Core::ERROR_NONE;
    }

    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::endpoint_process(PluginHost::MetaData::Server& response)
    {
        PluginHost::WorkerPool::Instance().GetMetaData(response);

        return Core::ERROR_NONE;
    }

    // Retrieves status of subsystems.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::endpoint_subsystems(Core::JSON::ArrayType<SubsystemsResultData>& response)
    {
        ASSERT(_service != nullptr);
        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        uint8_t index(0);
        if (subSystem != nullptr) {
            while (index < PluginHost::ISubSystem::END_LIST) {
                PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(index));
                SubsystemsResultData status;
                status.Subsystem = current;
                status.Active = subSystem->IsActive(current);
                response.Add(status);
                ++index;
            }
            subSystem->Release();
        }

        return Core::ERROR_NONE;
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

    // Retrieves network discovery results.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Controller::endpoint_discovery(Core::JSON::ArrayType<PluginHost::MetaData::Bridge>& response)
    {
        ASSERT(_probe != nullptr);
        Probe::Iterator index(_probe->Instances());

        while (index.Next() == true) {
            PluginHost::MetaData::Bridge element((*index).URL().Text().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure());
            response.Add(element);
        }

        return Core::ERROR_NONE;
    }

    // Retrieves the value of an environment variable.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The given variable is not defined
    uint32_t Controller::endpoint_getenv(const GetenvParamsData& params, Core::JSON::String& response)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& variable = params.Variable.Value();
        string value;

        if (Core::SystemInfo::GetEnvironment(variable, value) == true) {
            response = value;
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Retrieves the configuration of a service.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The service does not exist
    uint32_t Controller::endpoint_getconfig(const GetconfigParamsData& params, Core::JSON::String& response)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& callsign = params.Callsign.Value();
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
            response.SetQuoted(false);
            response = service->ConfigLine();
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Updates the configuration of a service.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The service does not exist
    //  - ERROR_GENERAL: Failed to update the configuration
    uint32_t Controller::endpoint_setconfig(const SetconfigParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& callsign = params.Callsign.Value();
        Core::ProxyType<PluginHost::Server::Service> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
            const string& configuration = params.Configuration.Value();
            result = service->ConfigLine(configuration);

            // Normalise return code
            if (result != Core::ERROR_NONE) {
                result = Core::ERROR_GENERAL;
            }
        }

        return result;
    }

    // Stores the configuration.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to store the configuration as it was invalid or malformed
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

    // Downloads a file to the persistent memory.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Operation in progress
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    //  - ERROR_BAD_REQUEST: The given destination path or hash was invalid
    //  - ERROR_WRITE_ERROR: Failed to save the file to the persistent storage (e.g. the file already exists)
    uint32_t Controller::endpoint_download(const Download& params)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;
        const string& source = params.Source.Value();
        const string& destination = params.Destination.Value();
        const string& hash = params.Hash.Value();

        if (source.empty() == false) {
            if (destination.empty() == false) {
                ASSERT(_service != nullptr);

                string path(_service->PersistentPath() + destination);
                uint8_t digest[Crypto::HASH_SHA256];
                uint16_t length = sizeof(digest);

                Core::FromString(hash, digest, length);
                if (length == sizeof(digest)) {

                    if (Core::File(path).Create() == true) {
                        ASSERT(_downloader != nullptr);

                        result = _downloader->Start(source, destination, digest);

                        // Normalise the result
                        if (result == Core::ERROR_COULD_NOT_SET_ADDRESS ) {
                            result = Core::ERROR_INCORRECT_URL;
                        }
                        else if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS) && (result != Core::ERROR_INCORRECT_URL)) {
                            result = Core::ERROR_WRITE_ERROR;
                        }
                    }
                    else {
                        result = Core::ERROR_WRITE_ERROR;
                    }
                }
            }
        }
        else {
            result = Core::ERROR_INCORRECT_URL;
        }

        return result;
    }

    // Removes contents of a directory from the persistent storage.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: The given path was incorrect
    //    ERROR_PRIVILEGED_REQUEST: The path points outside of persistent directory or some files/directories couldn't have been deleted
    uint32_t Controller::endpoint_delete(const DeleteParamsData& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& path = params.Path.Value();

        if (path.empty() == false) {
            if (path.find("..") == string::npos) {
                ASSERT(_service != nullptr);

                DeleteDirectory(_service->PersistentPath() +  path);
                result = Core::ERROR_NONE; // FIXME: return the real deletion result instead
            }
            else {
                result = Core::ERROR_PRIVILIGED_REQUEST;
            }
        }

        return result;
    }

    // Reboots the device.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Rebooting procedure is not available on the device
    //  - ERROR_PRIVILEGED_REQUEST: Insufficient privileges to reboot the device
    //    ERROR_GENERAL: Failed to reboot the device
    uint32_t Controller::endpoint_harakiri()
    {
        uint32_t result =  Core::System::Reboot();

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_UNAVAILABLE) && (result != Core::ERROR_PRIVILIGED_REQUEST) && (result != Core::ERROR_GENERAL)) {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    // Signals a plugin state change.
    void Controller::event_statechange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        StatechangeParamsData params;
        params.Callsign = callsign;
        params.State = state;
        params.Reason = reason;

        Notify(_T("statechange"), params);
    }

    // Signals that a file download has completed.
    void Controller::event_downloadcompleted(uint32_t result, const string& source, const string& destination)
    {
        DownloadcompletedParamsData params;
        params.Result = result;
        params.Source = source;
        params.Destination = destination;

        Notify(_T("downloadcompleted"), params);
    }

} // namespace Plugin

}

