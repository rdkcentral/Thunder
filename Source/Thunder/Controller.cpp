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

#include "Controller.h"
#include "SystemInfo.h"

#include <plugins/json/json_IController.h>

namespace Thunder {

    namespace {

        static Plugin::Metadata<Plugin::Controller> metadata(
            // Version (Major, Minor, Patch)
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

namespace Plugin {

    // Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
    // Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
    // typedef Web::SignedJSONBodyType<Plugin::Config, Crypto::SHA256HMAC> SignedConfig;
    // Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::Metadata>> jsonBodyMetadataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::Metadata::Service>> jsonBodyServiceFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::Metadata::Version>> jsonBodyVersionFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::CallstackData>>> jsonBodyCallstackFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::Metadata::COMRPC>>> jsonBodyProxiesFactory(1);

    static Core::ProxyPoolType<Web::TextBody> jsonBodyTextFactory(2);

    void Controller::Callstack(const ThreadId id, Core::JSON::ArrayType<PluginHost::CallstackData>& response) const {
        std::list<Core::callstack_info> stackList;

        ::DumpCallStack(id, stackList);

        for (const Core::callstack_info& entry : stackList) {
            response.Add() = entry;
        }
    }

   // Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
    /* virtual */ const string Controller::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(_probe == nullptr);

        _resumes.clear();
        _service = service;

        RPC::ConnectorController::Instance().Announce(service);

        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        Config config;
        config.FromString(_service->ConfigLine());

        if (config.Probe.IsSet() == true) {
            // "239.255.255.250:1900";
            Core::NodeId node (config.Probe.Node.Value().c_str());

            if (node.IsValid() == false) {
                SYSLOG(Logging::Startup, (_T("Probing requested but invalid IP address [%s]"), config.Probe.Node.Value().c_str()));
            }
            else {
                _probe = new Probe(node, _service, config.Probe.TTL.Value(), service->Model());
            }
        }

        Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>>::ConstIterator eventListIterator(static_cast<const Config&>(config).SubSystems.Elements());

        // Insert the subsystems found in the config..
        while (eventListIterator.Next() == true) {
            PluginHost::ISubSystem::subsystem current = eventListIterator.Current().Value();

            if (current >= PluginHost::ISubSystem::END_LIST) {
                Core::EnumerateType<PluginHost::ISubSystem::subsystem> name(current);
                SYSLOG(Logging::Startup, (Core::Format(_T("Subsystem [%s] can not be used as a control value in controller config!!!"), name.Data())));
            }
            else {
                _externalSubsystems.emplace_back(current);
            }
        }

        if ((config.Resumes.IsSet() == true) && (config.Resumes.Length() > 0)) {
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(config.Resumes.Elements());

            while (index.Next() == true) {
                _resumes.push_back(index.Current().Value());
            }
        }

        _service->Register(&_systemInfoReport);

        if (config.Ui.Value() == true) {
            _service->EnableWebServer(_T("UI"), EMPTY_STRING);
        } else {
            _service->DisableWebServer();
        }

        Exchange::Controller::JConfiguration::Register(*this, this);
        Exchange::Controller::JDiscovery::Register(*this, this);
        Exchange::Controller::JSystem::Register(*this, this);
        Exchange::Controller::JLifeTime::Register(*this, this);
        Exchange::Controller::JMetadata::Register(*this, this);
        Exchange::Controller::JSubsystems::Register(*this, this);
        Exchange::Controller::JEvents::Register(*this, this);

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Controller::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        Exchange::Controller::JConfiguration::Unregister(*this);
        Exchange::Controller::JDiscovery::Unregister(*this);
        Exchange::Controller::JSystem::Unregister(*this);
        Exchange::Controller::JLifeTime::Unregister(*this);
        Exchange::Controller::JMetadata::Unregister(*this);
        Exchange::Controller::JSubsystems::Unregister(*this);
        Exchange::Controller::JEvents::Unregister(*this);

        // Detach the SubSystems, we are shutting down..
        PluginHost::ISubSystem* subSystems(_service->SubSystems());

        ASSERT(subSystems != nullptr);

        if (subSystems != nullptr) {
            subSystems->Unregister(&_systemInfoReport);
            subSystems->Release();
        }

        if (_probe != nullptr) {
            delete _probe;
            _probe = nullptr;
        }

        _service->Unregister(&_systemInfoReport);

        /* stop the file serving over http.... */
        service->DisableWebServer();

        RPC::ConnectorController::Instance().Revoke(service);
    }

    /* virtual */ string Controller::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Controller::Inbound(Web::Request& request)
    {
        ASSERT(request.HasBody() == false);

        if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(PluginHost::IFactories::Instance().JSONRPC());
        } else if (request.Verb == Web::Request::HTTP_PUT) {
            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            if ( (index.Next() == true) && (index.Current() == _T("Configuration")) ) {
                request.Body(Core::ProxyType<Web::IBody>(jsonBodyTextFactory.Element()));
            }
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> Controller::Process(const Web::Request& request)
    {
        ASSERT(_pluginServer != nullptr);

        TRACE(Trace::Information, (string(_T("Received request"))));

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

        // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
        index.Next();

        // For now, whatever the URL, we will just, on a get, drop all info we have
        if (request.Verb == Web::Request::HTTP_POST) {
            result = PluginHost::IFactories::Instance().Response();
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("There are no POST handlers!");
        } else if (request.Verb == Web::Request::HTTP_GET) {
            result = GetMethod(index);
        } else if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_DELETE) {
            // Time to remove a plugin, indicated by Current.
            result = DeleteMethod(index, request);
        }

        return (result);
    }

    Core::hresult Controller::Persist()
    {
        ASSERT(_pluginServer != nullptr);

        Core::hresult result = _pluginServer->Persist();

        // Normalise return code
        if (result != Core::ERROR_NONE) {
            result = Core::ERROR_GENERAL;
        }

        return result;

    }

    Core::hresult Controller::Delete(const string& path)
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;
        bool valid;
        string normalized_path = Core::File::Normalize(path, valid);

        ASSERT(_service != nullptr);

        if (valid == false) {
            result = Core::ERROR_PRIVILIGED_REQUEST;
        }
        else {
            Core::File file(_service->PersistentPath() + normalized_path);

            if (file.Exists() == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else if (file.IsDirectory() == true) {
                result = (Core::Directory((_service->PersistentPath() + normalized_path).c_str()).Destroy() == true) ? Core::ERROR_NONE : Core::ERROR_DESTRUCTION_FAILED;
            }
            else {
                result = (file.Destroy() == true) ? Core::ERROR_NONE : Core::ERROR_DESTRUCTION_FAILED;
            }
        }

        return result;
    }

    Core::hresult Controller::Reboot()
    {
        Core::hresult result =  Core::System::Reboot();

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_UNAVAILABLE) && (result != Core::ERROR_PRIVILIGED_REQUEST) && (result != Core::ERROR_GENERAL)) {
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    Core::hresult Controller::Environment(const string& variable, string& value) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        if (Core::SystemInfo::GetEnvironment(variable, value) == true) {
            result = Core::ERROR_NONE;
        }

        return result;
    }

    Core::hresult Controller::Configuration(const string& callsign, string& configuration) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;
        Core::ProxyType<PluginHost::IShell> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
            configuration = service->ConfigLine();
            result = Core::ERROR_NONE;
        }

        return result;
    }

    Core::hresult Controller::Configuration(const string& callsign, const string& configuration)
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;
        Core::ProxyType<PluginHost::IShell> service;

        ASSERT(_pluginServer != nullptr);

        if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
            result = service->ConfigLine(configuration);

            // Normalise return code
            if (result != Core::ERROR_NONE) {
                result = Core::ERROR_GENERAL;
            }
        }

        return result;
    }

    Core::hresult Controller::Clone(const string& basecallsign, const string& newcallsign)
    {
        Core::hresult result = Core::ERROR_PRIVILIGED_REQUEST;
        const string controllerName = _pluginServer->Controller()->Callsign();

        ASSERT(_pluginServer != nullptr);

        if ((basecallsign.empty() == false) && (newcallsign.empty() == false) && (basecallsign != controllerName) && (newcallsign != controllerName)) {
            Core::ProxyType<PluginHost::IShell> baseService, newService;

            if (_pluginServer->Services().FromIdentifier(basecallsign, baseService) != Core::ERROR_NONE) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else if (_pluginServer->Services().FromIdentifier(newcallsign, newService) == Core::ERROR_NONE) {
                result = Core::ERROR_DUPLICATE_KEY;
            }
            else {
                result = _pluginServer->Services().Clone(baseService, newcallsign, newService);
            }
        }

        return result;
    }

    Core::hresult Controller::Hibernate(const string& callsign, const uint32_t timeout)
    {
        Core::hresult result = Core::ERROR_BAD_REQUEST;
        const string controllerName = _pluginServer->Controller()->Callsign();

        if ((callsign.empty() == false) && (callsign != controllerName)) {
            Core::ProxyType<PluginHost::IShell> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) != Core::ERROR_NONE) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else {
                result = service->Hibernate(timeout);
            }
        }
        return (result);
    }

    Core::ProxyType<Web::Response> Controller::GetMethod(Core::TextSegmentIterator& index) const
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ContentType = Web::MIME_JSON;

        if (index.Next() == false) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

            // No more parameters, flush it all..
            _pluginServer->Metadata(response->Channels);
            _pluginServer->Metadata(response->Plugins);
            WorkerPoolMetadata(response->Process);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Links")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

            _pluginServer->Metadata(response->Channels);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Plugins")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

            _pluginServer->Services().GetMetadata(response->Plugins);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Environment")) {
            // We do not want Environment to be included in the variable
            if (index.Next() == true) {
                string value;

                if (Core::SystemInfo::GetEnvironment(index.Remainder().Text(), value) == true) {
                    Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());
                    response->Value = value;

                    result->Body(Core::ProxyType<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Environment variable does not exist");
                }
            }
        } else if (index.Current() == _T("Plugin")) {
            if (index.Next() == true) {
                Core::ProxyType<PluginHost::Server::Service> serviceInfo(FromIdentifier(index.Current().Text()));

                if (serviceInfo.IsValid() == true) {
                    Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata::Service>> response(jsonBodyServiceFactory.Element());

                    serviceInfo->GetMetadata(*response);

                    result->Body(Core::ProxyType<Web::IBody>(response));
                }
            }
        } else if (index.Current() == _T("Configuration")) {
            if (index.Next() == true) {
                Core::ProxyType<PluginHost::Service> serviceInfo(FromIdentifier(index.Current().Text()));

                if (serviceInfo.IsValid() == true) {
                    Core::ProxyType<Web::TextBody> response(jsonBodyTextFactory.Element());

                    *response = serviceInfo->ConfigLine();

                    result->Body(Core::ProxyType<Web::IBody>(response));
                }
            }
        } else if (index.Current() == _T("Process")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

            WorkerPoolMetadata(response->Process);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T ("Callstack")) {
            if (index.Next() == false) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Please supply an index for the callstack you need!");
            }
            else {
                Core::NumberType<uint8_t> threadIndex(index.Current());

                Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::CallstackData>>> response = jsonBodyCallstackFactory.Element();
                Callstack(_pluginServer->WorkerPool().Id(threadIndex.Value()), *response);
                result->Body(Core::ProxyType<Web::IBody>(response));
            }
        } else if (index.Current() == _T ("Monitor")) {
            Core::NumberType<uint8_t> threadIndex(index.Current());
            Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::CallstackData>>> response = jsonBodyCallstackFactory.Element();
            Callstack(Core::ResourceMonitor::Instance().Id(), *response);
            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Discovery")) {

            if (_probe == nullptr) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Probe functionality not enabled!");
            }
            else {
                Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

                Probe::Iterator index(_probe->Instances());

                while (index.Next() == true) {
                    PluginHost::Metadata::Bridge newElement;
                    newElement.Locator = (*index).URL().Text();
                    newElement.Latency = (*index).Latency();
                    if ((*index).Model().empty() == false) {
                        newElement.Model = (*index).Model();
                    }
                    newElement.Secure = (*index).IsSecure();
                    response->Bridges.Add(newElement);
                }

                result->Body(Core::ProxyType<Web::IBody>(response));
            }

        } else if (index.Current() == _T("SubSystems")) {
            PluginHost::ISubSystem* subSystem = _service->SubSystems();
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata>> response(jsonBodyMetadataFactory.Element());

            uint8_t index(0);
            if (subSystem != nullptr) {
                while (index < PluginHost::ISubSystem::END_LIST) {
                    PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(index));
                    response->SubSystems.Add(current, subSystem->IsActive(current));
                    ++index;
                }
                subSystem->Release();
            }

            result->Body(Core::ProxyType<Web::IBody>(response));
        }
        else if (index.Current() == _T("Version")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::Metadata::Version>> response(jsonBodyVersionFactory.Element());
            _pluginServer->Metadata(*response);
            result->Body(Core::ProxyType<Web::IBody>(response));
        }
        else if (index.Current() == _T("Proxies")) {
            Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::Metadata::COMRPC>>> response(jsonBodyProxiesFactory.Element());
            Proxies(*response);
            result->Body(Core::ProxyType<Web::IBody>(response));
        }

        return (result);
    }
    Core::ProxyType<Web::Response> Controller::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // All PUT commands require an additional parameter, so go look for it.
        if (index.Next() == true) {
            if (index.Current() == _T("Activate")) {
                if (index.Next()) {
                    const string callSign(index.Current().Text());
                    if (callSign == _service->Callsign()) {
                        result->ErrorCode = Web::STATUS_FORBIDDEN;
                        result->Message = _T("The PluginHost Controller can not be activated.");
                    } else {
                        Core::ProxyType<PluginHost::Server::Service> pluginInfo(FromIdentifier(callSign));

                        if (pluginInfo.IsValid()) {
                            if (pluginInfo->State() == PluginHost::IShell::DEACTIVATED) {
                                // Activate the plugin.
                                uint32_t returnCode = pluginInfo->Activate(PluginHost::IShell::REQUESTED);

                                // See if this was a successful activation...
                                if (pluginInfo->HasError() == true) {
                                    // Oops seems we failed. Send out the error message
                                    result->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                                    result->Message = pluginInfo->ErrorMessage();
                                } else if (returnCode != Core::ERROR_NONE) {
                                    result->ErrorCode = Web::STATUS_NOT_MODIFIED;
                                    result->Message = _T("Activation already in progress.");
                                }
                            }
                        } else {
                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = _T("There is no callsign: ") + callSign;
                        }
                    }
                }
            } else if (index.Current() == _T("Deactivate")) {
                if (index.Next()) {
                    const string callSign(index.Current().Text());
                    if (callSign == _service->Callsign()) {
                        result->ErrorCode = Web::STATUS_FORBIDDEN;
                        result->Message = _T("The PluginHost Controller can not be deactivated.");
                    } else {
                        Core::ProxyType<PluginHost::Server::Service> pluginInfo(FromIdentifier(callSign));

                        if (pluginInfo.IsValid()) {
                            if (pluginInfo->State() == PluginHost::IShell::ACTIVATED) {
                                // Deactivate the plugin.
                                if (pluginInfo->Deactivate(PluginHost::IShell::REQUESTED) != Core::ERROR_NONE) {
                                    result->ErrorCode = Web::STATUS_NOT_MODIFIED;
                                    result->Message = _T("Deactivation already in progress.");
                                }
                            }
                        } else {
                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = _T("There is no callsign: ") + callSign;
                        }
                    }
                }
            } else if (index.Current() == _T("Unavailable")) {
                if (index.Next()) {
                    const string callSign(index.Current().Text());
                    if (callSign == _service->Callsign()) {
                        result->ErrorCode = Web::STATUS_FORBIDDEN;
                        result->Message = _T("The PluginHost Controller can not set Unavailable.");
                    } else {
                        Core::ProxyType<PluginHost::Server::Service> pluginInfo(FromIdentifier(callSign));

                        if (pluginInfo.IsValid()) {
                            if (pluginInfo->State() == PluginHost::IShell::DEACTIVATED) {
                                // Mark the plugin as unavailable.
                                if (pluginInfo->Unavailable(PluginHost::IShell::REQUESTED) != Core::ERROR_NONE) {
                                    result->ErrorCode = Web::STATUS_NOT_MODIFIED;
                                    result->Message = _T("Marking the plugin as Unavailble failed.");
                                }
                            }
                        } else {
                            result->ErrorCode = Web::STATUS_NOT_FOUND;
                            result->Message = _T("There is no callsign: ") + callSign;
                        }
                    }
                }
            } else if (index.Current() == _T("Configuration")) {
                if ((index.Next() == true) && (request.HasBody() == true)) {

                    Core::ProxyType<PluginHost::Service> serviceInfo(FromIdentifier(index.Current().Text()));
                    Core::ProxyType<const Web::TextBody> data(request.Body<const Web::TextBody>());

                    if ((data.IsValid() == false) || (serviceInfo.IsValid() == false)) {

                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = _T("Not sufficent data to comply to the request");
                    } else {

                        uint32_t error;

                        if ((error = serviceInfo->ConfigLine(*data)) == Core::ERROR_NONE) {
                            result->ErrorCode = Web::STATUS_OK;
                        } else {
                            result->ErrorCode = Web::STATUS_BAD_REQUEST;
                            result->Message = _T("Could not update the config. Error: ") + Core::NumberType<uint32_t>(error).Text();
                        }
                    }
                }
            } else if (index.Current() == _T("Discovery")) {
                if (_probe != nullptr) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Probe functionality not enabled!");
                }
                else {
                    Core::URL::KeyValue options(request.Query.Value());
                    uint8_t ttl = options.Number<uint8_t>(_T("TTL"), 0);

                    _probe->Ping(ttl);

                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Discovery cycle initiated");
                }
            } else if (index.Current() == _T("Persist")) {

                _pluginServer->Persist();

                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Current configuration stored");
            } else if (index.Current() == _T("Harakiri")) {
                uint32_t status = Core::System::Reboot();
                if (status == Core::ERROR_NONE) {
                    result->ErrorCode = Web::STATUS_OK;
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = _T("Could not issue a reboot request. Error: ") + Core::NumberType<uint32_t>(status).Text();
                }
            }
        }
        return (result);
    }

    Core::ProxyType<Web::Response> Controller::DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& /* request */)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // All delete commands require an additional parameter, so go look for it.
        if (index.Next() == true) {
            if (index.Current() == _T("Persistent")) {
                string remainder;

                if (index.Next() == true) {
                    remainder = index.Remainder().Text();
                }

                bool valid;
                string normalized(Core::File::Normalize(remainder, valid));

                if (valid == false) {
                    result->Message = "incorrect path";
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                }
                else {
                    Core::Directory((_service->PersistentPath() + normalized).c_str()).Destroy();
                    result->Message = "OK";
                    result->ErrorCode = Web::STATUS_OK;
                }
            }
        }
        return (result);
    }

    void Controller::StartupResume(const string& callsign, PluginHost::IShell* plugin)
    {
        if (_resumes.size() > 0) {
            Resumes::iterator index(_resumes.begin());

            ASSERT(_service != nullptr);

            while ((index != _resumes.end()) && (*index != callsign)) {
                index++;
            }

            if (index != _resumes.end()) {

                PluginHost::IStateControl* stateControl = plugin->QueryInterface<PluginHost::IStateControl>();

                TRACE_L1("Resuming %s", callsign.c_str());

                if (stateControl != nullptr) {
                    uint32_t error = stateControl->Request(PluginHost::IStateControl::RESUME);

                    if (error != Core::ERROR_NONE) {
                        TRACE_L1("Failed to resume %s, error %d", callsign.c_str(), error);
                    }

                    stateControl->Release();
                }

                _resumes.erase(index);
            }
        }
    }

    void Controller::Proxies(Core::JSON::ArrayType<PluginHost::Metadata::COMRPC>& response) const {
        RPC::Administrator::Instance().Visit([&](const RPC::Administrator::Proxies& proxies)
            {
                PluginHost::Metadata::COMRPC& entry(response.Add());
                const Core::SocketPort* connection = proxies.front()->Socket();

                if (connection != nullptr) {
                    entry.Remote = PluginHost::ChannelIdentifier(*connection);
                }

                for (const auto& proxy : proxies) {
                    PluginHost::Metadata::COMRPC::Proxy& info(entry.Proxies.Add());
                    info.Instance = proxy->Implementation();
                    info.Interface = proxy->InterfaceId();
                    info.Count = proxy->ReferenceCount();
                }
            }
        );
    }

    void Controller::SubSystems()
    {
#if THUNDER_RESTFULL_API || defined(__DEBUG__)
        PluginHost::Metadata response;
#endif
        Core::JSON::ArrayType<JsonData::Subsystems::SubsystemInfo> responseJsonRpc;
        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        // Now prepare a message for the Javascript world.
        bool sendReport = false;
        uint8_t index(0);

        _adminLock.Lock();

        if (subSystem != nullptr) {
            uint32_t reportMask = 0;

            while (index < PluginHost::ISubSystem::END_LIST) {
                PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(index));
                uint32_t bit(1 << index);
                reportMask |= (subSystem->IsActive(current) ? bit : 0);

                if (((reportMask & bit) != 0) ^ ((_lastReported & bit) != 0)) {
                    JsonData::Subsystems::SubsystemInfo status;
                    status.Subsystem = current;
                    status.Active = ((reportMask & bit) != 0);
                    responseJsonRpc.Add(status);

#if THUNDER_RESTFULL_API || defined(__DEBUG__)
                    response.SubSystems.Add(current, ((reportMask & bit) != 0));
#endif

                    sendReport = true;
                }
                ++index;
            }

            _lastReported = reportMask;
            subSystem->Release();
        }

        _adminLock.Unlock();

        if (sendReport == true) {

#if THUNDER_RESTFULL_API || defined(__DEBUG__)
            string message;
            response.ToString(message);
            TRACE_L1("Sending out a SubSystem change notification. %s", message.c_str());
#endif

#if THUNDER_RESTFULL_API
            _pluginServer->_controller->Notification(message);
#endif

            Exchange::Controller::JSubsystems::Event::SubsystemChange(*this, responseJsonRpc);
        }
    }

    Core::hresult Controller::Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response /* @out */) /* override */
    {
        Core::hresult result = Core::ERROR_BAD_REQUEST;
        string callsign(Core::JSONRPC::Message::Callsign(method));

        if (callsign.empty() || (callsign == PluginHost::JSONRPC::Callsign())) {
            result = PluginHost::JSONRPC::Invoke(channelId, id, token, method, parameters, response);
        }
        else {
            Core::ProxyType<PluginHost::IShell> service;

            result = _pluginServer->Services().FromIdentifier(callsign, service);

            if (result == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                PluginHost::IShell::state currrentState = service->State();
                if (currrentState != PluginHost::IShell::state::ACTIVATED)
                {
                    result = (currrentState == PluginHost::IShell::state::HIBERNATED ? Core::ERROR_HIBERNATED : Core::ERROR_UNAVAILABLE);
                    response = (currrentState == PluginHost::IShell::state::HIBERNATED ? _T("Service is hibernated") : _T("Service is not active"));
                }
                else {
                    ASSERT(service.IsValid());

                    PluginHost::IDispatcher* dispatcher = service->QueryInterface<PluginHost::IDispatcher>();

                    if (dispatcher != nullptr) {
                        result = dispatcher->Invoke(channelId, id, token, method, parameters, response);

                        dispatcher->Release();
                    }
                }
            }
        }

        return (result);
    }

    Core::hresult Controller::Register(Exchange::Controller::ILifeTime::INotification* notification)
    {
        ASSERT(notification != nullptr);

        Core::hresult result = Core::ERROR_ALREADY_CONNECTED;
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        LifeTimeNotifiers::iterator index(std::find(_lifeTimeObservers.begin(), _lifeTimeObservers.end(), notification));
        ASSERT(index == _lifeTimeObservers.end());

        if (index == _lifeTimeObservers.end()) {
            _lifeTimeObservers.push_back(notification);
            notification->AddRef();
            result = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        return (result);
    }

    Core::hresult Controller::Unregister(Exchange::Controller::ILifeTime::INotification* notification)
    {
        ASSERT(notification != nullptr);

        Core::hresult result = Core::ERROR_NOT_EXIST;
        _adminLock.Lock();

        LifeTimeNotifiers::iterator index(std::find(_lifeTimeObservers.begin(), _lifeTimeObservers.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _lifeTimeObservers.end());

        if (index != _lifeTimeObservers.end()) {
            (*index)->Release();
            _lifeTimeObservers.erase(index);
            result = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        return (result);
    }

    Core::hresult Controller::Register(Exchange::Controller::IShells::INotification* notification)
    {
        _pluginServer->Services().Register(notification);

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Unregister(Exchange::Controller::IShells::INotification* notification)
    {
        _pluginServer->Services().Unregister(notification);

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Activate(const string& callsign)
    {
        Core::hresult result = Core::ERROR_NONE;
        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::IShell> service;

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

    Core::hresult Controller::Deactivate(const string& callsign)
    {
        Core::hresult result = Core::ERROR_NONE;

        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::IShell> service;

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

    Core::hresult Controller::Unavailable(const string& callsign)
    {
        Core::hresult result = Core::ERROR_NONE;
        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::IShell> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                result = service->Unavailable(PluginHost::IShell::REQUESTED);

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

    Core::hresult Controller::Suspend(const string& callsign)
    {
        Core::hresult result = Core::ERROR_NONE;
        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::IShell> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                PluginHost::IStateControl* stateControl = service->QueryInterface<PluginHost::IStateControl>();

                if (stateControl == nullptr) {
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    result = stateControl->Request(PluginHost::IStateControl::command::SUSPEND);
                    stateControl->Release();
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

    Core::hresult Controller::Resume(const string& callsign)
    {
        Core::hresult result = Core::ERROR_NONE;
        ASSERT(_pluginServer != nullptr);

        if (callsign != Callsign()) {
            Core::ProxyType<PluginHost::IShell> service;

            if (_pluginServer->Services().FromIdentifier(callsign, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                PluginHost::IStateControl* stateControl = service->QueryInterface<PluginHost::IStateControl>();

                if (stateControl == nullptr) {
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    result = stateControl->Request(PluginHost::IStateControl::command::RESUME);
                    stateControl->Release();
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

    Core::hresult Controller::Clone(const string& callsign, const string& newcallsign, string& response)
    {
        Core::hresult result = Clone(callsign, newcallsign);

        if (result == Core::ERROR_NONE) {
            response = newcallsign;
        }
        return result;
    }

    Core::hresult Controller::StartDiscovery(const uint8_t ttl)
    {
        if (_probe != nullptr) {
            _probe->Ping(ttl);
        }

        return Core::ERROR_NONE;
    }

    Core::hresult Controller::DiscoveryResults(IDiscovery::Data::IDiscoveryResultsIterator*& outResults) const
    {
        std::list<IDiscovery::Data::DiscoveryResult> results;

        if (_probe != nullptr) {
            Probe::Iterator index(_probe->Instances());

            while (index.Next() == true) {
                results.push_back({ (*index).URL().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure() });
            }
        }

        if (results.empty() == false) {
            using Iterator = IDiscovery::Data::IDiscoveryResultsIterator;

            outResults = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(results);
            ASSERT(outResults != nullptr);
        }
        else {
            outResults = nullptr;
        }

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Services(const string& callsign, IMetadata::Data::IServicesIterator*& outServices) const
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;
        std::list<IMetadata::Data::Service> services;

        auto _Populate = [&services, this](const string& callsign, PluginHost::IShell* shell) -> void {

            string info;
            IMetadata::Data::Service service{};

            if (shell->Metadata(info) == Core::ERROR_NONE) {
                PluginHost::Metadata::Service meta;
                meta.FromString(info);


                service.Callsign = meta.Callsign;
                service.Locator = meta.Locator;
                service.ClassName = meta.ClassName;
                service.Module = meta.Module;
                service.StartMode = meta.StartMode;
                service.State = meta.JSONState;
                service.Version = meta.ServiceVersion;
                service.Communicator = meta.Communicator;
                service.PersistentPathPostfix = meta.PersistentPathPostfix;
                service.VolatilePathPostfix = meta.VolatilePathPostfix;
                service.SystemRootPath = meta.SystemRootPath;
                service.Configuration = meta.Configuration;
                service.Precondition = meta.Precondition;
                service.Termination = meta.Termination;

                #if THUNDER_RESTFULL_API
                service.Observers = meta.Observers;
                #endif

                #if THUNDER_RUNTIME_STATISTICS
                service.ProcessedRequests = meta.ProcessedRequests;
                service.ProcessedObjects = meta.ProcessedObjects;
                #endif

                // Make sure the list is sorted..
                std::list<IMetadata::Data::Service>::iterator index(services.begin());
                while ((index != services.end()) && (index->Callsign < callsign)) {
                    index++;
                }
                services.insert(index, service);
            }
        };

        if (callsign.empty() == true) {
            auto it = _pluginServer->Services().Services();

            while (it.Next() == true) {
                _Populate(it.Current()->Callsign(), it.Current().operator->());
            }
        }
        else {
            PluginHost::IShell* shell = _service->QueryInterfaceByCallsign<PluginHost::IShell>(callsign);
            if (shell != nullptr) {
                _Populate(callsign, shell);
                shell->Release();
            }
        }

        if (services.empty() == false) {
            using Iterator = IMetadata::Data::IServicesIterator;

            outServices = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(services);
            ASSERT(outServices != nullptr);
            result = Core::ERROR_NONE;
        }
        else {
            outServices = nullptr;
        }

        return (result);
    }

    Core::hresult Controller::CallStack(const uint8_t thread, IMetadata::Data::ICallStackIterator*& outCallStack) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        std::list<Core::callstack_info> callStackInfo;

        ASSERT(_pluginServer != nullptr);

        ::DumpCallStack(_pluginServer->WorkerPool().Id(thread), callStackInfo);

        if (callStackInfo.empty() == false) {

            std::list<IMetadata::Data::CallStack> callstack;

            for (const Core::callstack_info& entry : callStackInfo) {
                callstack.push_back({ reinterpret_cast<Core::instance_id>(entry.address), entry.function, entry.module,
                                      (entry.line != static_cast<uint32_t>(~0)? entry.line : 0) });
            }

            using Iterator = IMetadata::Data::ICallStackIterator;

            outCallStack = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(callstack);
            ASSERT(outCallStack != nullptr);

            result = Core::ERROR_NONE;
        }
        else {
            outCallStack = nullptr;
        }

        return (result);
    }

    Core::hresult Controller::Links(IMetadata::Data::ILinksIterator*& outLinks) const
    {
        Core::JSON::ArrayType<PluginHost::Metadata::Channel> meta;

        ASSERT(_pluginServer != nullptr);

        _pluginServer->Metadata(meta);

        if (meta.Length() > 0) {
            std::list<IMetadata::Data::Link> links;

            auto it = meta.Elements();

            while (it.Next() == true) {
                auto const& entry = it.Current();
                links.push_back({ entry.Remote.Value(), entry.JSONState.Value(), entry.Name.Value(), entry.ID.Value(), entry.Activity.Value() });
            }

            using Iterator = IMetadata::Data::ILinksIterator;

            outLinks = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(links);
            ASSERT(outLinks != nullptr);
        }
        else {
            outLinks = nullptr;
        }

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Proxies(const uint32_t linkId, IMetadata::Data::IProxiesIterator*& outProxies) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        Core::JSON::ArrayType<PluginHost::Metadata::Channel> meta;
       _pluginServer->Services().GetMetadata(meta);

        Core::JSON::ArrayType<PluginHost::Metadata::COMRPC> comrpc;
        Proxies(comrpc);

        string link;
        std::list<IMetadata::Data::Proxy> proxies;

        auto it = meta.Elements();

        while (it.Next() == true) {

            if (it.Current().ID.Value() == linkId) {
                link = it.Current().Remote.Value();
                break;
            }
        }

        if (link.empty() == false) {
            auto it = comrpc.Elements();

            while (it.Next() == true) {

                if (it.Current().Remote.Value() == link) {
                    auto it2 = it.Current().Proxies.Elements();

                    while (it2.Next() == true) {
                        auto const& entry = it2.Current();

                        proxies.push_back({ entry.Interface.Value(), entry.Instance.Value(), entry.Count.Value() });
                    }

                    break;
                }
            }
        }

        if (proxies.empty() == false) {
            using Iterator = IMetadata::Data::IProxiesIterator;

            outProxies = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(proxies);
            ASSERT(outProxies != nullptr);

            result = Core::ERROR_NONE;
        }
        else {
            outProxies = nullptr;
        }

        return (result);
    }

    Core::hresult Controller::Threads(IMetadata::Data::IThreadsIterator*& outThreads) const
    {
        PluginHost::Metadata::Server meta;

        WorkerPoolMetadata(meta);

        if (meta.ThreadPoolRuns.Length() > 0) {

            std::list<IMetadata::Data::Thread> threads;

            auto it = meta.ThreadPoolRuns.Elements();

            while (it.Next() == true) {
                auto const& entry = it.Current();
                threads.push_back({ entry.Id.Value(), entry.Job.Value(), entry.Runs.Value() });
            }

            using Iterator = IMetadata::Data::IThreadsIterator;

            outThreads = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(threads);
            ASSERT(outThreads != nullptr);
        }
        else {
            outThreads = nullptr;
        }

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::PendingRequests(IMetadata::Data::IPendingRequestsIterator*& outRequests) const
    {
        PluginHost::Metadata::Server meta;

        WorkerPoolMetadata(meta);

        if (meta.PendingRequests.Length() > 0) {

            std::list<string> requests;

            auto it = meta.PendingRequests.Elements();

            while (it.Next() == true) {
                requests.push_back(it.Current().Value());
            }

            using Iterator = IMetadata::Data::IPendingRequestsIterator;

            outRequests = Core::ServiceType<RPC::IteratorType<Iterator>>::Create<Iterator>(requests);
            ASSERT(outRequests != nullptr);
        }
        else {
            outRequests = nullptr;
        }

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Subsystems(ISubsystems::ISubsystemsIterator*& outSubsystems) const
    {
        ASSERT(_service != nullptr);

        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        if (subSystem != nullptr) {
            std::list<ISubsystems::Subsystem> subsystems;

            std::underlying_type<PluginHost::ISubSystem::subsystem>::type i = 0;

            while (i < PluginHost::ISubSystem::END_LIST) {

                const PluginHost::ISubSystem::subsystem subsystem = static_cast<PluginHost::ISubSystem::subsystem>(i);
                subsystems.push_back({ subsystem, subSystem->IsActive(subsystem)});
                i++;
            }

            subSystem->Release();

            outSubsystems = Core::ServiceType<RPC::IteratorType<ISubsystems::ISubsystemsIterator>>::Create<ISubsystems::ISubsystemsIterator>(subsystems);
            ASSERT(outSubsystems != nullptr);
        }
        else {
            outSubsystems = nullptr;
        }

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Version(IMetadata::Data::Version& version) const
    {
        PluginHost::Metadata::Version ver;

        _pluginServer->Metadata(ver);

        version.Hash = ver.Hash;
        version.Major = ver.Major;
        version.Minor = ver.Minor;
        version.Patch = ver.Patch;

        return (Core::ERROR_NONE);
    }

    void Controller::NotifyStateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        _adminLock.Lock();

        LifeTimeNotifiers::const_iterator index = _lifeTimeObservers.begin();

        while(index != _lifeTimeObservers.end()) {
            (*index)->StateChange(callsign, state, reason);
            index++;
        }

        _adminLock.Unlock();

        // also notify the JSON RPC listeners (if any)
        Exchange::Controller::JLifeTime::Event::StateChange(*this, callsign, state, reason);
    }
}
}
