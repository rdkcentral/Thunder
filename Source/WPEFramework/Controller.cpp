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

#include "JsonData_SystemManagement.h"
#include "JsonData_LifeTime.h"
#include "JsonData_Discovery.h"

#include "JDiscovery.h"
#include "JConfiguration.h"
#include "JSystemManagement.h"
#include "JLifeTime.h"
#include "JMetadata.h"

namespace WPEFramework {

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
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData>> jsonBodyMetaDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData::Service>> jsonBodyServiceFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<PluginHost::MetaData::Version>> jsonBodyVersionFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Controller::CallstackData>>> jsonBodyCallstackFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::MetaData::COMRPC>>> jsonBodyProxiesFactory(1);

    static Core::ProxyPoolType<Web::TextBody> jsonBodyTextFactory(2);

    void Controller::Callstack(const ThreadId id, Core::JSON::ArrayType<CallstackData>& response) const {
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
        _service->EnableWebServer(_T("UI"), EMPTY_STRING);

        Register(this);
        Exchange::IController::JConfiguration::Register(*this, this);
        Exchange::IController::JDiscovery::Register(*this, this);
        Exchange::IController::JSystemManagement::Register(*this, this);
        Exchange::IController::JLifeTime::Register(*this, this);
        Exchange::IController::JMetadata::Register(*this, this);

        // On succes return a name as a Callsign to be used in the URL, after the "service"prefix
        return (_T(""));
    }

    /* virtual */ void Controller::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        Exchange::IController::JConfiguration::Unregister(*this);
        Exchange::IController::JDiscovery::Unregister(*this);
        Exchange::IController::JSystemManagement::Unregister(*this);
        Exchange::IController::JLifeTime::Unregister(*this);
        Exchange::IController::JMetadata::Unregister(*this);

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

        Unregister(this);
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

    Core::hresult Controller::Environment(const string& index, string& environment) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        if (Core::SystemInfo::GetEnvironment(index, environment) == true) {
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
        Core::hresult result = Core::ERROR_NONE;
        const string controllerName = _pluginServer->Controller()->Callsign();

        ASSERT(_pluginServer != nullptr);

        if ((basecallsign.empty() == false) && (newcallsign.empty() == false) && (basecallsign != controllerName) && (newcallsign != controllerName)) {
            Core::ProxyType<PluginHost::IShell> baseService, newService;

            if (_pluginServer->Services().FromIdentifier(basecallsign, baseService) != Core::ERROR_NONE) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else if (_pluginServer->Services().FromIdentifier(newcallsign, newService) != Core::ERROR_NONE) {
                result = _pluginServer->Services().Clone(baseService, newcallsign, newService);
            }
            else if (baseService->ClassName() != newService->ClassName()) {
                result = Core::ERROR_GENERAL;
            }
        }
        else {
            result = Core::ERROR_PRIVILIGED_REQUEST;
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
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            // No more parameters, flush it all..
            _pluginServer->Dispatcher().GetMetaData(response->Channels);
            _pluginServer->Services().GetMetaData(response->Channels);
            _pluginServer->Services().GetMetaData(response->Plugins);
            WorkerPoolMetaData(response->Process);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Links")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            _pluginServer->Dispatcher().GetMetaData(response->Channels);
            _pluginServer->Services().GetMetaData(response->Channels);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Plugins")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            _pluginServer->Services().GetMetaData(response->Plugins);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T("Environment")) {
            // We do not want Environment to be included in the variable
            if (index.Next() == true) {
                string value;

                if (Core::SystemInfo::GetEnvironment(index.Remainder().Text(), value) == true) {
                    Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());
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
                    Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData::Service>> response(jsonBodyServiceFactory.Element());

                    serviceInfo->GetMetaData(*response);

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
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData>> response(jsonBodyMetaDataFactory.Element());

            WorkerPoolMetaData(response->Process);

            result->Body(Core::ProxyType<Web::IBody>(response));
        } else if (index.Current() == _T ("Callstack")) {
            if (index.Next() == false) {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = _T("Please supply an index for the callstack you need!");
            }
            else {
                Core::NumberType<uint8_t> threadIndex(index.Current());
                
                Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<CallstackData>>> response = jsonBodyCallstackFactory.Element();
                Callstack(_pluginServer->WorkerPool().Id(threadIndex.Value()), *response);
                result->Body(Core::ProxyType<Web::IBody>(response));
            }
        } else if (index.Current() == _T ("Monitor")) {
            Core::NumberType<uint8_t> threadIndex(index.Current());
            Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<CallstackData>>> response = jsonBodyCallstackFactory.Element();
            Callstack(Core::ResourceMonitor::Instance().Id(), *response);
            result->Body(Core::ProxyType<Web::IBody>(response));
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

                result->Body(Core::ProxyType<Web::IBody>(response));
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

            result->Body(Core::ProxyType<Web::IBody>(response));
        }
        else if (index.Current() == _T("Version")) {
            Core::ProxyType<Web::JSONBodyType<PluginHost::MetaData::Version>> response(jsonBodyVersionFactory.Element());
            _pluginServer->Metadata(*response);
            result->Body(Core::ProxyType<Web::IBody>(response));
        }
        else if (index.Current() == _T("Proxies")) {
            Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<PluginHost::MetaData::COMRPC>>> response(jsonBodyProxiesFactory.Element());
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
            std::list<string>::iterator index(_resumes.begin());

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

    void Controller::Proxies(Core::JSON::ArrayType<PluginHost::MetaData::COMRPC>& response) const {
        RPC::Administrator::Instance().Visit([&](const Core::IPCChannel& channel, const RPC::Administrator::Proxies& proxies)
            {
                PluginHost::MetaData::COMRPC& entry(response.Add());
                const RPC::Communicator::Client* comchannel = dynamic_cast<const RPC::Communicator::Client*>(&channel);

                if (comchannel != nullptr) {
                    string identifier = PluginHost::ChannelIdentifier(comchannel->Source());

                    if (identifier.empty() == false) {
                        entry.Remote = identifier;
                    }
                }

                for (const auto& proxy : proxies) {
                    PluginHost::MetaData::COMRPC::Proxy& info(entry.Proxies.Add());
                    info.InstanceId = proxy->Implementation();
                    info.InterfaceId = proxy->InterfaceId();
                    info.RefCount = proxy->ReferenceCount();
                }
            }
        );
    }

    void Controller::SubSystems()
    {
        string message;
#if THUNDER_RESTFULL_API
        PluginHost::MetaData response;
#endif
        Core::JSON::ArrayType<SubsystemsData> responseJsonRpc;
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
                    SubsystemsData status;
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

    uint32_t Controller::Validate(const string& token, const string& method, const string& paramaters) const /* override */ {
        return(PluginHost::JSONRPC::Validate(token, Core::JSONRPC::Message::Method(method), paramaters));
    }

    uint32_t Controller::Invoke(const uint32_t channelId, const uint32_t id, const string& token, const string& method, const string& parameters, string& response /* @out */) /* override */
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
                    result = (currrentState == PluginHost::IShell::state::HIBERNATED ? Core::ERROR_HIBERNATED : Core::ERROR_ILLEGAL_STATE);
                }
                else {
                    ASSERT(service.IsValid());

                    PluginHost::IDispatcher* dispatcher = reinterpret_cast<PluginHost::IDispatcher*>(service->QueryInterface(PluginHost::IDispatcher::ID));

                    if (dispatcher != nullptr) {
                        PluginHost::ILocalDispatcher* localDispatcher = dispatcher->Local();

                        ASSERT(localDispatcher != nullptr);

                        if (localDispatcher != nullptr) {
                            result = localDispatcher->Invoke(channelId, id, token, Core::JSONRPC::Message::VersionedFullMethod(method), parameters, response);
                        }
                        dispatcher->Release();
                    }
                }
            }
        }

        return (result);
    }

    Core::hresult Controller::Register(Exchange::IController::ILifeTime::INotification* notification)
    {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

        _observers.push_back(notification);
        notification->AddRef();

        _adminLock.Unlock();

        return (Core::ERROR_NONE);
    }

    Core::hresult Controller::Unregister(Exchange::IController::ILifeTime::INotification* notification)
    {
        _adminLock.Lock();

        std::list<Exchange::IController::ILifeTime::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _observers.end());

        if (index != _observers.end()) {
            (*index)->Release();
            _observers.erase(index);
        }

        _adminLock.Unlock();

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

    Core::hresult Controller::Harakiri()
    {
        return (Reboot());
    }

    Core::hresult Controller::Storeconfig()
    {
        return Persist();
    }

    Core::hresult Controller::Proxies(string& response) const
    {
        Core::JSON::ArrayType<PluginHost::MetaData::COMRPC> jsonResponse;
        Proxies(jsonResponse);
        jsonResponse.ToString(response);
        return(Core::ERROR_NONE);
    }

    Core::hresult Controller::StartDiscovery(const uint8_t& ttl)
    {
        if (_probe != nullptr) {
            _probe->Ping(ttl);
        }

        return Core::ERROR_NONE;
    }

    Core::hresult Controller::Status(const string& index, string& response) const
    {
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        Core::JSON::ArrayType<PluginHost::MetaData::Service> jsonResponse;
        Core::ProxyType<PluginHost::IShell> service;

        ASSERT(_pluginServer != nullptr);

        if (index.empty() == true) {
            _pluginServer->Services().GetMetaData(jsonResponse);
            result = Core::ERROR_NONE;
        }
        else {
            if (_pluginServer->Services().FromIdentifier(index, service) == Core::ERROR_NONE) {
                ASSERT(service.IsValid());
                string info;
                result = Core::ERROR_BAD_REQUEST;

                if (service->Metadata(info) == Core::ERROR_NONE) {
                    PluginHost::MetaData::Service status;
                    status.FromString(info);
                    jsonResponse.Add(status);
                    result = Core::ERROR_NONE;
                }
            }
        }
        jsonResponse.ToString(response);

        return result;
    }

    Core::hresult Controller::CallStack(const string& index, string& callstack) const
    {
        Core::JSON::ArrayType<CallstackData> jsonResponse;
        Core::hresult result = Core::ERROR_UNKNOWN_KEY;

        if (index.empty() == true) {
            uint8_t indexValue = Core::NumberType<uint8_t>(Core::TextFragment(index)).Value();

            result = Core::ERROR_NONE;

            Callstack(_pluginServer->WorkerPool().Id(indexValue), jsonResponse);
        }
        jsonResponse.ToString(callstack);

        return result;
    }

    Core::hresult Controller::Links(string& response) const
    {
        Core::JSON::ArrayType<PluginHost::MetaData::Channel> jsonResponse;
        ASSERT(_pluginServer != nullptr);

        _pluginServer->Dispatcher().GetMetaData(jsonResponse);
        _pluginServer->Services().GetMetaData(jsonResponse);
        jsonResponse.ToString(response);

        return Core::ERROR_NONE;
    }

    Core::hresult Controller::ProcessInfo(string& response) const
    {
        PluginHost::MetaData::Server jsonResponse;
        WorkerPoolMetaData(jsonResponse);
        jsonResponse.ToString(response);

        return Core::ERROR_NONE;
    }

    Core::hresult Controller::Subsystems(string& response) const
    {
        Core::JSON::ArrayType<SubsystemsData> jsonResponse;
ASSERT(_service != nullptr);
        PluginHost::ISubSystem* subSystem = _service->SubSystems();

        if (subSystem != nullptr) {
            uint8_t i = 0;
            while (i < PluginHost::ISubSystem::END_LIST) {
                PluginHost::ISubSystem::subsystem current(static_cast<PluginHost::ISubSystem::subsystem>(i));
                SubsystemsData status;
                status.Subsystem = current;
                status.Active = subSystem->IsActive(current);
                jsonResponse.Add(status);
                ++i;
            }
            subSystem->Release();
        }

        jsonResponse.ToString(response);

        return Core::ERROR_NONE;
    }
    Core::hresult Controller::DiscoveryResults(string& response) const
    {
        Core::JSON::ArrayType<PluginHost::MetaData::Bridge> jsonResponse;
        if (_probe != nullptr) {
            Probe::Iterator index(_probe->Instances());

            while (index.Next() == true) {
                PluginHost::MetaData::Bridge element((*index).URL().Text(), (*index).Latency(), (*index).Model(), (*index).IsSecure());
                jsonResponse.Add(element);
            }
        }
        jsonResponse.ToString(response);

        return Core::ERROR_NONE;
    }

    Core::hresult Controller::Version(string& response) const
    {
        PluginHost::MetaData::Version jsonResponse;
        _pluginServer->Metadata(jsonResponse);
        jsonResponse.ToString(response);

        return Core::ERROR_NONE;
    }

    void Controller::StateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        Exchange::IController::JLifeTime::Event::StateChange(*this, callsign, state, reason);
    }

    void Controller::NotifyStateChange(const string& callsign, const PluginHost::IShell::state& state, const PluginHost::IShell::reason& reason)
    {
        _adminLock.Lock();

        std::list<Exchange::IController::ILifeTime::INotification*>::const_iterator index = _observers.begin();

        while(index != _observers.end()) {
            (*index)->StateChange(callsign, state, reason);
            index++;
        }

        _adminLock.Unlock();
    }
}
}
