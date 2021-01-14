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

#ifndef __WEBBRIDGEPLUGINSERVER_H
#define __WEBBRIDGEPLUGINSERVER_H

#include "Module.h"
#include "SystemInfo.h"
#include "Config.h"
#include "IRemoteInstantiation.h"

#ifdef PROCESSCONTAINERS_ENABLED
#include "../processcontainers/ProcessContainer.h"
#endif

#ifndef HOSTING_COMPROCESS
#error "Please define the name of the COM process!!!"
#endif

#define MAX_EXTERNAL_WAITS 2000 /* Wait for 2 Seconds */

namespace WPEFramework {

namespace Core {
    namespace System {
        extern "C" {
        typedef const char* (*ModuleNameImpl)();
        typedef const char* (*ModuleBuildRefImpl)();
        }
    }
}

namespace Plugin {
    class Controller;
}

namespace PluginHost {
    class Server {
    private:
        Server() = delete;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

    public:
        static const TCHAR* ConfigFile;
        static const TCHAR* PluginOverrideFile;
        static const TCHAR* PluginConfigDirectory;
        static const TCHAR* CommunicatorConnector;

        class ForwardMessage : public Core::JSON::Container {
        private:
            ForwardMessage(const ForwardMessage&) = delete;
            ForwardMessage& operator=(const ForwardMessage&) = delete;

        public:
            ForwardMessage()
                : Core::JSON::Container()
                , Callsign(true)
                , Data(false)
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("data"), &Data);
            }
            ForwardMessage(const string& callsign, const string& message)
                : Core::JSON::Container()
                , Callsign(true)
                , Data(false)
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("data"), &Data);

                Callsign = callsign;
                Data = message;
            }
            ~ForwardMessage()
            {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::String Data;
        };

    private:
        class WorkerPoolImplementation : public Core::WorkerPool {
        public:
            WorkerPoolImplementation() = delete;
            WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
            WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

            WorkerPoolImplementation(const uint32_t stackSize)
                : Core::WorkerPool(THREADPOOL_COUNT, stackSize, 16)
            {
            }
            virtual ~WorkerPoolImplementation()
            {
            }
        };

        class FactoriesImplementation : public IFactories {
        public:
            FactoriesImplementation(const FactoriesImplementation&) = delete;
            FactoriesImplementation& operator=(const FactoriesImplementation&) = delete;

            FactoriesImplementation()
                : _requestFactory(5)
                , _responseFactory(5)
                , _fileBodyFactory(5)
                , _jsonRPCFactory(5)
            {
            }
            ~FactoriesImplementation() override = default;

        public:
            Core::ProxyType<Web::Request> Request() override
            {
                return (_requestFactory.Element());
            }
            Core::ProxyType<Web::Response> Response() override
            {
                return (_responseFactory.Element());
            }
            Core::ProxyType<Web::FileBody> FileBody() override
            {
                return (_fileBodyFactory.Element());
            }
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> JSONRPC() override
            {
                return (Core::proxy_cast< Web::JSONBodyType<Core::JSONRPC::Message> >(_jsonRPCFactory.Element()));
            }

        private:
            Core::ProxyPoolType<Web::Request> _requestFactory;
            Core::ProxyPoolType<Web::Response> _responseFactory;
            Core::ProxyPoolType<Web::FileBody> _fileBodyFactory;
            Core::ProxyPoolType<PluginHost::JSONRPCMessage> _jsonRPCFactory;
        };

        class ServiceMap;
        friend class Plugin::Controller;

        // Trace class for internal information of the PluginHost
        class Activity {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            Activity(const Activity& a_Copy) = delete;
            Activity& operator=(const Activity& a_RHS) = delete;

        public:
            Activity(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            Activity(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~Activity()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class WebFlow {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            WebFlow(const WebFlow& a_Copy) = delete;
            WebFlow& operator=(const WebFlow& a_RHS) = delete;

        public:
            WebFlow(const Core::ProxyType<Web::Request>& request)
            {
                if (request.IsValid() == true) {
                    string text;

                    request->ToString(text);

                    _text = Core::ToString(string('\n' + text + '\n'));
                }
            }
            WebFlow(const Core::ProxyType<Web::Response>& response)
            {
                if (response.IsValid() == true) {
                    string text;

                    response->ToString(text);

                    _text = Core::ToString(string('\n' + text + '\n'));
                }
            }
            ~WebFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class SocketFlow {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            SocketFlow(const SocketFlow& a_Copy) = delete;
            SocketFlow& operator=(const SocketFlow& a_RHS) = delete;

        public:
            SocketFlow(const Core::ProxyType<Core::JSON::IElement>& object)
            {
                if (object.IsValid() == true) {
                    string text;

                    object->ToString(text);

                    _text = Core::ToString(text);
                }
            }
            ~SocketFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };
        class TextFlow {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            TextFlow(const TextFlow& a_Copy) = delete;
            TextFlow& operator=(const TextFlow& a_RHS) = delete;

        public:
            TextFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~TextFlow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };

        class Service : public IShell::ICOMLink, public PluginHost::Service {
        private:
            Service() = delete;
            Service(const Service&) = delete;
            Service& operator=(const Service&) = delete;

            class Condition {
            private:
                Condition() = delete;
                Condition(const Condition&) = delete;
                Condition& operator=(const Condition&) = delete;

            public:
                Condition(const Core::JSON::ArrayType<Core::JSON::EnumType<ISubSystem::subsystem>>& input, const bool defaultValue)
                    : _events(0)
                    , _value(0)
                {
                    Core::JSON::ArrayType<Core::JSON::EnumType<ISubSystem::subsystem>>::ConstIterator index(input.Elements());

                    while (index.Next() == true) {
                        uint32_t bitNr = static_cast<uint32_t>(index.Current());

                        if (bitNr >= ISubSystem::NEGATIVE_START) {
                            bitNr -= ISubSystem::NEGATIVE_START;
                        } else {
                            _value |= (1 << bitNr);
                        }

                        // Make sure the event is only set once (POSITIVE or NEGATIVE)
                        ASSERT((_events & (1 << bitNr)) == 0);
                        _events |= 1 << bitNr;
                    }

                    if (_events == 0) {
                        _events = (defaultValue ? 0 : ~0);
                        _value = (defaultValue ? 0 : ~0);
                    }
                }
                ~Condition()
                {
                }

            public:
                inline bool IsMet() const
                {
                    return ((_events == 0) || ((_value != static_cast<uint32_t>(~0)) && ((_events & (1 << ISubSystem::END_LIST)) != 0)));
                }
                inline bool Evaluate(const uint32_t subsystems)
                {
                    bool result = (subsystems & _events) == _value;

                    if (result ^ IsMet()) {
                        // We changed from setup, signal it...
                        if (result == true) {
                            _events |= (1 << ISubSystem::END_LIST);
                        } else {
                            _events &= ~(1 << ISubSystem::END_LIST);
                        }
                        result = true;
                    } else {
                        result = false;
                    }

                    return (result);
                }
                inline uint32_t Delta(const uint32_t currentSet)
                {
                    return ((currentSet & _events) ^ _value);
                }

            private:
                uint32_t _events;
                uint32_t _value;
            };

        public:
            Service(const PluginHost::Config& server, const Plugin::Config& plugin, ServiceMap& administrator)
                : PluginHost::Service(plugin, server.WebPrefix(), server.PersistentPath(), server.DataPath(), server.VolatilePath())
                , _pluginHandling()
                , _handler(nullptr)
                , _extended(nullptr)
                , _webRequest(nullptr)
                , _webSocket(nullptr)
                , _textSocket(nullptr)
                , _rawSocket(nullptr)
                , _webSecurity(nullptr)
                , _jsonrpc(nullptr)
                , _precondition(plugin.Precondition, true)
                , _termination(plugin.Termination, false)
                , _activity(0)
                , _connection(nullptr)
                , _administrator(administrator)
            {
            }
            ~Service()
            {
                Deactivate(IShell::SHUTDOWN);

                ASSERT(_handler == nullptr);
                ASSERT(_extended == nullptr);
                ASSERT(_webRequest == nullptr);
                ASSERT(_webSocket == nullptr);
                ASSERT(_textSocket == nullptr);
                ASSERT(_rawSocket == nullptr);
                ASSERT(_webSecurity == nullptr);
                ASSERT(_jsonrpc == nullptr);
                ASSERT(_connection == nullptr);
            }

        public:
            static void Initialize()
            {
                _missingHandler->ErrorCode = Web::STATUS_SERVICE_UNAVAILABLE;
                _missingHandler->Message = _T("The requested service is not loaded.");

                _unavailableHandler->CacheControl = _T("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0");
                _unavailableHandler->ErrorCode = Web::STATUS_GONE;
                _unavailableHandler->Message = _T("The requested service is currently in the deactivated mode.");
            }

        public:
            inline const string& ModuleName() const
            {
                return (_moduleName);
            }
            inline const string& VersionHash() const
            {
                return (_versionHash);
            }
            template <typename CLASSTYPE>
            inline CLASSTYPE* ClassType()
            {
                return (_handler != nullptr ? dynamic_cast<CLASSTYPE*>(_handler) : nullptr);
            }
            inline bool WebRequestSupported() const
            {
                return (_webRequest != nullptr);
            }
            inline bool WebSocketSupported() const
            {
                return (_webSocket != nullptr);
            }
            inline bool TextSocketSupported() const
            {
                return (_textSocket != nullptr);
            }
            inline bool RawChannelSupported() const
            {
                return (_rawSocket != nullptr);
            }
            inline bool Subscribe(Channel& channel)
            {
#if THUNDER_RESTFULL_API
                bool result = PluginHost::Service::Subscribe(channel);

                if ((result == true) && (_extended != nullptr)) {
                    _extended->Attach(channel);
                }

                return (result);
#else
                if (_extended != nullptr) {
                    _extended->Attach(channel);
                }

                return (_extended != nullptr);
#endif
            }
            inline void Unsubscribe(Channel& channel)
            {
#if THUNDER_RESTFULL_API
                PluginHost::Service::Unsubscribe(channel);
#endif
                if (_extended != nullptr) {
                    _extended->Detach(channel);
                }
            }
            inline void Configuration(const string& config)
            {
                PluginHost::Service::ConfigLine(config);
            }
            string Configuration() const
            {

                Lock();
                string result(PluginHost::Service::ConfigLine());
                Unlock();

                return (result);
            }
            bool Update(const Plugin::Config& config)
            {
                bool succeeded = false;

                Lock();

                if (State() == DEACTIVATED) {
                    // Start by updating the config
                    Update(config);

                    TRACE(Activity, (_T("Updated plugin [%s]:[%s]"), PluginHost::Service::Configuration().ClassName.Value().c_str(), PluginHost::Service::Configuration().Callsign.Value().c_str()));
                    succeeded = true;
                }

                Unlock();

                return (succeeded);
            }
            void Destroy()
            {
                ASSERT(_handler != nullptr);

                Lock();

                // It's reference counted, so just take it out of the list, state to DESTROYED
                // Also unsubscribe all subscribers. They need to go..
                State(DESTROYED);
                _administrator.StateChange(this);

                Unlock();
            }

            // The service might be still alive and refered to in the request/links but they will
            // not trigger any reuests internally, oly for the destroy we require everything to be killed...
            inline void Inbound(Web::Request& request)
            {
                Lock();

                if ((_webRequest != nullptr) && (IsActive() == true)) {
                    _webRequest->Inbound(request);
                }

                Unlock();
            }
            virtual Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier)
            {
                Core::ProxyType<Core::JSON::IElement> result;
                Lock();

                if ((_webSocket != nullptr) && (IsActive() == true)) {
                    result = _webSocket->Inbound(identifier);
                }

                Unlock();

                return (result);
            }
            inline Core::ProxyType<Web::Response> Evaluate(const Request& request)
            {
                Core::ProxyType<Web::Response> result;

                Lock();

                if (IsActive() == false) {
                    result = _unavailableHandler;
                } else if (IsWebServerRequest(request.Path) == true) {
                    result = IFactories::Instance().Response();
                    FileToServe(request.Path, *result);
                } else if (request.Verb == Web::Request::HTTP_OPTIONS) {

                    result = IFactories::Instance().Response();

                    TRACE_L1("Filling the Options on behalf of: %s", request.Path.c_str());

                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = _T("No Content"); // Core::EnumerateType<Web::WebStatus>(_optionResponse->ErrorCode).Text();
                    result->Allowed = request.AccessControlMethod.Value();
                    result->AccessControlMethod = Request::HTTP_GET | Request::HTTP_POST | Request::HTTP_PUT | Request::HTTP_DELETE;
                    result->AccessControlOrigin = _T("*");
                    result->AccessControlHeaders = _T("Content-Type");

                    // This will last for an hour, try again after an hour :-)
                    result->AccessControlMaxAge = 60 * 60;

                    result->Date = Core::Time::Now();
                } else if (WebRequestSupported() == false) {
                    result = _missingHandler;
                }

                Unlock();

                return (result);
            }
            inline Core::ProxyType<Web::Response> Process(const Web::Request& request)
            {
                Core::ProxyType<Web::Response> result;

                Lock();

                if ((_webRequest != nullptr) && (IsActive() == true)) {
                    IWeb* service(_webRequest);
                    service->AddRef();
                    Unlock();

#if THUNDER_RUNTIME_STATISTICS
                    IncrementProcessedRequests();
#endif
                    Core::InterlockedIncrement(_activity);
                    result = service->Process(request);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                } else {
                    Unlock();
                }

                return (result);
            }
            Core::ProxyType<Core::JSONRPC::Message> Invoke(const string& token, const uint32_t id, const Core::JSONRPC::Message& message)
            {
                Core::ProxyType<Core::JSONRPC::Message> result;

                Lock();

                if ( (_jsonrpc == nullptr) || (IsActive() == false) ) {
                    Unlock();

                    result = Core::proxy_cast<Core::JSONRPC::Message>(Factories::Instance().JSONRPC());
                    result->Error.SetError(Core::ERROR_BAD_REQUEST);
                    result->Error.Text = _T("Service is not active");
                    result->Id = message.Id;
                }
                else {
                    IDispatcher* service(_jsonrpc);
                    service->AddRef();
                    Unlock();

#if THUNDER_RUNTIME_STATISTICS
                    IncrementProcessedRequests();
#endif
                    Core::InterlockedIncrement(_activity);
                    result = service->Invoke(token, id, message);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                } 

                return (result);
            }
            inline Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element)
            {
                Core::ProxyType<Core::JSON::IElement> result;

                Lock();

                if ((_webSocket != nullptr) && (IsActive() == true)) {
                    IWebSocket* service(_webSocket);
                    service->AddRef();
                    Unlock();

#if THUNDER_RUNTIME_STATISTICS
                    IncrementProcessedObjects();
#endif
                    Core::InterlockedIncrement(_activity);
                    result = service->Inbound(ID, element);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                } else {
                    Unlock();
                }

                return (result);
            }
            inline string Inbound(const uint32_t ID, const string& value)
            {
                string result;

                Lock();

                if ((_textSocket != nullptr) && (IsActive() == true)) {

                    ITextSocket* service(_textSocket);
                    service->AddRef();
                    Unlock();

                    Core::InterlockedIncrement(_activity);
                    result = service->Inbound(ID, value);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                } else {
                    Unlock();
                }

                return (result);
            }
            inline uint32_t Inbound(const uint32_t ID, const uint8_t data[], const uint16_t length)
            {
                uint32_t result = length;

                Lock();

                if ((_rawSocket != nullptr) && (IsActive() == true)) {
                    result = _rawSocket->Inbound(ID, data, length);
                }

                Unlock();

                return (result);
            }
            inline uint32_t Outbound(const uint32_t ID, uint8_t data[], const uint16_t length) const
            {
                uint32_t result = 0;

                Lock();

                if ((_rawSocket != nullptr) && (IsActive() == true)) {
                    result = _rawSocket->Outbound(ID, data, length);
                }

                Unlock();

                return (result);
            }
            inline void GetMetaData(MetaData::Service& metaData) const
            {
                if (_moduleName.empty() == false)
                    metaData.Module = _moduleName;
                if (_versionHash.empty() == false)
                    metaData.Hash = _versionHash;

                PluginHost::Service::GetMetaData(metaData);
            }
            inline void Evaluate()
            {
                Lock();

                uint32_t subsystems = _administrator.SubSystemInfo();

                IShell::state current(State());

                // Active or not, update the condition state !!!!
                if ((_precondition.Evaluate(subsystems) == true) && (current == IShell::PRECONDITION)) {
                    if (_precondition.IsMet() == true) {

                        Unlock();

                        Activate(_reason);

                        Lock();
                    }
                }

                if ((_termination.Evaluate(subsystems) == true) && (current == IShell::ACTIVATED)) {
                    if (_termination.IsMet() == true) {

                        Unlock();

                        Deactivate(IShell::CONDITIONS);

                        Lock();
                    }
                }

                Unlock();
            }

            uint32_t Submit(const uint32_t id, const Core::ProxyType<Core::JSON::IElement>& response) override;
            ISubSystem* SubSystems() override;
            void Notify(const string& message) override;
            void* QueryInterface(const uint32_t id) override;
            void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override;
            void Register(IPlugin::INotification* sink) override;
            void Unregister(IPlugin::INotification* sink) override;

            string Version() const override {
                return (_administrator.Configuration().Version());
            }
            string Model() const override {
                return (_administrator.Configuration().Model());
            }
            bool Background() const override {
                return (_administrator.Configuration().Background());
            }
            string Accessor() const override {
                return (_administrator.Configuration().URL() + "/" + PluginHost::Service::Configuration().Callsign.Value());
            }
            string ProxyStubPath () const override {
                return (_administrator.Configuration().ProxyStubPath());
            }
            string HashKey() const override {
                return (_administrator.Configuration().HashKey());
            }
            string Substitute(const string& input) const override {
                return (_administrator.Configuration().Substitute(input, PluginHost::Service::Configuration()));
            }
            bool PostMortemAllowed(PluginHost::IShell::reason why) const {
                return (_administrator.Configuration().PostMortemAllowed(why));
            }

            // Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
            IShell::ICOMLink* COMLink() override {
                return (this);
            }
            void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& sessionId, const string& className, const string& callsign) override
            {
                ASSERT(_connection == nullptr);

                void* result(_administrator.Instantiate(object, waitTime, sessionId, className, callsign, DataPath(), PersistentPath(), VolatilePath()));

                _connection = _administrator.RemoteConnection(sessionId);

                return (result);
            }
            void Register(RPC::IRemoteConnection::INotification* sink) override
            {
                _administrator.Register(sink);
            }
            void Unregister(RPC::IRemoteConnection::INotification* sink) override
            {
                _administrator.Unregister(sink);
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId) override
            {
                return (_administrator.RemoteConnection(connectionId));
            }

            // Methods to Activate and Deactivate the aggregated Plugin to this shell.
            // These are Blocking calls!!!!!
            virtual uint32_t Activate(const reason) override;
            virtual uint32_t Deactivate(const reason) override;
            virtual reason Reason() const
            {
                return (_reason);
            }
            bool HasVersionSupport(const string& number) const
            {

                return (number.length() > 0) && (std::all_of(number.begin(), number.end(), [](TCHAR item) { return std::isdigit(item); })) && (Service::IsSupported(static_cast<uint8_t>(atoi(number.c_str()))));
            }

        private:
            inline IPlugin* CheckLibrary(const string& name, const TCHAR* className, const uint32_t version)
            {
                IPlugin* newIF = nullptr;
                Core::File libraryToLoad(name, true);

                if (libraryToLoad.Exists() != true) {
                    if (HasError() == false) {
                        ErrorMessage(_T("library does not exist"));
                    }
                } else {
                    Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
                    Core::Library myLib(name.c_str());

                    if (myLib.IsLoaded() == false) {
                        if ((HasError() == false) || (ErrorMessage().substr(0, 7) == _T("library"))) {
                            ErrorMessage(myLib.Error());
                        }
                    } else if ((newIF = admin.Instantiate<IPlugin>(myLib, className, version)) == nullptr) {
                        ErrorMessage(_T("class definitions does not exist"));
                    } else {
                        Core::System::ModuleNameImpl moduleName = reinterpret_cast<Core::System::ModuleNameImpl>(myLib.LoadFunction(_T("ModuleName")));
                        Core::System::ModuleBuildRefImpl moduleBuildRef = reinterpret_cast<Core::System::ModuleBuildRefImpl>(myLib.LoadFunction(_T("ModuleBuildRef")));

                        if (moduleName != nullptr) {
                            _moduleName = moduleName();
                        }
                        if (moduleBuildRef != nullptr) {
                            _versionHash = moduleBuildRef();
                        }
                    }
                }
                return (newIF);
            }

            void AquireInterfaces()
            {
                ASSERT((State() == DEACTIVATED) || (State() == PRECONDITION));

                IPlugin* newIF = nullptr;
                const string locator(PluginHost::Service::Configuration().Locator.Value());
                const string classNameString(PluginHost::Service::Configuration().ClassName.Value());
                const TCHAR* className(classNameString.c_str());
                uint32_t version(static_cast<uint32_t>(~0));

                _moduleName.clear();
                _versionHash.clear();

                if (locator.empty() == true) {
                    Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
                    newIF = admin.Instantiate<IPlugin>(Core::Library(), className, version);
                } else {
                    if ((newIF = CheckLibrary((_administrator.Configuration().PersistentPath() + locator), className, version)) == nullptr) {
                        if ((newIF = CheckLibrary((_administrator.Configuration().SystemPath() + locator), className, version)) == nullptr) {
                            newIF = CheckLibrary((_administrator.Configuration().AppPath() + _T("Plugins/") + locator), className, version);
                        }
                    }
                }

                if (newIF != nullptr) {
                    _extended = newIF->QueryInterface<IPluginExtended>();
                    _webRequest = newIF->QueryInterface<IWeb>();
                    _webSocket = newIF->QueryInterface<IWebSocket>();
                    _textSocket = newIF->QueryInterface<ITextSocket>();
                    _rawSocket = newIF->QueryInterface<IChannel>();
                    _webSecurity = newIF->QueryInterface<ISecurity>();
                    _jsonrpc = newIF->QueryInterface<IDispatcher>();
                    if (_webSecurity == nullptr) {
                        _webSecurity = _administrator.Configuration().Security();
                        _webSecurity->AddRef();
                    }

                    _pluginHandling.Lock();
                    _handler = newIF;
                    _pluginHandling.Unlock();
                }
            }
            void ReleaseInterfaces()
            {
                _pluginHandling.Lock();

                ASSERT(State() != ACTIVATED);
                ASSERT(_handler != nullptr);

                IPlugin* currentIF = _handler;

                if (_webRequest != nullptr) {
                    _webRequest->Release();
                    _webRequest = nullptr;
                }
                if (_webSocket != nullptr) {
                    _webSocket->Release();
                    _webSocket = nullptr;
                }
                if (_textSocket != nullptr) {
                    _textSocket->Release();
                    _textSocket = nullptr;
                }
                if (_rawSocket != nullptr) {
                    _rawSocket->Release();
                    _rawSocket = nullptr;
                }
                if (_webSecurity != nullptr) {
                    _webSecurity->Release();
                    _webSecurity = nullptr;
                }
                if (_extended != nullptr) {
                    _extended->Release();
                    _extended = nullptr;
                }
                if (_jsonrpc != nullptr) {
                    _jsonrpc->Release();
                    _jsonrpc = nullptr;
                }

                _handler = nullptr;

                _pluginHandling.Unlock();

                currentIF->Release();

                // Could be that we can now drop the dynamic library...
                Core::ServiceAdministrator::Instance().FlushLibraries();
            }

        private:
            Core::CriticalSection _pluginHandling;

            // The handlers that implement the actual logic behind the service
            IPlugin* _handler;
            IPluginExtended* _extended;
            IWeb* _webRequest;
            IWebSocket* _webSocket;
            ITextSocket* _textSocket;
            IChannel* _rawSocket;
            ISecurity* _webSecurity;
            IDispatcher* _jsonrpc;
            reason _reason;
            string _moduleName;
            string _versionHash;
            Condition _precondition;
            Condition _termination;
            uint32_t _activity;
            RPC::IRemoteConnection* _connection;

            ServiceMap& _administrator;
            static Core::ProxyType<Web::Response> _unavailableHandler;
            static Core::ProxyType<Web::Response> _missingHandler;
        };
        class ServiceMap {
        public:
            typedef Core::IteratorMapType<std::map<const string, Core::ProxyType<Service>>, Core::ProxyType<Service>, const string&> Iterator;
            typedef std::map<const string, IRemoteInstantiation*> RemoteInstantiators;

        private:
            ServiceMap() = delete;
            ServiceMap(const ServiceMap&) = delete;
            ServiceMap& operator=(const ServiceMap&) = delete;

            class CommunicatorServer : public RPC::Communicator {
            private:
                class RemoteHost : public RPC::Communicator::RemoteConnection {
                private:
                    friend class Core::Service<RemoteHost>;

                    RemoteHost(const RemoteHost&) = delete;
                    RemoteHost& operator=(const RemoteHost&) = delete;

                public:
                    RemoteHost(const RPC::Object& instance, const RPC::Config& config)
                        : RemoteConnection()
                        , _object(instance)
                        , _config(config)
                    {
                    }
                    virtual ~RemoteHost()
                    {
                        TRACE_L1("Destructor for RemoteHost process for %d", Id());
                    }

                    uint32_t Launch() override
                    {
                        uint32_t result = Core::ERROR_INVALID_DESIGNATOR;

                        Core::NodeId remoteNode(_object.RemoteAddress());

                        if (remoteNode.IsValid() == true) {
                            Core::ProxyType<Core::IIPCServer> engine(Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance()));
                            Core::ProxyType<RPC::CommunicatorClient> client(
                                Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, engine));

                            // Oke we have ou selves a COMClient link. Lets see if we can get the proepr interface...
                            IRemoteInstantiation* instantiation = client->Open<IRemoteInstantiation>(_config.Connector(), ~0, 3000);

                            if (instantiation == nullptr) {
                                result = Core::ERROR_ILLEGAL_STATE;
                            } else {
                                result = instantiation->Instantiate(
                                    RPC::Communicator::RemoteConnection::Id(),
                                    _object.Locator(),
                                    _object.ClassName(),
                                    _object.Callsign(),
                                    _object.Interface(),
                                    _object.Version(),
                                    _object.User(),
                                    _object.Group(),
                                    _object.Threads(),
                                    _object.Priority(),
                                    _object.Configuration());

                                instantiation->Release();
                            }
                        }

                        return (result);
                    }

                private:
                    RPC::Object _object;
                    const RPC::Config& _config;
                };

            public:
                CommunicatorServer() = delete;
                CommunicatorServer(const CommunicatorServer&) = delete;
                CommunicatorServer& operator=(const CommunicatorServer&) = delete;
                CommunicatorServer(
                    ServiceMap& parent,
                    const Core::NodeId& node,
                    const string& persistentPath,
                    const string& systemPath,
                    const string& dataPath,
                    const string& volatilePath,
                    const string& appPath,
                    const string& proxyStubPath,
                    const string& postMortemPath,
                    const Core::ProxyType<RPC::InvokeServer>& handler)
                    : RPC::Communicator(node, proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                    , _parent(parent)
                    , _persistentPath(persistentPath.empty() == false ? Core::Directory::Normalize(persistentPath) : persistentPath)
                    , _systemPath(systemPath.empty() == false ? Core::Directory::Normalize(systemPath) : systemPath)
                    , _dataPath(dataPath.empty() == false ? Core::Directory::Normalize(dataPath) : dataPath)
                    , _volatilePath(volatilePath.empty() == false ? Core::Directory::Normalize(volatilePath) : volatilePath)
                    , _appPath(appPath.empty() == false ? Core::Directory::Normalize(appPath) : appPath)
                    , _proxyStubPath(proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath)
                    , _postMortemPath(postMortemPath.empty() == false ? Core::Directory::Normalize(postMortemPath) : postMortemPath)
#ifdef __WINDOWS__
                    , _application(_systemPath + EXPAND_AND_QUOTE(HOSTING_COMPROCESS))
#else
                    , _application(EXPAND_AND_QUOTE(HOSTING_COMPROCESS))
#endif
                    , _adminLock()
                {
                    // Make sure the engine knows how to call the Announcment handler..
                    handler->Announcements(Announcement());

                    if (RPC::Communicator::Open(RPC::CommunicationTimeOut) != Core::ERROR_NONE) {
                        TRACE_L1("We can not open the RPC server. No out-of-process communication available. %d", __LINE__);
                    } else {
                        // We need to pass the communication channel NodeId via an environment variable, for process,
                        // not being started by the rpcprocess...
                        Core::SystemInfo::SetEnvironment(string(CommunicatorConnector), RPC::Communicator::Connector());
                    }
                }
                virtual ~CommunicatorServer()
                {
                }

            public:
                void* Create(uint32_t& connectionId, const RPC::Object& instance, const string& classname, const string& callsign, const uint32_t waitTime, const string& dataPath, const string& persistentPath, const string& volatilePath)
                {
                    return (RPC::Communicator::Create(connectionId, instance, RPC::Config(RPC::Communicator::Connector(), _application, persistentPath, _systemPath, dataPath, volatilePath, _appPath, _proxyStubPath, _postMortemPath), waitTime));
                }
                const string& PersistentPath() const
                {
                    return (_persistentPath);
                }
                const string& SystemPath() const
                {
                    return (_systemPath);
                }
                const string& DataPath() const
                {
                    return (_dataPath);
                }
                const string& VolatilePath() const
                {
                    return (_volatilePath);
                }
                const string& AppPath() const
                {
                    return (_appPath);
                }
                const string& ProxyStubPath() const
                {
                    return (_proxyStubPath);
                }
                const string& PostMortemPath() const
                {
                    return (_postMortemPath);
                }
                const string& Application() const
                {
                    return (_application);
                }

            private:
                RPC::Communicator::RemoteConnection* CreateStarter(const RPC::Config& config, const RPC::Object& instance) override
                {
                    RPC::Communicator::RemoteConnection* result = nullptr;

                    if (instance.Type() == RPC::Object::HostType::DISTRIBUTED) {
                        result = Core::Service<RemoteHost>::Create<RPC::Communicator::RemoteConnection>(instance, config);
                    } else {
                        result = RPC::Communicator::CreateStarter(config, instance);
                    }

                    return result;
                }

                void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t version) override
                {
                    return (_parent.Aquire(interfaceId, className, version));
                }

            private:
                ServiceMap& _parent;
                const string _persistentPath;
                const string _systemPath;
                const string _dataPath;
                const string _volatilePath;
                const string _appPath;
                const string _proxyStubPath;
                const string _postMortemPath;
                const string _application;
                mutable Core::CriticalSection _adminLock;
            };
            class RemoteInstantiation : public IRemoteInstantiation {
            private:
                RemoteInstantiation(ServiceMap& parent, const CommunicatorServer& comms, const string& connector)
                    : _refCount(1)
                    , _parent(parent)
                    , _comms(comms)
                    , _connector(connector)
                {
                }

            public:
                ~RemoteInstantiation() override
                {
                }

            public:
                static IRemoteInstantiation* Create(ServiceMap& parent, const CommunicatorServer& comms, const string& connector)
                {
                    return (new RemoteInstantiation(parent, comms, connector));
                }
                void AddRef() const override
                {
                    Core::InterlockedIncrement(_refCount);
                }
                uint32_t Release() const override
                {
                    _parent._adminLock.Lock();

                    if (Core::InterlockedDecrement(_refCount) == 0) {
                        _parent.Remove(_connector);

                        _parent._adminLock.Unlock();

                        delete this;

                        return (Core::ERROR_DESTRUCTION_SUCCEEDED);
                    } else {
                        _parent._adminLock.Unlock();
                    }

                    return (Core::ERROR_NONE);
                }
                uint32_t Instantiate(
                    const uint32_t requestId,
                    const string& libraryName,
                    const string& className,
                    const string& callsign,
                    const uint32_t interfaceId,
                    const uint32_t version,
                    const string& user,
                    const string& group,
                    const uint8_t threads,
                    const int8_t priority,
                    const string configuration) override
                {
                    string persistentPath(_comms.PersistentPath());
                    string dataPath(_comms.DataPath());
                    string volatilePath(_comms.VolatilePath());

                    if (callsign.empty() == false) {
                        dataPath += callsign + '/';
                        persistentPath += callsign + '/';
                        volatilePath += callsign + '/';
                    }

                    uint32_t id;
                    RPC::Config config(_connector, _comms.Application(), persistentPath, _comms.SystemPath(), dataPath, volatilePath, _comms.AppPath(), _comms.ProxyStubPath(), _comms.PostMortemPath());
                    RPC::Object instance(libraryName, className, callsign, interfaceId, version, user, group, threads, priority, RPC::Object::HostType::LOCAL, _T(""), configuration);

                    RPC::Process process(requestId, config, instance);

                    return (process.Launch(id));
                }

                BEGIN_INTERFACE_MAP(RemoteInstantiation)
                INTERFACE_ENTRY(IRemoteInstantiation)
                END_INTERFACE_MAP

            private:
                mutable uint32_t _refCount;
                ServiceMap& _parent;
                const CommunicatorServer& _comms;
                const string _connector;
            };
            class Override : public Core::JSON::Container {
            private:
                Override(const Override&) = delete;
                Override& operator=(const Override&) = delete;

            public:
                class Plugin : public Core::JSON::Container {
                private:
                    Plugin& operator=(Plugin const& other) = delete;

                public:
                    Plugin()
                        : Core::JSON::Container()
                        , AutoStart()
                        , Configuration(_T("{}"), false)
                    {
                        Add(_T("autostart"), &AutoStart);
                        Add(_T("configuration"), &Configuration);
                    }
                    Plugin(const string& config, const bool autoStart)
                        : Core::JSON::Container()
                        , AutoStart(autoStart)
                        , Configuration(config, false)
                    {
                        Add(_T("autostart"), &AutoStart);
                        Add(_T("configuration"), &Configuration);
                    }
                    Plugin(Plugin const& copy)
                        : Core::JSON::Container()
                        , AutoStart(copy.AutoStart)
                        , Configuration(copy.Configuration)
                    {
                        Add(_T("autostart"), &AutoStart);
                        Add(_T("configuration"), &Configuration);
                    }

                    virtual ~Plugin()
                    {
                    }

                public:
                    Core::JSON::Boolean AutoStart;
                    Core::JSON::String Configuration;
                };

                typedef std::map<string, Plugin>::iterator Iterator;

            public:
                Override(ServiceMap& services, const string& persitentFile)
                    : _services(services)
                    , _fileName(persitentFile)
                    , _callsigns()
                {

                    // Add all service names (callsigns) that are not yet in there...
                    ServiceMap::Iterator service(services.Services());

                    while (service.Next() == true) {
                        const string& name(service->Callsign());

                        // Create an element for this service with its callsign
                        std::pair<Iterator, bool> index(_callsigns.insert(std::pair<string, Plugin>(name, Plugin(_T("{}"), false))));

                        // Store the override config in the JSON String created in the map
                        Add(index.first->first.c_str(), &(index.first->second));
                    }
                }
                ~Override()
                {
                }

            public:
                uint32_t Load()
                {

                    uint32_t result = Core::ERROR_NONE;

                    Core::File storage(_fileName);

                    if ((storage.Exists() == true) && (storage.Open(true) == true)) {

                        result = true;

                        // Clear all currently set values, they might be from the precious run.
                        Clear();

                        // Red the file and parse it into this object.
                        IElement::FromFile(storage);

                        // Convey the real JSON struct information into the specific services.
                        ServiceMap::Iterator index(_services.Services());

                        while (index.Next() == true) {

                            std::map<string, Plugin>::const_iterator current(_callsigns.find(index->Callsign()));

                            // ServiceMap should *NOT* change runtime...
                            ASSERT(current != _callsigns.end());

                            if (current->second.IsSet() == true) {
                                if (current->second.Configuration.IsSet() == true) {
                                    (*index)->Configuration(current->second.Configuration.Value());
                                }
                                if (current->second.AutoStart.IsSet() == true) {
                                    (*index)->AutoStart(current->second.AutoStart.Value());
                                }
                            }
                        }

                        storage.Close();
                    } else {
                        result = storage.ErrorCode();
                    }

                    return (result);
                }

                bool Save()
                {

                    uint32_t result = Core::ERROR_NONE;

                    Core::File storage(_fileName);

                    if (storage.Create() == true) {

                        // Clear all currently set values, they might be from the precious run.
                        Clear();

                        // Convey the real information from he specific services into the JSON struct.
                        ServiceMap::Iterator index(_services.Services());

                        while (index.Next() == true) {

                            std::map<string, Plugin>::iterator current(_callsigns.find(index->Callsign()));

                            // ServiceMap should *NOT* change runtime...
                            ASSERT(current != _callsigns.end());

                            string config((*index)->Configuration());

                            if (config.empty() == true) {
                                current->second.Configuration = _T("{}");
                            } else {
                                current->second.Configuration = config;
                            }
                            current->second.AutoStart = (index)->AutoStart();
                        }

                        // Persist the currently set information
                        IElement::ToFile(storage);

                        storage.Close();
                    } else {
                        result = storage.ErrorCode();
                    }

                    return (result);
                }

            private:
                ServiceMap& _services;
                Core::string _fileName;
                std::map<string, Plugin> _callsigns;
            };
            class SubSystems : public Core::IDispatch, public SystemInfo {
            private:
                SubSystems() = delete;
                SubSystems(const SubSystems&) = delete;
                SubSystems& operator=(const SubSystems&) = delete;

                class Job : public Core::IDispatchType<void> {
                private:
                    Job() = delete;
                    Job(const Job&) = delete;
                    Job& operator=(const Job&) = delete;

                public:
                    Job(SubSystems* parent)
                        : _parent(*parent)
                        , _schedule(false)
                    {
                        ASSERT(parent != nullptr);
                    }
                    virtual ~Job()
                    {
                    }

                public:
                    void Schedule()
                    {
                        if (_schedule == false) {
                            _schedule = true;
                            _parent.WorkerPool().Submit(Core::ProxyType<Core::IDispatchType<void>>(*this));
                        }
                    }
                    virtual void Dispatch()
                    {
                        _schedule = false;
                        _parent.Evaluate();
                    }

                private:
                    SubSystems& _parent;
                    bool _schedule;
                };

            public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
                SubSystems(ServiceMap* parent)
                    : SystemInfo(this)
                    , _parent(*parent)
                    , _decoupling(Core::ProxyType<Job>::Create(this))
                {
                }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
                virtual ~SubSystems()
                {
                    _parent.WorkerPool().Revoke(Core::ProxyType<Core::IDispatch>(_decoupling));
                }

            private:
                virtual void Dispatch() override
                {
                    static uint32_t previousState = 0;
                    uint32_t changedFlags = (previousState ^ Value());
                    previousState = Value();

                    if ((changedFlags & (1 << ISubSystem::NETWORK)) != 0) {
                         const_cast<PluginHost::Config&>(_parent.Configuration()).UpdateAccessor();
                    }

                    if ((changedFlags & (1 << ISubSystem::SECURITY)) != 0) {
                        _parent.Security(SystemInfo::IsActive(ISubSystem::SECURITY));
                    }

                    _decoupling->Schedule();
                }
                inline void Evaluate()
                {
                    _parent.Evaluate();
                }
                inline Core::WorkerPool& WorkerPool()
                {
                    return (_parent.WorkerPool());
                }

            private:
                ServiceMap& _parent;
                Core::ProxyType<Job> _decoupling;
            };

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            ServiceMap(Server& server, Config& config, const uint32_t stackSize)
                : _webbridgeConfig(config)
                , _adminLock()
                , _notificationLock()
                , _services()
                , _notifiers()
                , _engine(Core::ProxyType<RPC::InvokeServer>::Create(&(server._dispatcher)))
                , _processAdministrator(
                    *this, 
                    config.Communicator(), 
                    config.PersistentPath(), 
                    config.SystemPath(), 
                    config.DataPath(), 
                    config.VolatilePath(), 
                    config.AppPath(), 
                    config.ProxyStubPath(), 
                    config.PostMortemPath(), 
                    _engine)
                , _server(server)
                , _subSystems(this)
                , _authenticationHandler(nullptr)
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~ServiceMap()
            {
                // Make sure all services are deactivated before we are killed (call Destroy on this object);
                ASSERT(_services.size() == 0);
            }

        public:
            inline void Security(const bool enabled)
            {
                _adminLock.Lock();

                if ((_authenticationHandler == nullptr) ^ (enabled == false)) {
                    if (_authenticationHandler == nullptr) {
                        // Let get the AuthentcationHandler.
                        _authenticationHandler = reinterpret_cast<IAuthenticate*>(QueryInterfaceByCallsign(IAuthenticate::ID, _subSystems.SecurityCallsign()));
                    } else {
                        // Remove the security from all the channels.
                        _server.Dispatcher().SecurityRevoke(_webbridgeConfig.Security());
                    }
                }

                _adminLock.Unlock();
            }
            inline ISecurity* Officer(const string& token)
            {
                ISecurity* result;

                _adminLock.Lock();

                if (_authenticationHandler != nullptr) {
                    result = _authenticationHandler->Officer(token);
                } else {
                    result = _webbridgeConfig.Security();
                }

                _adminLock.Unlock();
                return (result);
            }
            inline uint32_t Submit(const uint32_t id, const Core::ProxyType<Core::JSON::IElement>& response)
            {
                return (_server.Dispatcher().Submit(id, response));
            }
            inline uint32_t SubSystemInfo() const
            {
                return (_subSystems.Value());
            }
            inline ISubSystem* SubSystemsInterface()
            {
                return (reinterpret_cast<ISubSystem*>(_subSystems.QueryInterface(ISubSystem::ID)));
            }
            void StateChange(PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                std::list<PluginHost::IPlugin::INotification*> currentlist(_notifiers);

                while (currentlist.size()) {
                    currentlist.front()->StateChange(entry);
                    currentlist.pop_front();
                }

                _notificationLock.Unlock();
            }
            void Register(PluginHost::IPlugin::INotification* sink)
            {
                _notificationLock.Lock();

                ASSERT(std::find(_notifiers.begin(), _notifiers.end(), sink) == _notifiers.end());

                sink->AddRef();
                _notifiers.push_back(sink);

                // Tell this "new" sink all our actived plugins..
                std::map<const string, Core::ProxyType<Service>>::iterator index(_services.begin());

                // Notifty all plugins that we have sofar..
                while (index != _services.end()) {
                    ASSERT(index->second.IsValid());

                    Core::ProxyType<Service> service(index->second);

                    ASSERT(service.IsValid());

                    if ( (service.IsValid() == true) && (service->State() == IShell::ACTIVATED) ) {
                        sink->StateChange(&(service.operator*()));
                    }

                    index++;
                }

                _notificationLock.Unlock();
            }
            void Unregister(PluginHost::IPlugin::INotification* sink)
            {
                _notificationLock.Lock();

                std::list<PluginHost::IPlugin::INotification*>::iterator index(std::find(_notifiers.begin(), _notifiers.end(), sink));

                if (index != _notifiers.end()) {
                    (*index)->Release();
                    _notifiers.erase(index);
                }

                _notificationLock.Unlock();
            }
            inline void* QueryInterfaceByCallsign(const uint32_t id, const string& name)
            {
                void* result = nullptr;

                const string callsign(name.empty() == true ? _server.Controller()->Callsign() : name);

                Core::ProxyType<Service> service;

                FromIdentifier(callsign, service);

                if (service.IsValid() == true) {

                    result = service->QueryInterface(id);
                }
                return (result);
            }

            void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& sessionId, const string& className, const string& callsign, const string& dataPath, const string& persistentPath, const string& volatilePath)
            {
                return (_processAdministrator.Create(sessionId, object, className, callsign, waitTime, dataPath, persistentPath, volatilePath));
            }
            void Register(RPC::IRemoteConnection::INotification* sink)
            {
                _processAdministrator.Register(sink);
            }
            void Unregister(RPC::IRemoteConnection::INotification* sink)
            {
                _processAdministrator.Unregister(sink);
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId)
            {
                return (connectionId != 0 ? _processAdministrator.Connection(connectionId) : nullptr);
            }
            uint32_t Persist()
            {
                Override infoBlob(*this, _webbridgeConfig.PersistentPath() + PluginOverrideFile);

                return (infoBlob.Save());
            }
            uint32_t Load()
            {
                Override infoBlob(*this, _webbridgeConfig.PersistentPath() + PluginOverrideFile);

                return (infoBlob.Load());
            }
            inline Core::ProxyType<Service> Insert(const Plugin::Config& configuration)
            {
                // Whatever plugin is needse, we at least have our MetaData plugin available (as the first entry :-).
                Core::ProxyType<Service> newService(Core::ProxyType<Service>::Create(_webbridgeConfig, configuration, *this));

                if (newService.IsValid() == true) {
                    _adminLock.Lock();

                    // Fire up the interface. Let it handle the messages.
                    _services.insert(std::pair<const string, Core::ProxyType<Service>>(configuration.Callsign.Value(), newService));

                    _adminLock.Unlock();
                }

                return (newService);
            }

            inline uint32_t Clone(const Core::ProxyType<Service>& original, const string& newCallsign, Core::ProxyType<Service>& newService)
            {
                uint32_t result = Core::ERROR_GENERAL;

                ASSERT(original.IsValid());

                _adminLock.Lock();
                if (_services.find(newCallsign) == _services.end()) {
                    // Copy original configuration
                    Plugin::Config newConfiguration = original->PluginHost::Service::Configuration();
                    newConfiguration.Callsign = newCallsign;

                    newService = Core::ProxyType<Service>::Create(_webbridgeConfig, newConfiguration, *this);

                    if (newService.IsValid() == true) {
                        // Fire up the interface. Let it handle the messages.
                        _services.insert(std::pair<const string, Core::ProxyType<Service>>(newConfiguration.Callsign.Value(), newService));

                        newService->Evaluate();

                        result = Core::ERROR_NONE;
                    }
                }
                _adminLock.Unlock();

                return (result);
            }

            inline void Destroy(const string& callSign)
            {
                _adminLock.Lock();

                // First stop all services running ...
                std::map<const string, Core::ProxyType<Service>>::iterator index(_services.find(callSign));

                if (index != _services.end()) {
                    index->second->Destroy();
                    _services.erase(index);
                }

                _adminLock.Unlock();
            }
            inline Iterator Services()
            {
                return (Iterator(_services));
            }
            inline void Notification(const ForwardMessage& message)
            {
                _server.Notification(message);
            }
#if THUNDER_RESTFULL_API
            inline void Notification(const string& message)
            {
                _server.Controller()->Notification(message);
            }
#endif
            void GetMetaData(Core::JSON::ArrayType<MetaData::Service>& metaData) const
            {
                _adminLock.Lock();

                std::list<Core::ProxyType<Service>> duplicates;
                std::map<const string, Core::ProxyType<Service>>::const_iterator index(_services.begin());

                while (index != _services.end()) {
                    duplicates.push_back(index->second);
                    index++;
                }

                _adminLock.Unlock();

                while (duplicates.size() > 0) {
                    MetaData::Service newInfo;
                    duplicates.front()->GetMetaData(newInfo);
                    metaData.Add(newInfo);
                    duplicates.pop_front();
                }
            }
            uint32_t FromIdentifier(const string& callSign, Core::ProxyType<Service>& service)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                _adminLock.Lock();

                for (auto index : _services) {
                    const string& source(index.first);
                    uint32_t length = static_cast<uint32_t>(source.length());

                    if (callSign.compare(0, source.length(), source) == 0) {
                        if (callSign.length() == length) {
                            // Service found, did not requested specific version
                            service = index.second;
                            result = Core::ERROR_NONE;
                            break;
                        } else if (callSign[length] == '.') {
                            // Requested specific version of a plugin
                            if (index.second->HasVersionSupport(callSign.substr(length + 1)) == true) {
                                // Requested version of service is supported!
                                service = index.second;
                                result = Core::ERROR_NONE;
                            } else {
                                // Requested version is not supported
                                result = Core::ERROR_INVALID_SIGNATURE;
                            }
                            break;
                        }
                    }
                }

                _adminLock.Unlock();

                return (result);
            }
            uint32_t FromLocator(const string& identifier, Core::ProxyType<Service>& service, bool& serviceCall);

            void Destroy();

            inline const PluginHost::Config& Configuration() const
            {
                return _server.Configuration();
            }

        private:
            void Remove(const string& connector) const
            {
                // This is already locked by the callee, so safe to operate on the map..
                RemoteInstantiators::const_iterator index(_instantiators.find(connector));

                if (index == _instantiators.cend()) {
                    _instantiators.erase(index);
                }
            }
            void* Aquire(const uint32_t interfaceId, const string& className, const uint32_t version)
            {
                void* result = nullptr;

                if ((interfaceId == IRemoteInstantiation::ID) && (version == static_cast<uint32_t>(~0))) {

                    _adminLock.Lock();
                    // className == Connector..
                    RemoteInstantiators::iterator index(_instantiators.find(className));

                    if (index == _instantiators.end()) {
                        IRemoteInstantiation* newIF = RemoteInstantiation::Create(*this, _processAdministrator, className);

                        ASSERT(newIF != nullptr);

                        _instantiators.emplace(std::piecewise_construct,
                            std::make_tuple(className),
                            std::make_tuple(newIF));

                        result = newIF;
                    } else {
                        result = index->second->QueryInterface(IRemoteInstantiation::ID);
                    }

                    _adminLock.Unlock();
                } else {
                    result = QueryInterfaceByCallsign(interfaceId, className);
                }

                return (result);
            }
            void RecursiveNotification(std::map<const string, Core::ProxyType<Service>>::iterator& index)
            {
                if (index != _services.end()) {
                    Core::ProxyType<Service> element(index->second);
                    index++;
                    RecursiveNotification(index);
                    element->Evaluate();
                } else {
                    _adminLock.Unlock();
                }
            }
            void Evaluate()
            {
                _adminLock.Lock();

                // First stop all services running ...
                std::map<const string, Core::ProxyType<Service>>::iterator index(_services.begin());

                RecursiveNotification(index);
            }
            inline Core::WorkerPool& WorkerPool()
            {
                return (_server.WorkerPool());
            }

        private:
            Config& _webbridgeConfig;
            mutable Core::CriticalSection _adminLock;
            Core::CriticalSection _notificationLock;
            std::map<const string, Core::ProxyType<Service>> _services;
            mutable RemoteInstantiators _instantiators;
            std::list<IPlugin::INotification*> _notifiers;
            Core::ProxyType<RPC::InvokeServer> _engine;
            CommunicatorServer _processAdministrator;
            Server& _server;
            Core::Sink<SubSystems> _subSystems;
            IAuthenticate* _authenticationHandler;
        };

        // Connection handler is the listening socket and keeps track of all open
        // Links. A Channel is identified by an ID, this way, whenever a link dies
        // (is closed) during the service process, the ChannelMap will
        // not find it and just "flush" the presented work.
        class Channel : public PluginHost::Channel {
        private:
            class Job : public Core::IDispatch {
            public:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                Job(Server* server)
                    : _ID(~0)
                    , _server(server)
                    , _service()
                {
                    ASSERT(server != nullptr);
                }
                ~Job() override
                {
                    if (_service.IsValid()) {
                        _service.Release();
                    }
                    _server = nullptr;
                }

            public:
                void Close()
                {
                    TRACE(Activity, (_T("HTTP Request with direct close on [%d]"), _ID));
                    _server->Dispatcher().Suspend(_ID);
                }
                bool HasService() const
                {
                    return (_service.IsValid());
                }
                void Clear()
                {
                    _ID = ~0;
                    if (_service.IsValid() == true) {
                        _service.Release();
                    }
                }
                void Set(const uint32_t id, Core::ProxyType<Service>& service)
                {
                    ASSERT(_service.IsValid() == false);
                    ASSERT(_ID == static_cast<uint32_t>(~0));
                    ASSERT(id != static_cast<uint32_t>(~0));

                    _ID = id;
                    _service = service;
                }
                string Process(const string& message)
                {
                    return (_service->Inbound(_ID, message));
                }
                Core::ProxyType<Core::JSONRPC::Message> Process(const string& token, const Core::ProxyType<Core::JSONRPC::Message>& message)
                {
                    return (_service->Invoke(token, _ID, *message));
                }
                Core::ProxyType<Web::Response> Process(const Core::ProxyType<Web::Request>& message)
                {
                    return (_service->Process(*message));
                }
                Core::ProxyType<Core::JSON::IElement> Process(const Core::ProxyType<Core::JSON::IElement>& message)
                {
                    return (_service->Inbound(_ID, message));
                }
                template <typename PACKAGE>
                void Submit(PACKAGE package)
                {
                    _server->Dispatcher().Submit(_ID, package);
                }
                void RequestClose() {
                    _server->Dispatcher().RequestClose(_ID);
                }

            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
            };
            class WebRequestJob : public Job {
            public:
                WebRequestJob() = delete;
                WebRequestJob(const WebRequestJob&) = delete;
                WebRequestJob& operator=(const WebRequestJob&) = delete;

                WebRequestJob(Server* server)
                    : Job(server)
                    , _request()
                    , _jsonrpc(false)
                {
                }
                ~WebRequestJob() override
                {
                    ASSERT(_request.IsValid() == false);

                    if (_request.IsValid()) {
                        _request.Release();
                    }
                }

            public:
                static void Initialize()
                {
                    _missingResponse->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                    _missingResponse->Message = _T("There is no response from the requested service.");
                }
                void Set(const uint32_t id, Core::ProxyType<Service>& service, Core::ProxyType<Web::Request>& request, const string& token, const bool JSONRPC)
                {
                    Job::Set(id, service);

                    ASSERT(_request.IsValid() == false);

                    _request = request;
                    _jsonrpc = JSONRPC && (_request->HasBody() == true);
                    _token = token;
                }
                void Dispatch() override
                {
                    ASSERT(_request.IsValid());
                    ASSERT(Job::HasService() == true);

                    Core::ProxyType<Web::Response> response;

                    if (_jsonrpc == true) {
                        Core::ProxyType<Core::JSONRPC::Message> message(_request->Body<Core::JSONRPC::Message>());

                        if (message->IsSet()) {
                            Core::ProxyType<Core::JSONRPC::Message> body = Job::Process(_token, message);

                            // If we have no response body, it looks like an async-call...
                            if (body.IsValid() == false) {
                                // It's a a-synchronous call, seems we should just queue this request, it will be answered later on..
                                if (_request->Connection.Value() == Web::Request::CONNECTION_CLOSE) {
                                    Job::RequestClose();
                                }
                            }
                            else {
                                response = IFactories::Instance().Response();
                                response->Body(body);
                                if (body->Error.IsSet() == false) {
                                    response->ErrorCode = Web::STATUS_OK;
                                    response->Message = _T("JSONRPC executed succesfully");
                                } else {
                                    response->ErrorCode = Web::STATUS_ACCEPTED;
                                    response->Message = _T("Failure on JSONRPC: ") + Core::NumberType<uint32_t>(body->Error.Code).Text();
                                }
                            }
                        } else {
                            response = IFactories::Instance().Response();
                            response->ErrorCode = Web::STATUS_ACCEPTED;
                            response->Message = _T("Failed to parse JSONRPC message");
                        }
                    } else {
                        response = Job::Process(_request);
                        if (response.IsValid() == false) {
                            response = _missingResponse;
                        }
                    }

                    if (response.IsValid() == true) {
                        // Seems we can handle..
                        if (response->AccessControlOrigin.IsSet() == false)
                            response->AccessControlOrigin = _T("*");

                        if (response->CacheControl.IsSet() == false)
                            response->CacheControl = _T("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0");

                        Job::Submit(response);

                        if (_request->Connection.Value() == Web::Request::CONNECTION_CLOSE) {
                            Job::Close();
                        }
                    }

                    // We are done, clear all info
                    _request.Release();

                    Job::Clear();
                }

            private:
                Core::ProxyType<Web::Request> _request;
                string _token;
                bool _jsonrpc;

                static Core::ProxyType<Web::Response> _missingResponse;
            };
            class JSONElementJob : public Job {
            public:
                JSONElementJob() = delete;
                JSONElementJob(const JSONElementJob&) = delete;
                JSONElementJob& operator=(const JSONElementJob&) = delete;

                JSONElementJob(Server* server)
                    : Job(server)
                    , _element()
                    , _jsonrpc(false)
                {
                }
                ~JSONElementJob() override
                {
                    ASSERT(_element.IsValid() == false);

                    if (_element.IsValid()) {
                        _element.Release();
                    }
                }

            public:
                void Set(const uint32_t id, Core::ProxyType<Service>& service, Core::ProxyType<Core::JSON::IElement>& element, const string& token, const bool JSONRPC)
                {
                    Job::Set(id, service);

                    ASSERT(_element.IsValid() == false);

                    _element = element;
                    _jsonrpc = JSONRPC;
                    _token = token;
                }
                virtual void Dispatch()
                {
                    ASSERT(Job::HasService() == true);
                    ASSERT(_element.IsValid() == true);

                    if (_jsonrpc == true) {
#if THUNDER_PERFORMANCE
                        Core::ProxyType<TrackingJSONRPC> tracking (Core::proxy_cast<TrackingJSONRPC>(_element));
                        ASSERT (tracking.IsValid() == true);
			tracking->Dispatch();
#endif
                        Core::ProxyType<Core::JSONRPC::Message> message(Core::proxy_cast<Core::JSONRPC::Message>(_element));
                        ASSERT(message.IsValid() == true);

                        _element = Core::ProxyType<Core::JSON::IElement>(Job::Process(_token, message));

#if THUNDER_PERFORMANCE
			tracking->Execution();
#endif

                    } else {
                        _element = Job::Process(_element);
                    }

                    if (_element.IsValid()) {
                        // Fire and forget, We are done !!!
                        Job::Submit(_element);
                        _element.Release();
                    }

                    Job::Clear();
                }

            private:
                Core::ProxyType<Core::JSON::IElement> _element;
                string _token;
                bool _jsonrpc;
            };
            class TextJob : public Job {
            public:
                TextJob() = delete;
                TextJob(const TextJob&) = delete;
                TextJob& operator=(const TextJob&) = delete;

                TextJob(Server* server)
                    : Job(server)
                    , _text()
                {
                }
                ~TextJob() override
                {
                }

            public:
                void Set(const uint32_t id, Core::ProxyType<Service>& service, const string& text)
                {
                    Job::Set(id, service);
                    _text = text;
                }
                void Dispatch() override
                {
                    ASSERT(HasService() == true);

                    _text = Job::Process(_text);

                    if (_text.empty() == false) {
                        // Fire and forget, We are done !!!
                        Job::Submit(_text);
                        _text.clear();
                    }

                    Job::Clear();
                }

            private:
                string _text;
            };

        public:
            Channel() = delete;
            Channel(const Channel& copy) = delete;
            Channel& operator=(const Channel&) = delete;

            Channel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Channel>* parent);
            ~Channel() override;

        public:
            inline uint32_t Id() const
            {
                return (PluginHost::Channel::Id());
            }
            static void Initialize(const string& serverPrefix)
            {
                WebRequestJob::Initialize();

                _missingCallsign->ErrorCode = Web::STATUS_BAD_REQUEST;
                _missingCallsign->Message = _T("After the /") + serverPrefix + _T("/ URL a Callsign is expected.");

                _incorrectVersion->ErrorCode = Web::STATUS_BAD_REQUEST;
                _incorrectVersion->Message = _T("Callsign was oke, but the requested version was not supported.");

                _unauthorizedRequest->ErrorCode = Web::STATUS_UNAUTHORIZED;
                _unauthorizedRequest->Message = _T("Request needs authorization, but it was not authorized");
            }
            void Revoke(PluginHost::ISecurity* baseRights)
            {
                PluginHost::Channel::Lock();

                if (_security != baseRights) {
                    if (_security != nullptr) {
                        _security->Release();
                    }
                    _security = baseRights;
                    _security->AddRef();
                }

                PluginHost::Channel::Unlock();
            }
            inline void Submit(const string& text)
            {
                PluginHost::Channel::Submit(text);
            }
            inline void Submit(const Core::ProxyType<Web::Response>& entry)
            {
                PluginHost::Channel::Submit(entry);
            }
            void Submit(const Core::ProxyType<Core::JSON::IElement>& entry) 
            {
                if (State() == Channel::ChannelState::WEB) {
                    Core::ProxyType<Web::Response> response = IFactories::Instance().Response();

                    if (response->AccessControlOrigin.IsSet() == false)
                        response->AccessControlOrigin = _T("*");

                    if (response->CacheControl.IsSet() == false)
                        response->CacheControl = _T("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0");

                    response->Body(entry);

                    PluginHost::Channel::Submit(response);
                }
                else {
                    PluginHost::Channel::Submit(entry);
                }
            }
            inline void RequestClose() {
                _requestClose = true;
            }

        private:
            bool Allowed(const string& pathParameter, const string& queryParameters)
            {
                Core::URL::KeyValue options(queryParameters);

                Core::TextFragment token = options.Value(_T("token"), false);

                if (token.IsEmpty() == false) {
                    ISecurity* security = _parent.Officer(token.Text());

                    if (security != nullptr) {
                        PluginHost::Channel::Lock();

                        if (_security != nullptr) {
                            _security->Release();
                        }
                        _security = security;
                        _security->AddRef();

                        PluginHost::Channel::Unlock();
                    }
                }

                ASSERT(_security != nullptr);

                return (_security != nullptr ? _security->Allowed(pathParameter) : false);
            }

            // Handle the HTTP Web requests.
            // [INBOUND]  Completed received requests are triggering the Received,
            // [OUTBOUND] Completed send responses are triggering the Send.
            virtual void
            LinkBody(Core::ProxyType<Request>& request)
            {
                // This is the time where we determine what body is needed for the incoming request.
                TRACE(WebFlow, (Core::proxy_cast<Web::Request>(request)));

                // Remember the path and options..
                Core::ProxyType<Service> service;
                bool serviceCall;

                uint32_t status = _parent.Services().FromLocator(request->Path, service, serviceCall);

                request->Service(status, Core::proxy_cast<PluginHost::Service>(service), serviceCall);

                ASSERT(request->State() != Request::INCOMPLETE);

                if (request->State() == Request::COMPLETE) {

                    ASSERT(service.IsValid() == true);

                    if (serviceCall == true) {
                        service->Inbound(*request);
                    } else {
                        request->Body(IFactories::Instance().JSONRPC());
                    }
                }
            }
            virtual void Received(Core::ProxyType<Request>& request)
            {
                ISecurity* security = nullptr;

                TRACE(WebFlow, (Core::proxy_cast<Web::Request>(request)));

                // See if a token has been hooked up to the request, maybe we need a
                // different security provider.
                if (request->WebToken.IsSet()) {
                    security = _parent.Officer(request->WebToken.Value().Token());

                    // Do we now want this token to be permant for this channel or only
                    // for this request ??? For now we will only use it for this request
                    // If it must be made permanent, swap the current _security with this
                    // one...
                } else if ((request->Verb == Web::Request::HTTP_GET) && (request->Query.IsSet())) {
                    Core::URL::KeyValue options(request->Query.Value());

                    string token = options.Value(_T("token"), false).Text();
                    if (token.empty() == false) {
                        security = _parent.Officer(token);
                    }
                }

                if (security == nullptr) {
                    PluginHost::Channel::Lock();
                    security = _security;
                    security->AddRef();
                    PluginHost::Channel::Unlock();
                }

                // See if we are allowed to process this request..
                if ((security == nullptr) || (security->Allowed(*request) == false)) {
                    request->Unauthorized();
                } else {
                    // If there was no body, we are still incomplete.
                    if (request->State() == Request::INCOMPLETE) {

                        Core::ProxyType<Service> service;
                        bool serviceCall;
                        uint32_t status = _parent.Services().FromLocator(request->Path, service, serviceCall);

                        request->Service(status, Core::proxy_cast<PluginHost::Service>(service), serviceCall);
                    } else if ((request->State() == Request::COMPLETE) && (request->HasBody() == true)) {
                        Core::ProxyType<Core::JSONRPC::Message> message(request->Body<Core::JSONRPC::Message>());
                        if ((message.IsValid() == true) && (security->Allowed(*message) == false)) {
                            request->Unauthorized();
                        }
                    }
                }

                if (security != nullptr) {
                    // We are done with the security related items, let go of the officer.
                    security->Release();
                }

                switch (request->State()) {
                case Request::OBLIVIOUS: {
                    Core::ProxyType<Web::Response> result(IFactories::Instance().Response());

                    if ((request->Path.empty() == true) || (request->Path == _T("/"))) {
                        result->ErrorCode = Web::STATUS_MOVED_PERMANENTLY;
                        result->Location = _parent.Configuration().Redirect() + _T("?ip=") + _parent.Configuration().Accessor().HostAddress() + _T("&port=") + Core::NumberType<uint16_t>(_parent.Configuration().Accessor().PortNumber()).Text();
                    } else {
                        result->ErrorCode = Web::STATUS_NOT_FOUND;
                        result->Message = "Not Found";
                    }

                    Submit(result);

                    break;
                }
                case Request::MISSING_CALLSIGN: {
                    // Report that we, at least, need a call sign.
                    Submit(_missingCallsign);
                    break;
                }
                case Request::INVALID_VERSION: {
                    // Report that we, at least, need a call sign.
                    Submit(_incorrectVersion);
                    break;
                }
                case Request::UNAUTHORIZED: {
                    // Report that we, at least, need a call sign.
                    Submit(_unauthorizedRequest);
                    break;
                }
                case Request::COMPLETE: {
                    Core::ProxyType<Service> service(Core::proxy_cast<Service>(request->Service()));

                    ASSERT(service.IsValid());

                    Core::ProxyType<Web::Response> response(service->Evaluate(*request));

                    if (response.IsValid() == true) {
                        // Report that the calls sign could not be found !!
                        Submit(response);
                    } else {
                        // Send the Request object out to be handled.
                        // By definition, we can issue it on a rental thread..
                        Core::ProxyType<WebRequestJob> job(_webJobs.Element(&_parent));

                        ASSERT(job.IsValid() == true);

                        if (job.IsValid() == true) {
                            Core::ProxyType<Web::Request> baseRequest(Core::proxy_cast<Web::Request>(request));
                            job->Set(Id(), service, baseRequest, _security->Token(), !request->ServiceCall());
                            _parent.Submit(Core::proxy_cast<Core::IDispatchType<void>>(job));
                        }
                    }
                    break;
                }
                default: {
                    // I think we handled every possible situation
                    ASSERT(false);
                }
                }
            }
            virtual void Send(const Core::ProxyType<Web::Response>& response)
            {
                if (_requestClose == true) {
                    PluginHost::Channel::Close(0);
                }
                TRACE(WebFlow, (response));
            }

            // Handle the JSON structs flowing over the WebSocket.
            // [INBOUND]  Completed deserialized JSON objects that are Received, will trigger the Received.
            // [OUTBOUND] Completed serialized JSON objects that are send out, will trigger the Send.
            virtual Core::ProxyType<Core::JSON::IElement> Element(const string& identifier)
            {
                Core::ProxyType<Core::JSON::IElement> result;

                if (_service.IsValid() == true) {
                    if (State() == JSONRPC) {
                        result = Core::ProxyType<Core::JSON::IElement>(IFactories::Instance().JSONRPC());
                    } else {
                        result = _service->Inbound(identifier);
                    }
                }

                return (result);
            }
            virtual void Send(const Core::ProxyType<Core::JSON::IElement>& element)
            {
                TRACE(SocketFlow, (element));
            }
            virtual void Received(Core::ProxyType<Core::JSON::IElement>& element)
            {
                bool securityClearance = ((State() & Channel::JSONRPC) == 0);

                ASSERT(_service.IsValid() == true);

                TRACE(SocketFlow, (element));

                if (securityClearance == false) {
                    Core::ProxyType<Core::JSONRPC::Message> message(Core::proxy_cast<Core::JSONRPC::Message>(element));
                    if (message.IsValid()) {
                        PluginHost::Channel::Lock();
                        securityClearance = _security->Allowed(*message);
                        PluginHost::Channel::Unlock();

                        if (securityClearance == false) {
                            // Oopsie daisy we are not allowed to handle this request.
                            // TODO: How shall we report back on this?
                            message->Error.SetError(Core::ERROR_PRIVILIGED_REQUEST);
                            message->Error.Text = _T("method invokation not allowed.");
                            Submit(Core::ProxyType<Core::JSON::IElement>(message));
                        }
                    }
                }

                if (securityClearance == true) {
                    // Send the JSON object out to be handled.
                    // By definition, we can issue it on a rental thread..
                    Core::ProxyType<JSONElementJob> job(_jsonJobs.Element(&_parent));

                    ASSERT(job.IsValid() == true);

                    if ((_service.IsValid() == true) && (job.IsValid() == true)) {
                        job->Set(Id(), _service, element, _security->Token(), ((State() & Channel::JSONRPC) != 0));
                        _parent.Submit(Core::proxy_cast<Core::IDispatch>(job));
                    }
                }
            }
            virtual void Received(const string& value)
            {
                ASSERT(_service.IsValid() == true);

                TRACE(TextFlow, (value));

                // Send the JSON object out to be handled.
                // By definition, we can issue it on a rental thread..
                Core::ProxyType<TextJob> job(_textJobs.Element(&_parent));

                ASSERT(job.IsValid() == true);

                if ((_service.IsValid() == true) && (job.IsValid() == true)) {
                    job->Set(Id(), _service, value);
                    _parent.Submit(Core::proxy_cast<Core::IDispatch>(job));
                }
            }

            // We are in an upgraded mode, we are a websocket. Time to "deserialize and serialize
            // INBOUND and OUTBOUND information.
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                uint16_t result = 0;

                if (State() == RAW) {
                    result = _service->Outbound(Id(), dataFrame, maxSendSize);
                } else {
                    result = PluginHost::Channel::Serialize(dataFrame, maxSendSize);
                }

                return (result);
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                uint16_t result = receivedSize;

                if (State() == RAW) {
                    result = _service->Inbound(Id(), dataFrame, receivedSize);
                } else {
                    result = PluginHost::Channel::Deserialize(dataFrame, receivedSize);
                }

                return (result);
            }

            // Whenever there is a state change on the link, it is reported here.
            virtual void StateChange()
            {
                TRACE(Activity, (_T("State change on [%d] to [%s]"), Id(), (IsSuspended() ? _T("SUSPENDED") : (IsUpgrading() ? _T("UPGRADING") : (IsWebSocket() ? _T("WEBSOCKET") : _T("WEBSERVER"))))));

                // If we are closing (or closed) do the clean up
                if (IsOpen() == false) {
                    if (_service.IsValid() == true) {
                        _service->Unsubscribe(*this);

                        _service.Release();
                    }

                    State(CLOSED, false);
                } else if (IsWebSocket() == true) {
                    ASSERT(_service.IsValid() == false);
                    bool serviceCall;
                    // see if we need to subscribe...
                    _parent.Services().FromLocator(Path(), _service, serviceCall);

                    if (_service.IsValid() == false) {
                        AbortUpgrade(Web::STATUS_SERVICE_UNAVAILABLE, _T("Could not find a correct service for this socket."));
                    } else if (Allowed(Path(), Query()) == false) {
                        AbortUpgrade(Web::STATUS_FORBIDDEN, _T("Security prohibites this connection."));
                    } else if (serviceCall == true) {
                        const string& serviceHeader(_parent._config.WebPrefix());

                        if (Protocol() == _T("notification")) {
                            State(TEXT, true);
                        } else if (Protocol() == _T("json")) {
                            State(JSON, false);
                        } else if (Protocol() == _T("text")) {
                            State(TEXT, false);
                        } else if (Protocol() == _T("jsonrpc")) {
                            State(JSONRPC, false);
                        } else {
                            // Channel is a raw communication channel.
                            // This channel allows for passing binary data back and forth
                            State(RAW, false);
                        }
                        if (Name().length() > (serviceHeader.length() + 1)) {
                            Properties(static_cast<uint32_t>(serviceHeader.length()) + 1);
                        }
                        // The state needs to be correct before we c
                        if (_service->Subscribe(*this) == false) {
                            State(WEB, false);
                            AbortUpgrade(Web::STATUS_FORBIDDEN, _T("Subscription rejected by the destination plugin."));
                        }
                    } else {
                        const string& JSONRPCHeader(_parent._config.JSONRPCPrefix());
                        if (Name().length() > (JSONRPCHeader.length() + 1)) {
                            Properties(static_cast<uint32_t>(JSONRPCHeader.length()) + 1);
                        }
                        State(JSONRPC, false);

                        // The state needs to be correct before we c
                        if (_service->Subscribe(*this) == false) {
                            State(WEB, false);
                            AbortUpgrade(Web::STATUS_FORBIDDEN, _T("Subscription rejected by the destination plugin."));
                        }
                    }
                }
            }

            friend class Core::SocketServerType<Channel>;

            inline void Id(const uint32_t id)
            {
                SetId(id);
            }

        private:
            Server& _parent;
            PluginHost::ISecurity* _security;
            Core::ProxyType<Service> _service;
            bool _requestClose;

            // Factories for creating jobs that can be placed on the PluginHost Worker pool.
            static Core::ProxyPoolType<WebRequestJob> _webJobs;
            static Core::ProxyPoolType<JSONElementJob> _jsonJobs;
            static Core::ProxyPoolType<TextJob> _textJobs;

            // If there is no call sign or the associated handler does not exist,
            // we can return a proper answer, without dispatching.
            static Core::ProxyType<Web::Response> _missingCallsign;

            // If there is a call sign but the version request is not avilable,
            // we can return a proper answer, without dispatching.
            static Core::ProxyType<Web::Response> _incorrectVersion;

            // If a request requires security clearance, but it is not give, for
            // whatever reason, we will report back that the request is unauthorized.
            static Core::ProxyType<Web::Response> _unauthorizedRequest;
        };
        class ChannelMap : public Core::SocketServerType<Channel> {
        private:
            typedef Core::SocketServerType<Channel> BaseClass;

            class Job : public Core::IDispatchType<void> {
            private:
                Job() = delete;
                Job(const Job& copy) = delete;
                Job& operator=(const Job& RHS) = delete;

            public:
                Job(ChannelMap* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                virtual ~Job()
                {
                }

            public:
                virtual void Dispatch() override
                {

                    return (_parent.Timed());
                }

            private:
                ChannelMap& _parent;
            };

        public:
            ChannelMap() = delete;
            ChannelMap(const ChannelMap&) = delete;
            ChannelMap& operator=(const ChannelMap&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            ChannelMap(Server& parent, const Core::NodeId& listeningNode, const uint16_t connectionCheckTimer)
                : Core::SocketServerType<Channel>(listeningNode)
                , _parent(parent)
                , _connectionCheckTimer(connectionCheckTimer * 1000)
                , _job(Core::ProxyType<Job>::Create(this))
            {
                if (connectionCheckTimer != 0) {
                    Core::Time NextTick = Core::Time::Now();

                    NextTick.Add(_connectionCheckTimer);

                    _parent.Schedule(NextTick.Ticks(), _job);
                }
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~ChannelMap()
            {

                _parent.Revoke(_job);

                // Start by closing the server thread..
                Close(100);

                // Kill all open connections, we are shutting down !!!
                BaseClass::Iterator index(BaseClass::Clients());

                while (index.Next() == true) {
                    // Oops nothing hapened for a long time, kill the connection
                    // give it 100ms to actually close, if not do it forcefully !!
                    index.Client()->Close(100);
                }

                // Cleanup the closed sockets we created..
                Cleanup();
            }

        public:
            void SecurityRevoke(ISecurity* fallback)
            {
                BaseClass::Lock();

                BaseClass::Iterator index(BaseClass::Clients());

                while (index.Next() == true) {
                    index.Client()->Revoke(fallback);
                }

                BaseClass::Unlock();
            }
            inline Server& Parent()
            {
                return (_parent);
            }
            inline uint32_t ActiveClients() const
            {
                return (Core::SocketServerType<Channel>::Count());
            }
            inline void RequestClose(const uint32_t id) {
                Core::ProxyType<Channel> client(BaseClass::Client(id));

                if (client.IsValid() == true) {
                    client->RequestClose();
                }
            }
            void GetMetaData(Core::JSON::ArrayType<MetaData::Channel>& metaData) const;

        private:
            void Timed()
            {
                TRACE(Activity, (string(_T("Cleanup job running..\n"))));

                Core::Time NextTick(Core::Time::Now());

                NextTick.Add(_connectionCheckTimer);

                // First clear all shit from last time..
                Cleanup();

                // Now suspend those that have no activity.
                BaseClass::Iterator index(BaseClass::Clients());

                while (index.Next() == true) {
                    if (index.Client()->HasActivity() == false) {
                        TRACE(Activity, (_T("Client close without activity on ID [%d]"), index.Client()->Id()));

                        // Oops nothing hapened for a long time, kill the connection
                        // Give it all the time (0) if it i not yet suspended to close. If it is
                        // suspended, force the close down if not closed in 100ms.
                        index.Client()->Close(0);
                    } else {
                        index.Client()->ResetActivity();
                    }
                }

                _parent.Schedule(NextTick.Ticks(), _job);
            }

        private:
            Server& _parent;
            const uint32_t _connectionCheckTimer;
            Core::ProxyType<Core::IDispatchType<void>> _job;
        };

    public:
        Server(Config& configuration, const bool background);
        virtual ~Server();

    public:
        inline ChannelMap& Dispatcher()
        {
            return (_connections);
        }
        inline ServiceMap& Services()
        {
            return (_services);
        }
        inline Server::WorkerPoolImplementation& WorkerPool()
        {
            return (_dispatcher);
        }
        inline void Submit(const Core::ProxyType<Core::IDispatchType<void>>& job)
        {
            _dispatcher.Submit(job);
        }
        inline void Schedule(const uint64_t time, const Core::ProxyType<Core::IDispatchType<void>>& job)
        {
            _dispatcher.Schedule(time, job);
        }
        inline void Revoke(const Core::ProxyType<Core::IDispatchType<void>> job)
        {
            _dispatcher.Revoke(job);
        }
        inline const PluginHost::Config& Configuration() const
        {
            return (_config);
        }
        void Notification(const ForwardMessage& message);
        void Open();
        void Close();

        static void PostMortem(Service& service, const IShell::reason why, RPC::IRemoteConnection* connection);

    private:
        inline Core::ProxyType<Service> Controller()
        {
            return (_controller);
        }
        ISecurity* Officer(const string& token)
        {
            return (_services.Officer(token));
        }
        inline ISecurity* Officer()
        {
            return (_config.Security());
        }

    private:
        Core::NodeId _accessor;

        // Here we start dispatching to different threads for different requests if required and if we have a service
        // that can handle the request.
        WorkerPoolImplementation _dispatcher;

        // Create the server. This is a socket listening for incoming connections. Any connection comming in, will be
        // linked to this server and will forward the received requests to this server. This server will than handl it using a thread pool.
        ChannelMap _connections;

        // Remember the interesting and properly formatted part of the configuration.
        PluginHost::Config& _config;

        // Maintain a list of all the loaded plugin servers. Here we can dispatch work to.
        ServiceMap _services;

        PluginHost::InputHandler _inputHandler;

        // Hold on to the controller that controls the PluginHost. Using this plugin, the
        // system can externally control the webbridge.
        Core::ProxyType<Service> _controller;

        // All the object required for regular communication are coming from proxypools, which
        // will be parts of this server.
        FactoriesImplementation _factoriesImplementation;
    };
}
}

#endif // __WEBPLUGINSERVER_H
