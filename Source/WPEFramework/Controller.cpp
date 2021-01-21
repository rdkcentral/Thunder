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

#include "Controller.h"
#include "SystemInfo.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(Controller, 1, 0);

    // Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
    // Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
    // typedef Web::SignedJSONBodyType<Plugin::Config, Crypto::SHA256HMAC> SignedConfig;
    // Signing will be done on BackOffice level. The Controller I/F will never be exposed to the outside world.
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData>> jsonBodyMetaDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData::Service>> jsonBodyServiceFactory(1);
    static Core::ProxyPoolType<Web::TextBody> jsonBodyTextFactory(2);

    void Controller::SubSystems(Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem>>::ConstIterator& index)
    {
        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        if (subSystem != nullptr) {
            uint32_t entries = ((1 << PluginHost::ISubSystem::END_LIST) - 1);

            while (index.Next() == true) {
                if (index.Current() < PluginHost::ISubSystem::END_LIST) {
                    entries &= ~(1 << index.Current());
                }
            }

            uint8_t setFlag = 0;
            while (setFlag < PluginHost::ISubSystem::END_LIST) {
                if ((entries & (1 << setFlag)) != 0) {
                    TRACE_L1("Setting the default SubSystem: %d", setFlag);
                    subSystem->Set(static_cast<PluginHost::ISubSystem::subsystem>(setFlag), nullptr);
                }
                setFlag++;
            }

            subSystem->Release();
        }
    }

   // Access to this interface will be through the BackOffice Plugin, if external exposure is required !!!
    /* virtual */ const string Controller::Initialize(PluginHost::IShell* service)
    {

        ASSERT(_service == nullptr);
        ASSERT(_probe == nullptr);

        _resumes.clear();
        _service = service;
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
        SubSystems(eventListIterator);

        if ((config.Resumes.IsSet() == true) && (config.Resumes.Length() > 0)) {
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(config.Resumes.Elements());

            while (index.Next() == true) {
                _resumes.push_back(index.Current().Value());
            }
        }

        _service->Register(&_systemInfoReport);

        _service->EnableWebServer(_T("UI"), EMPTY_STRING);

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Controller::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        // Detach the SubSystems, we are shutting down..
        PluginHost::ISubSystem* subSystems(_service->SubSystems());

        ASSERT(subSystems != nullptr);

        if (subSystems != nullptr) {
            subSystems->Unregister(&_systemInfoReport);
        }

        if (_probe != nullptr) {
            delete _probe;
            _probe = nullptr;
        }

        _service->Unregister(&_systemInfoReport);

        /* stop the file serving over http.... */
        service->DisableWebServer();
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
                request.Body(Core::proxy_cast<Web::IBody>(jsonBodyTextFactory.Element()));
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

    Core::ProxyType<Web::Response> Controller::GetMethod(Core::TextSegmentIterator& index) const
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ContentType = Web::MIME_JSON;

        if (index.Next() == false) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            // No more parameters, flush it all..
            _pluginServer->Dispatcher().GetMetaData(response->Channels);
            _pluginServer->Services().GetMetaData(response->Plugins);
            WorkerPoolMetaData(response->Process);

            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else if (index.Current() == _T("Links")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            _pluginServer->Dispatcher().GetMetaData(response->Channels);

            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else if (index.Current() == _T("Plugins")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            _pluginServer->Services().GetMetaData(response->Plugins);

            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else if (index.Current() == _T("Environment")) {
            // We do not want Environment to be included in the variable
            if (index.Next() == true) {
                string value;

                if (Core::SystemInfo::GetEnvironment(index.Remainder().Text(), value) == true) {
                    Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());
                    response->Value = value;

                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("Environment variable does not exist");
                }
            }
        } else if (index.Current() == _T("Plugin")) {
            if (index.Next() == true) {
                Core::ProxyType<PluginHost::Server::Service> serviceInfo(FromIdentifier(index.Current().Text()));

                if (serviceInfo.IsValid() == true) {
                    Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData::Service>> response(jsonBodyServiceFactory.Element());

                    serviceInfo->GetMetaData(*response);

                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
        } else if (index.Current() == _T("Configuration")) {
            if (index.Next() == true) {
                Core::ProxyType<PluginHost::Service> serviceInfo(FromIdentifier(index.Current().Text()));

                if (serviceInfo.IsValid() == true) {
                    Core::ProxyType<Web::TextBody> response(jsonBodyTextFactory.Element());

                    *response = serviceInfo->ConfigLine();

                    result->Body(Core::proxy_cast<Web::IBody>(response));
                }
            }
        } else if (index.Current() == _T("Process")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            WorkerPoolMetaData(response->Process);

            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else if (index.Current() == _T("Discovery")) {

            if (_probe == nullptr) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Probe functionality not enabled!");
            }
            else {
                Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

                Probe::Iterator index(_probe->Instances());

                while (index.Next() == true) {
                    PluginHost::MetaData::Bridge newElement((*index).URL().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure());
                    response->Bridges.Add(newElement);
                }

                result->Body(Core::proxy_cast<Web::IBody>(response));
            }

        } else if (index.Current() == _T("SubSystems")) {
            PluginHost::ISubSystem* subSystem = _service->SubSystems();
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            uint8_t index(0);
            if (subSystem != nullptr) {
                while (index < PluginHost::ISubSystem::END_LIST) {
                    PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(index));
                    response->SubSystems.Add(current, subSystem->IsActive(current));
                    ++index;
                }
                subSystem->Release();
            }

            result->Body(Core::proxy_cast<Web::IBody>(response));
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

                _pluginServer->Services().Persist();

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

                Core::Directory((_service->PersistentPath() + remainder).c_str()).Destroy(true);

                result->Message = "OK";
            }
        }
        return (result);
    }

    void Controller::StateChange(PluginHost::IShell* plugin)
    {
        event_statechange(plugin->Callsign(), plugin->State(), plugin->Reason());

        if ((plugin->State() == PluginHost::IShell::ACTIVATED) && (_resumes.size() > 0)) {
            string callsign(plugin->Callsign());
            std::list<string>::const_iterator index(_resumes.begin());

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

    void Controller::SubSystems()
    {
        string message;
#if THUNDER_RESTFULL_API
        PluginHost::MetaData response;
#endif
        Core::JSON::ArrayType<JsonData::Controller::SubsystemsParamsData> responseJsonRpc;
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
                    JsonData::Controller::SubsystemsParamsData status;
                    status.Subsystem = current;
                    status.Active = ((reportMask & bit) != 0);
                    responseJsonRpc.Add(status);

#if THUNDER_RESTFULL_API
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

            response.ToString(message);

            TRACE_L1("Sending out a SubSystem change notification. %s", message.c_str());

#if THUNDER_RESTFULL_API
            _pluginServer->_controller->Notification(message);
#endif
            Notify("subsystemchange", responseJsonRpc);
        }
    }
    /* virtual */ Core::ProxyType<Core::JSONRPC::Message> Controller::Invoke(const string& token, const uint32_t channelId, const Core::JSONRPC::Message& inbound)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;
        bool asyncCall = false;
        string callsign(inbound.Callsign());
        Core::ProxyType<Core::JSONRPC::Message> response;

        if (callsign.empty() || (callsign == PluginHost::JSONRPC::Callsign())) {
            response = PluginHost::JSONRPC::Invoke(token, channelId, inbound);
		} else {
			Core::ProxyType<PluginHost::Server::Service> service;

            uint32_t result = _pluginServer->Services().FromIdentifier(callsign, service);

            if (result == Core::ERROR_NONE) {
                ASSERT(service.IsValid());

                Core::JSONRPC::Message forwarder;

                forwarder.Id = inbound.Id;
                forwarder.Parameters = inbound.Parameters;
                    
                forwarder.Designator = inbound.VersionedFullMethod();
                response = service->Invoke(token, channelId, forwarder);
                asyncCall = (response.IsValid() == false);
            }
        }

        if ((inbound.Id.Value() != static_cast<uint32_t>(~0)) && (response.IsValid() == false) && (asyncCall == false)) {
            response = Message();
            response->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
            response->Id = inbound.Id.Value();
            response->Error.SetError(result);

            switch (result) {
                case Core::ERROR_UNAVAILABLE:
                    response->Error.Text = "Requested service is not available";
                    break;
                case Core::ERROR_INVALID_SIGNATURE:
                    response->Error.Text = "Invalid service name or version";
                    break;
                case Core::ERROR_BAD_REQUEST:
                    response->Error.Text = "Could not access requested service";
                    break;
                default:
                    response->Error.Text = "Invalid JSONRPC Request";
            }
        }

        return (response);
    }
}
}
