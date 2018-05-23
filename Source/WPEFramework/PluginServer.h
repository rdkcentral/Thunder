#ifndef __WEBBRIDGEPLUGINSERVER_H
#define __WEBBRIDGEPLUGINSERVER_H

#include "Module.h"
#include "SystemInfo.h"

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
    class EXTERNAL Server {
    private:
        Server() = delete;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

    public:
        static const TCHAR* ConfigFile;
        static const TCHAR* PluginOverrideFile;
        static const TCHAR* PluginConfigDirectory;
        static const TCHAR* CommunicatorConnector;

    public:
        // Configuration to get a server (PluginHost server) up and running.
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class ProcessSet : public Core::JSON::Container {
            public:
                ProcessSet()
                    : Core::JSON::Container()
                    , Priority(0)
                    , OOMAdjust(0)
                    , Policy()
                    , StackSize(0)
                {
                    Add(_T("priority"), &Priority);
                    Add(_T("policy"), &Policy);
                    Add(_T("oomadjust"), &OOMAdjust);
                    Add(_T("stacksize"), &StackSize);
                }
                ProcessSet(const ProcessSet& copy)
                    : Core::JSON::Container()
                    , Priority(copy.Priority)
                    , OOMAdjust(copy.OOMAdjust)
                    , Policy(copy.Policy)
                    , StackSize(copy.StackSize)
                {
                    Add(_T("priority"), &Priority);
                    Add(_T("policy"), &Policy);
                    Add(_T("oomadjust"), &OOMAdjust);
                    Add(_T("stacksize"), &StackSize);
                }
                ~ProcessSet()
                {
                }

                ProcessSet& operator=(const ProcessSet& RHS)
                {
                    Priority = RHS.Priority;
                    Policy = RHS.Policy;
                    OOMAdjust = RHS.OOMAdjust;
                    StackSize = RHS.StackSize;

                    return (*this);
                }

                Core::JSON::DecSInt8 Priority;
                Core::JSON::DecSInt8 OOMAdjust;
                Core::JSON::EnumType<Core::ProcessInfo::scheduler> Policy;
                Core::JSON::DecUInt32 StackSize;
            };

            class InputConfig : public Core::JSON::Container {
            public:
                InputConfig()
#ifdef __WIN32__
                    : Locator("127.0.0.1:9631")
                    , Type(PluginHost::InputHandler::VIRTUAL)
#else
					: Locator("/tmp/keyhandler")
					, Type(PluginHost::InputHandler::VIRTUAL)
#endif
                {

                    Add(_T("locator"), &Locator);
                    Add(_T("type"), &Type);
                }
                InputConfig(const InputConfig& copy)
                    : Locator(copy.Locator)
                    , Type(copy.Type)
                {
                    Add(_T("locator"), &Locator);
                    Add(_T("type"), &Type);
                }
                ~InputConfig()
                {
                }
                InputConfig& operator=(const InputConfig& RHS)
                {
                    Locator = RHS.Locator;
                    Type = RHS.Type;
                    return (*this);
                }

                Core::JSON::String Locator;
                Core::JSON::EnumType<PluginHost::InputHandler::type> Type;
            };

        public:
            Config()
                : Version()
                , Model()
                , Port(80)
                , Binding(_T("0.0.0.0"))
                , Interface()
                , Prefix(_T("Service"))
                , PersistentPath()
                , DataPath()
                , SystemPath()
#ifdef __WIN32__
                , VolatilePath(_T("c:/temp"))
#else
                , VolatilePath(_T("/tmp"))
#endif
                , ProxyStubPath()
#ifdef __WIN32__
                , Communicator(_T("127.0.0.1:7889"))
#else
                , Communicator(_T("/tmp/communicator"))
#endif
                , Redirect(_T("http://127.0.0.1/Service/Controller/UI"))
                , Signature(_T("TestSecretKey"))
                , IdleTime(0)
                , IPV6(false)
                , DefaultTraceCategories(false)
                , Process()
                , Input()
                , Configs()
            {
                // No IdleTime
                Add(_T("version"), &Version);
                Add(_T("model"), &Model);
                Add(_T("port"), &Port);
                Add(_T("binding"), &Binding);
                Add(_T("interface"), &Interface);
                Add(_T("prefix"), &Prefix);
                Add(_T("persistentpath"), &PersistentPath);
                Add(_T("datapath"), &DataPath);
                Add(_T("systempath"), &SystemPath);
                Add(_T("volatilepath"), &VolatilePath);
                Add(_T("proxystubpath"), &ProxyStubPath);
                Add(_T("communicator"), &Communicator);
                Add(_T("signature"), &Signature);
                Add(_T("idletime"), &IdleTime);
                Add(_T("ipv6"), &IPV6);
                Add(_T("tracing"), &DefaultTraceCategories);
                Add(_T("redirect"), &Redirect);
                Add(_T("process"), &Process);
                Add(_T("input"), &Input);
                Add(_T("configs"), &Configs);
                Add(_T("plugins"), &Plugins);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Version;
            Core::JSON::String Model;
            Core::JSON::DecUInt16 Port;
            Core::JSON::String Binding;
            Core::JSON::String Interface;
            Core::JSON::String Prefix;
            Core::JSON::String PersistentPath;
            Core::JSON::String DataPath;
            Core::JSON::String SystemPath;
            Core::JSON::String VolatilePath;
            Core::JSON::String ProxyStubPath;
            Core::JSON::String Communicator;
            Core::JSON::String Redirect;
            Core::JSON::String Signature;
            Core::JSON::DecUInt16 IdleTime;
            Core::JSON::Boolean IPV6;
            Core::JSON::String DefaultTraceCategories;
            ProcessSet Process;
            InputConfig Input;
            Core::JSON::String Configs;
            Core::JSON::ArrayType<Plugin::Config> Plugins;
        };

    private:
        class ServiceMap;
        friend class Plugin::Controller;

        // Trace class for internal information of the PluginHost
        class EXTERNAL Activity {
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
        class EXTERNAL WebFlow {
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
        class EXTERNAL SocketFlow {
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
        class EXTERNAL TextFlow {
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
            TextFlow(const string& text) : _text(Core::ToString(text))
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
 

        class EXTERNAL Service : public PluginHost::Service {
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
                Condition(const Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> >& input, const bool defaultValue)
                    : _events(0)
                    , _value(0) {
                    Core::JSON::ArrayType<Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> >::ConstIterator index (input.Elements());

                    while (index.Next() == true) {
                        ASSERT(index.Current() <= 0xFF);
                        uint8_t bitNr = static_cast<uint8_t>(index.Current());

                        if (bitNr > ISubSystem::END_LIST) {
                            bitNr -= ISubSystem::END_LIST;
                        }
                        else {
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
                ~Condition() {
                }

            public:
                inline bool IsMet() const
                {
                    return ((_events == 0) || ((_value != static_cast<uint32_t>(~0)) && ((_events & (1 << PluginHost::ISubSystem::END_LIST)) != 0)));
                }
                inline bool Evaluate(const uint32_t subsystems) {
                    bool result = (subsystems & _events) == _value;

                    if (result ^ IsMet()) {
                        // We changed from setup, signal it...
                        if (result == true) {
                            _events |= (1 << PluginHost::ISubSystem::END_LIST);
                        }
                        else {
                            _events &= ~(1 << PluginHost::ISubSystem::END_LIST);
                        }
                        result = true;
                    }
                    else {
                        result = false;
                    }

                    return (result);
                }
		inline uint32_t Delta (const uint32_t currentSet) {
                    return ((currentSet & _events) ^ _value);
                }

            private:
                uint32_t _events;
                uint32_t _value;
            };

        public:
            Service(const PluginHost::Config* server, const Plugin::Config* plugin, ServiceMap* administrator)
                : PluginHost::Service(*server, *plugin)
                , _handler(nullptr)
                , _extended(nullptr)
                , _webRequest(nullptr)
                , _webSocket(nullptr)
                , _textSocket(nullptr)
                , _rawSocket(nullptr)
                , _webSecurity(nullptr)
                , _precondition(plugin->Precondition, true)
                , _termination(plugin->Termination, false)
                , _activity(0)
                , _administrator(*administrator)
            {
                ASSERT(server != nullptr);
                ASSERT(plugin != nullptr);
                ASSERT(administrator != nullptr);
            }
            ~Service()
            {
                Deactivate(PluginHost::IShell::SHUTDOWN);

                ASSERT(_handler == nullptr);
                ASSERT(_extended == nullptr);
                ASSERT(_webRequest == nullptr);
                ASSERT(_webSocket == nullptr);
                ASSERT(_textSocket == nullptr);
                ASSERT(_rawSocket == nullptr);
                ASSERT(_webSecurity == nullptr);
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
                bool result = PluginHost::Service::Subscribe(channel);

                if ((result == true) && (_extended != nullptr)) {
                    _extended->Attach(channel);
                }

                return (result);
            }
            inline void Unsubscribe(Channel& channel)
            {
                PluginHost::Service::Unsubscribe(channel);

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
                }
                else if (IsWebServerRequest(request.Path) == true) {
                    result = Factories::Instance().Response();
                    FileToServe(request.Path, *result);
                }
                else if (request.Verb == Web::Request::HTTP_OPTIONS) {
                    ASSERT(_webSecurity != nullptr);

                    // Create a security response..
                    result = _webSecurity->Options(request);
                }
                else if (WebRequestSupported() == false) {
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

#ifdef RUNTIME_STATISTICS
                    IncrementProcessedRequests();
#endif
                    Core::InterlockedIncrement(_activity);
                    result = service->Process(request);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                }
                else {
                    Unlock();
                }

                return (result);
            }
            inline Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::JSON::IElement& element)
            {
                Core::ProxyType<Core::JSON::IElement> result;

                Lock();

                if ((_webSocket != nullptr) && (IsActive() == true)) {
                    IWebSocket* service(_webSocket);
                    service->AddRef();
                    Unlock();

#ifdef RUNTIME_STATISTICS
                    IncrementProcessedObjects();
#endif
                    Core::InterlockedIncrement(_activity);
                    result = service->Inbound(ID, element);
                    Core::InterlockedDecrement(_activity);

                    service->Release();
                }
                else {
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
                }
                else {
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
            inline void Evaluate() {
                Lock();

		uint32_t subsystems = _administrator.SubSystemInfo();

                PluginHost::IShell::state current (State());

                // Active or not, update the condition state !!!!
                if ((_precondition.Evaluate(subsystems) == true) && (current == PluginHost::IShell::PRECONDITION)) {
                    if (_precondition.IsMet() == true) {

                        Unlock();

                        Activate(_reason);

                        Lock();
                    }
                }

                if ((_termination.Evaluate(subsystems) == true) && (current == PluginHost::IShell::ACTIVATED))  {
                    if (_termination.IsMet() == true) {

                        Unlock();

                        Deactivate(IShell::CONDITIONS);

                        Lock();
                    }
                }

                Unlock();
            }
            virtual ISubSystem* SubSystems() override;
            virtual void Notify(const string& message) override;
            virtual void* QueryInterface(const uint32_t id) override;
            virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override;
            virtual void Register(IPlugin::INotification* sink) override;
            virtual void Unregister(IPlugin::INotification* sink) override;

            // Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
            virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version, uint32_t& pid, const string& locator) override;
            virtual void Register(RPC::IRemoteProcess::INotification* sink) override;
            virtual void Unregister(RPC::IRemoteProcess::INotification* sink) override;
            virtual RPC::IRemoteProcess* RemoteProcess(const uint32_t pid) override;

            // Methods to Activate and Deactivate the aggregated Plugin to this shell.
            // These are Blocking calls!!!!!
            virtual uint32_t Activate(const reason) override;
            virtual uint32_t Deactivate(const reason) override;
            virtual reason Reason() const
            {
                return (_reason);
            }

        private:
            void AquireInterfaces()
            {
                ASSERT(State() == DEACTIVATED);

                PluginHost::IPlugin* newIF = nullptr;
                const string locator(PluginHost::Service::Configuration().Locator.Value());
                const string classNameString(PluginHost::Service::Configuration().ClassName.Value());
                const TCHAR* className(classNameString.c_str());
                uint32_t version(PluginHost::Service::Configuration().Version.IsSet() ? PluginHost::Service::Configuration().Version.Value() : static_cast<uint32_t>(~0));
                Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
                Core::Library myLib;

                if (locator.empty() == true) {
                    newIF = admin.Instantiate<PluginHost::IPlugin>(myLib, className, version);
                }
                else {
                    myLib = Core::Library((Information().PersistentPath() + locator).c_str());

                    if ((!myLib.IsLoaded()) || ((newIF = admin.Instantiate<PluginHost::IPlugin>(myLib, className, version)) == nullptr)) {
                        myLib = Core::Library((Information().SystemPath() + locator).c_str());

                        if ((!myLib.IsLoaded()) || ((newIF = admin.Instantiate<PluginHost::IPlugin>(myLib, className, version)) == nullptr)) {
                            myLib = Core::Library((Information().DataPath() + locator).c_str());

                            if ((!myLib.IsLoaded()) || ((newIF = admin.Instantiate<PluginHost::IPlugin>(myLib, className, version)) == nullptr)) {
                                myLib = Core::Library((Information().AppPath() + "Plugins/" + locator).c_str());

                                if ((!myLib.IsLoaded()) || ((newIF = admin.Instantiate<PluginHost::IPlugin>(myLib, className, version)) == nullptr)) {
                                    ErrorMessage(_T("Location [") + locator + _T("] does not contain a loadable plugin."));
                                }
                            }
                        }
                    }
                }

                _moduleName.clear();
                _versionHash.clear();

                if (myLib.IsLoaded()) {
                    Core::System::ModuleNameImpl moduleName = reinterpret_cast<Core::System::ModuleNameImpl>(myLib.LoadFunction(_T("ModuleName")));
                    Core::System::ModuleBuildRefImpl moduleBuildRef = reinterpret_cast<Core::System::ModuleBuildRefImpl>(myLib.LoadFunction(_T("ModuleBuildRef")));

                    if (moduleName != nullptr) {
                        _moduleName = moduleName();
                    }
                    if (moduleBuildRef != nullptr) {
                        _versionHash = moduleBuildRef();
                    }
                }

                if (newIF == nullptr) {
                    if (HasError() == false) {
                        ErrorMessage(_T("plugin [") + string(className) + _T("] with callsign [") + PluginHost::Service::Configuration().Callsign.Value() + _T("] could not be instantiated."));
                    }
                }
                else {
                    _extended = newIF->QueryInterface<PluginHost::IPluginExtended>();
                    _webRequest = newIF->QueryInterface<PluginHost::IWeb>();
                    _webSocket = newIF->QueryInterface<PluginHost::IWebSocket>();
                    _textSocket = newIF->QueryInterface<PluginHost::ITextSocket>();
                    _rawSocket = newIF->QueryInterface<PluginHost::IChannel>();
                    _webSecurity = newIF->QueryInterface<PluginHost::ISecurity>();
                    if (_webSecurity == nullptr) {
                        _webSecurity = Information().Security();
                        _webSecurity->AddRef();
                    }

                    Lock();
                    _handler = newIF;
                    Unlock();
                }
            }
            void ReleaseInterfaces()
            {
                ASSERT(State() != ACTIVATED);
                ASSERT(_handler != nullptr);

                PluginHost::IPlugin* currentIF = _handler;

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

                Lock();
                _handler = nullptr;
                Unlock();

                currentIF->Release();

                // Could be that we can now drop the dynamic library...
                Core::ServiceAdministrator::Instance().FlushLibraries();
            }

        private:
            // The handlers that implement the actual logic behind the service
            IPlugin* _handler;
            IPluginExtended* _extended;
            IWeb* _webRequest;
            IWebSocket* _webSocket;
            ITextSocket* _textSocket;
            IChannel* _rawSocket;
            ISecurity* _webSecurity;
            reason _reason;
            string _moduleName;
            string _versionHash;
            Condition _precondition;
            Condition _termination;
            uint32_t _activity;

            ServiceMap& _administrator;
            static Core::ProxyType<Web::Response> _unavailableHandler;
            static Core::ProxyType<Web::Response> _missingHandler;
        };
        class EXTERNAL ServiceMap {
        public:
            typedef Core::IteratorMapType<std::map<const string, Core::ProxyType<Service> >, Core::ProxyType<Service>, const string&> Iterator;

        private:
            ServiceMap() = delete;
            ServiceMap(const ServiceMap&) = delete;
            ServiceMap& operator=(const ServiceMap&) = delete;

			class CommunicatorServer : public RPC::Communicator {
			private:
				CommunicatorServer() = delete;
				CommunicatorServer(const CommunicatorServer&) = delete;
				CommunicatorServer& operator= (const CommunicatorServer&) = delete;

			public:
				CommunicatorServer(const Core::NodeId& node, const string& persistentPath, const string& systemPath, const string& dataPath, const string& appPath, const string& proxyStubPath, const uint32_t stackSize)
					: RPC::Communicator(node, Core::ProxyType<RPC::InvokeServerType<64, 3> >::Create(stackSize))
#ifdef __WIN32__
					, _options(Core::Directory::Normalize(systemPath) + EXPAND_AND_QUOTE(ARTIFACT_COMPROCESS) )
#else
					, _options( EXPAND_AND_QUOTE(ARTIFACT_COMPROCESS) )
#endif
				{
					if (RPC::Communicator::Open(RPC::CommunicationTimeOut) != Core::ERROR_NONE) {
						TRACE_L1("We can not open the RPC server. No out-of-process communication available. %d", __LINE__);
					}
					else {
						// We need to pass the communication channel NodeId via an environment variable, for process,
						// not being started by the rpcprocess...
						Core::SystemInfo::SetEnvironment(string(CommunicatorConnector), Communicator::Connector());

						// Prepare the startup Options.
						if (persistentPath.empty() == false) {
							_options[_T("p")] = Core::Directory::Normalize(persistentPath);
						}
						if (systemPath.empty() == false) {
							_options[_T("s")] = Core::Directory::Normalize(systemPath);
						}
						if (dataPath.empty() == false) {
							_options[_T("d")] = Core::Directory::Normalize(dataPath);
						}
						if (appPath.empty() == false) {
							_options[_T("a")] = Core::Directory::Normalize(appPath);
						}
						if (proxyStubPath.empty() == false) {
							string searchPath = Core::Directory::Normalize(proxyStubPath);
							_options[_T("m")] = searchPath;

							RPC::Communicator::LoadProxyStubs(searchPath);
						}
					}
				}
				virtual ~CommunicatorServer() {
				}

			public:
				Communicator::RemoteProcess* Create(const string& className, const uint32_t interfaceId, const uint32_t version, const string& locator, uint32_t& pid, const string& dataExtension, const string& persistentExtension)
				{
					ASSERT(className.empty() == false);
					ASSERT(locator.empty() == false);

					Core::Process::Options options(_options);

					if (version != static_cast<uint32_t>(~0)) {
						options[_T("v")] = Core::NumberType<uint32_t>(version).Text();
					}

					options[_T("i")] = Core::NumberType<uint32_t>(interfaceId).Text();
					options[_T("r")] = RPC::Communicator::Connector();
					options[_T("c")] = className;
					options[_T("l")] = locator;

					if (dataExtension.empty() == false) {
						options[_T("d")] += dataExtension + '/';
					}
					if (persistentExtension.empty() == false) {
						options[_T("p")] += persistentExtension + '/';
					}

					return (RPC::Communicator::Create(pid, options));
				}

			private:
				Core::Process::Options _options;
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
                    Plugin ()
                        : Core::JSON::Container()
                        , AutoStart()
                        , Configuration(_T("{}"), false)
                    {
                        Add(_T("autostart"), &AutoStart);
                        Add(_T("configuration"), &Configuration);
                    }
                    Plugin (const string& config, const bool autoStart)
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
                        , Configuration(copy.AutoStart)
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
                        std::pair<Iterator, bool> index (_callsigns.insert(std::pair<string, Plugin>(name, Plugin(_T("{}"), false))));

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
                        FromFile(storage);

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
                    }
                    else {
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
                            }
                            else {
                                current->second.Configuration = config;
                            }
                            current->second.AutoStart = (index)->AutoStart();
                        }

                        // Persist the currently set information
                        ToFile(storage);

                        storage.Close();
                    }
                    else {
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
      
            private:
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
                    void Schedule() {
                        if (_schedule == false) {
                            _schedule = true;
                            PluginHost::WorkerPool::Instance().Submit(Core::ProxyType< Core::IDispatchType<void> >(*this));
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
                SubSystems(ServiceMap* parent) 
                    : SystemInfo(this) 
                    , _parent(*parent)
                    , _decoupling(Core::ProxyType<Job>::Create(this)) {
                }
                virtual ~SubSystems() {
		    PluginHost::WorkerPool::Instance().Revoke(_decoupling);
                }

            private:
                virtual void Dispatch() override
                {
                    _decoupling->Schedule();
                }
                inline void Evaluate() {
                    _parent.Evaluate();
                }

            private:
                ServiceMap& _parent;
                Core::ProxyType<Job> _decoupling;
        };

        public:
            ServiceMap(Server& server, PluginHost::Config& config, const uint32_t stackSize)
                : _webbridgeConfig(config)
                , _adminLock()
                , _services()
                , _notifiers()
                , _processAdministrator(config.Communicator(), config.PersistentPath(), config.SystemPath(), config.DataPath(), config.AppPath(), config.ProxyStubPath(), stackSize)
                , _server(server)
                , _subSystems(this)
            {
            }
            ~ServiceMap()
            {
                // Make sure all services are deactivated before we are killed (call Destroy on this object);
                ASSERT(_services.size() == 0);
            }

        public:
            inline uint32_t SubSystemInfo() const {
                return (_subSystems.Value());
            }
            inline ISubSystem* SubSystemsInterface() {
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

                _notifiers.push_back(sink);

                // Tell this "new" sink all our active/inactive plugins..
                std::map<const string, Core::ProxyType<Service> >::iterator index(_services.begin());

                // Notifty all plugins that we have sofar..
                while (index != _services.end()) {
                    ASSERT(index->second.IsValid());

                    Core::ProxyType<Service> service(index->second);

                    ASSERT(service.IsValid());

                    if (service.IsValid() == true) {
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
                    _notifiers.erase(index);
                }

                _notificationLock.Unlock();
            }
            inline void* QueryInterfaceByCallsign(const uint32_t id, const string& name)
            {
                void* result = nullptr;

                const string& callsign(name.empty() == true ? _server.ControllerName() : name);

                _adminLock.Lock();

                std::map<const string, Core::ProxyType<Service> >::iterator index(_services.find(callsign));

                if (index == _services.end()) {
                    _adminLock.Unlock();
                }
                else {

                    Core::ProxyType<Service> service(index->second);

                    _adminLock.Unlock();

                    ASSERT(service.IsValid());

                    if (service.IsValid() == true) {

                        result = service->QueryInterface(id);
                    }
                }
                return (result);
            }
            inline void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version, uint32_t& pid, const string& locator, const string& dataExtension, const string& persistentExtension)
            {
                void* result = nullptr;

                RPC::Communicator::RemoteProcess* process(_processAdministrator.Create(className, interfaceId, version, locator, pid, dataExtension, persistentExtension));

                if (process != nullptr) {
                    if (process->WaitState(RPC::IRemoteProcess::ACTIVE | RPC::IRemoteProcess::DEACTIVATED, waitTime) != Core::ERROR_TIMEDOUT) {
                        if (process->State() == RPC::IRemoteProcess::ACTIVE) {
                            result = process->QueryInterface(interfaceId);
                        }
                    }
                    if (result == nullptr) {
                        TRACE_L1("RPC out-of-process server activated, but failed. Terminating process. %d", __LINE__);
                        pid = 0;
                        process->Terminate();
                    }
                    process->Release();
                }
                return (result);
            }
            inline void Register(RPC::IRemoteProcess::INotification* sink)
            {
                _processAdministrator.Register(sink);
            }
            inline void Unregister(RPC::IRemoteProcess::INotification* sink)
            {
                _processAdministrator.Unregister(sink);
            }
            inline RPC::IRemoteProcess* RemoteProcess(const uint32_t pid)
            {
                return (_processAdministrator.Process(pid));
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
                Core::ProxyType<Service> newService(Core::ProxyType<Service>::Create(&_webbridgeConfig, &configuration, this));

                if (newService.IsValid() == true) {
                    _adminLock.Lock();

                    // Fire up the interface. Let it handle the messages.
                    _services.insert(std::pair<const string, Core::ProxyType<Service> >(configuration.Callsign.Value(), newService));

                    _adminLock.Unlock();
                }

                return (newService);
            }
            inline void Destroy(const string& callSign)
            {
                _adminLock.Lock();

                // First stop all services running ...
                std::map<const string, Core::ProxyType<Service> >::iterator index(_services.find(callSign));

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
            inline void Processes(std::list<uint32_t>& listOfPids) const
            {
                return (_processAdministrator.Processes(listOfPids));
            }
            inline void Notification(const string& message)
            {
                _server.Notification(message);
            }
            inline Core::ProxyType<Service> FromLocator(const string& callSign)
            {
                Core::ProxyType<Service> result;

                _adminLock.Lock();

                std::map<const string, Core::ProxyType<Service> >::const_iterator index(_services.find(callSign));

                if (index != _services.end()) {
                    result = index->second;
                }

                _adminLock.Unlock();

                return (result);
            }
            void GetMetaData(Core::JSON::ArrayType<MetaData::Service>& metaData) const {
                _adminLock.Lock();

                std::list< Core::ProxyType<Service> > duplicates;
                std::map<const string, Core::ProxyType<Service> >::const_iterator index(_services.begin());

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

            Core::ProxyType<Service> FromLocator(const string& identifier, bool& correctHeader);

            void Destroy();

        private:
           void RecursiveGetMetaData(Core::JSON::ArrayType<MetaData::Service>&, std::map<const string, Core::ProxyType<Service> >::const_iterator&) const;

           void RecursiveNotification(std::map<const string, Core::ProxyType<Service> >::iterator& index) {
               if (index != _services.end()) {
                   Core::ProxyType<Service> element (index->second);
                   index++;
                   RecursiveNotification(index);
                   element->Evaluate();
               }
               else {
                   _adminLock.Unlock();
               }
           }
           void Evaluate() {

               _adminLock.Lock();

               // First stop all services running ...
               std::map<const string, Core::ProxyType<Service> >::iterator index(_services.begin());

               RecursiveNotification(index);
           }

        private:
            // If there are no security arangements for the specific plugin, the overall security arangement is used.
            // Store the overall security arrangement in the server.
            PluginHost::Config& _webbridgeConfig;

            mutable Core::CriticalSection _adminLock;
            Core::CriticalSection _notificationLock;
            std::map<const string, Core::ProxyType<Service> > _services;
            std::list<IPlugin::INotification*> _notifiers;
            CommunicatorServer _processAdministrator;
            Server& _server;
            Core::Sink<SubSystems> _subSystems;
        };

        // Connection handler is the listening socket and keeps track of all open
        // Links. A Channel is identified by an ID, this way, whenever a link dies
        // (is closed) during the service process, the ChannelMap will
        // not find it and just "flush" the presented work.
        class EXTERNAL Channel : public PluginHost::Channel {
            Channel() = delete;
            Channel(const Channel& copy) = delete;
            Channel& operator=(const Channel&) = delete;

        private:
            class EXTERNAL WebRequestJob : public Core::IDispatchType<void> {
            private:
                WebRequestJob() = delete;
                WebRequestJob(const WebRequestJob&) = delete;
                WebRequestJob& operator=(const WebRequestJob&) = delete;

            public:
                WebRequestJob(Server* server)
                    : _server(server)
                    , _service()
                    , _request()
                {
                }
                virtual ~WebRequestJob()
                {

                    ASSERT((_request.IsValid() == false) && (_service.IsValid() == false));

                    if (_request.IsValid()) {
                        _request.Release();
                    }
                    if (_service.IsValid()) {
                        _service.Release();
                    }
                }

            public:
                static void Initialize()
                {
                    _missingResponse->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                    _missingResponse->Message = _T("There is no response from the requested service.");
                }
                void Set(const uint32_t id, Core::ProxyType<Service>& service, Core::ProxyType<Web::Request>& request)
                {
                    ASSERT(_request.IsValid() == false);
                    ASSERT(_service.IsValid() == false);

                    _service = service;
                    _request = request;
                    _ID = id;
                }
                virtual void Dispatch()
                {
                    ASSERT(_request.IsValid());

                    if (_request.IsValid()) {
                        Core::ProxyType<Web::Response> response;

                        ASSERT(_service.IsValid() == true);

                        if (_service.IsValid() == true) {
                            response = _service->Process(*_request);
                            _service.Release();
                        }

                        if (response.IsValid() == true) {
                            // Seems we can handle..
                            if (response->AccessControlOrigin.IsSet() == false)
                                response->AccessControlOrigin = _T("*");

                            if (response->CacheControl.IsSet() == false)
                                response->CacheControl = _T("no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0");

                            _server->Dispatcher().Submit(_ID, response);
                        }
                        else {
                            // Fire and forget, We are done !!!
                            _server->Dispatcher().Submit(_ID, _missingResponse);
                        }

                        if (_request->Connection.Value() == Web::Request::CONNECTION_CLOSE) {
                            TRACE(Activity, (_T("HTTP Request with direct close on [%d]"), _ID));
                            _server->Dispatcher().Suspend(_ID);
                        }

                        // We are done, clear all info
                        _request.Release();
                    }
                }

            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
                Core::ProxyType<Web::Request> _request;

                static Core::ProxyType<Web::Response> _missingResponse;
            };

            class EXTERNAL JSONElementJob : public Core::IDispatchType<void> {
            private:
                JSONElementJob() = delete;
                JSONElementJob(const JSONElementJob&) = delete;
                JSONElementJob& operator=(const JSONElementJob&) = delete;

            public:
                JSONElementJob(Server* server)
                    : _server(server)
                    , _service()
                    , _element()
                {
                }
                virtual ~JSONElementJob()
                {

                    ASSERT((_element.IsValid() == false) && (_service.IsValid() == false));

                    if (_element.IsValid()) {
                        _element.Release();
                    }
                    if (_service.IsValid()) {
                        _service.Release();
                    }
                }

            public:
                void Set(const uint32_t id, Core::ProxyType<Service>& service, Core::ProxyType<Core::JSON::IElement>& element)
                {
                    ASSERT(_element.IsValid() == false);
                    ASSERT(_service.IsValid() == false);

                    _service = service;
                    _element = element;
                    _ID = id;
                }
                virtual void Dispatch()
                {
                    ASSERT(_element.IsValid());

                    if (_element.IsValid()) {

                        ASSERT(_service.IsValid() == true);

                        if (_service.IsValid() == true) {

                            _element = _service->Inbound(_ID, *_element);
                            _service.Release();
                        }

                        if (_element.IsValid()) {
                            // Fire and forget, We are done !!!
                            _server->Dispatcher().Submit(_ID, _element);
                            _element.Release();
                        }
                    }
                }

            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
                Core::ProxyType<Core::JSON::IElement> _element;
            };

            class EXTERNAL TextJob : public Core::IDispatchType<void> {
            private:
                TextJob() = delete;
                TextJob(const TextJob&) = delete;
                TextJob& operator=(const TextJob&) = delete;

            public:
                TextJob(Server* server)
                    : _server(server)
                    , _service()
                    , _text()
                {
                }
                virtual ~TextJob()
                {
                    ASSERT(_service.IsValid() == false);

                    if (_service.IsValid()) {
                        _service.Release();
                    }
                }

            public:
                void Set(const uint32_t id, Core::ProxyType<Service>& service, const string& text)
                {
                    ASSERT(_service.IsValid() == false);

                    _service = service;
                    _text = text;
                    _ID = id;
                }
                virtual void Dispatch()
                {
                    ASSERT(_service.IsValid() == true);

                    if (_service.IsValid() == true) {

                        _text = _service->Inbound(_ID, _text);
                        _service.Release();
                    }

                    if (_text.empty() == false) {
                        // Fire and forget, We are done !!!
                        _server->Dispatcher().Submit(_ID, _text);
                        _text.clear();
                    }
                }

            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
                string _text;
            };


        public:
            Channel(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Channel>* parent);
            virtual ~Channel();

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
            }

        private:
            // Handle the HTTP Web requests.
            // [INBOUND]  Completed received requests are triggering the Received,
            // [OUTBOUND] Completed send responses are triggering the Send.
            virtual void LinkBody(Core::ProxyType<Request>& request)
            {
                // This is the time where we determine what body is needed for the incoming request.
                TRACE(WebFlow, (Core::proxy_cast<Web::Request>(request)));

                // Remember the path and options..
                bool correctHeader;

                Core::ProxyType<Service> service(_parent.Services().FromLocator(request->Path, correctHeader));

                request->Service(correctHeader, Core::proxy_cast<PluginHost::Service>(service));

                ASSERT(request->State() != Request::INCOMPLETE);

                if (request->State() == Request::COMPLETE) {
                    ASSERT(service.IsValid() == true);

                    service->Inbound(*request);
                }
            }
            virtual void Received(Core::ProxyType<Request>& request)
            {
                TRACE(WebFlow, (Core::proxy_cast<Web::Request>(request)));

                // If there was no body, we are still incomplete.
                if (request->State() == Request::INCOMPLETE) {
                    bool correctHeader;

                    Core::ProxyType<Service> service(_parent.Services().FromLocator(request->Path, correctHeader));

                    request->Service(correctHeader, Core::proxy_cast<PluginHost::Service>(service));
                }

                switch (request->State()) {
                case Request::OBLIVIOUS: {
                    Core::ProxyType<Web::Response> result(Factories::Instance().Response());

                    if ((request->Path.empty() == true) || (request->Path == _T("/"))) {
                        result->ErrorCode = Web::STATUS_MOVED_PERMANENTLY;
                        result->Location = _parent.Configuration().Redirect() + _T("?ip=") + _parent.Configuration().Accessor().HostAddress() + _T("&port=") + Core::NumberType<uint16_t>(_parent.Configuration().Accessor().PortNumber()).Text();
                    }
                    else {
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
                case Request::COMPLETE: {
                    Core::ProxyType<Service> service(Core::proxy_cast<Service>(request->Service()));

                    ASSERT(service.IsValid());

                    Core::ProxyType<Web::Response> response(service->Evaluate(*request));

                    if (response.IsValid() == true) {
                        // Report that the calls sign could not be found !!
                        Submit(response);
                    }
                    else {
                        // Send the Request object out to be handled.
                        // By definition, we can issue it on a rental thread..
                        Core::ProxyType<WebRequestJob> job(_webJobs.Element(&_parent));

                        ASSERT(job.IsValid() == true);

                        if (job.IsValid() == true) {
                            Core::ProxyType<Web::Request> baseRequest(Core::proxy_cast<Web::Request>(request));
                            job->Set(Id(), service, baseRequest);
                            _parent.Submit(Core::proxy_cast<Core::IDispatchType<void> >(job));
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
                TRACE(WebFlow, (response));
            }

            // Handle the JSON structs flowing over the WebSocket.
            // [INBOUND]  Completed deserialized JSON objects that are Received, will trigger the Received.
            // [OUTBOUND] Completed serialized JSON objects that are send out, will trigger the Send.
            virtual Core::ProxyType<Core::JSON::IElement> Element(const string& identifier)
            {
                Core::ProxyType<Core::JSON::IElement> result;

                if (_service.IsValid() == true) {
                    result = _service->Inbound(identifier);
                }

                return (result);
            }
            virtual void Send(const Core::ProxyType<Core::JSON::IElement>& element)
            {
                TRACE(SocketFlow, (element));
            }
            virtual void Received(Core::ProxyType<Core::JSON::IElement>& element)
            {
                ASSERT(_service.IsValid() == true);

                TRACE(SocketFlow, (element));

                // Send the JSON object out to be handled.
                // By definition, we can issue it on a rental thread..
                Core::ProxyType<JSONElementJob> job(_jsonJobs.Element(&_parent));

                ASSERT(job.IsValid() == true);

                if ((_service.IsValid() == true) && (job.IsValid() == true)) {
                    job->Set(Id(), _service, element);
                    _parent.Submit(Core::proxy_cast<Core::IDispatch>(job));
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
                }
                else {
                    result = PluginHost::Channel::Serialize(dataFrame, maxSendSize);
                }

                return (result);
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                uint16_t result = receivedSize;

                if (State() == RAW) {
                    result = _service->Inbound(Id(), dataFrame, receivedSize);
                }
                else {
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
                }
                else if (IsWebSocket() == true) {
                    ASSERT(_service.IsValid() == false);
                    bool correctHeader;

                    // see if we need to subscribe...
                    _service = _parent.Services().FromLocator(Path(), correctHeader);
                    if ((_service.IsValid() == true) && (Name().length() > _service->WebPrefix().length())) {
                        Properties(_service->WebPrefix().length() + 1);
                    }

                    if (_service.IsValid() == false) {
                        AbortUpgrade(Web::STATUS_SERVICE_UNAVAILABLE, _T("Could not find a correct service for this socket."));
                    }
                    else {
                        if (Protocol() == _T("notification")) {
                            State(TEXT, true);
                        }
                        else if (Protocol() == _T("json")) {
                            State(JSON, false);
                        }
                        else if (Protocol() == _T("text")) {
                            State(TEXT, false);
                        }
                        else {
                            // Channel is a raw communication channel.
                            // This channel allows for passing binary data back and forth
                            State(RAW, false);
                        }

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
            Core::ProxyType<Service> _service;
            Server& _parent;

            // Factories for creating jobs that can be placed on the PluginHost Worker pool.
            static Core::ProxyPoolType<WebRequestJob> _webJobs;
            static Core::ProxyPoolType<JSONElementJob> _jsonJobs;
            static Core::ProxyPoolType<TextJob> _textJobs;

            // If there is no call sign or the associated handler does not exist,
            // we can return a proper answer, without dispatching.
            static Core::ProxyType<Web::Response> _missingCallsign;
        };
        class EXTERNAL ChannelMap : public Core::SocketServerType<Channel> {
        private:
            ChannelMap() = delete;
            ChannelMap(const ChannelMap&) = delete;
            ChannelMap& operator=(const ChannelMap&) = delete;

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
#ifdef __WIN32__
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
#ifdef __WIN32__
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
            inline Server& Parent()
            {
                return (_parent);
            }
            inline uint32_t ActiveClients() const
            {
                return (Core::SocketServerType<Channel>::Count());
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
                    }
                    else {
                        index.Client()->ResetActivity();
                    }
                }

                _parent.Schedule(NextTick.Ticks(), _job);
            }

        private:
            Server& _parent;
            const uint32_t _connectionCheckTimer;
            Core::ProxyType<Core::IDispatchType<void> > _job;
        };

    public:
        Server(Config& configuration, ISecurity* securityHandler, const bool background);
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
        inline void Submit(const Core::ProxyType<Core::IDispatchType<void> >& job)
        {
            _dispatcher.Submit(job);
        }
        inline void Schedule(const uint64_t time, const Core::ProxyType<Core::IDispatchType<void> >& job)
        {
            _dispatcher.Schedule(time, job);
        }
        inline void Revoke(const Core::ProxyType<Core::IDispatchType<void> > job)
        {
            _dispatcher.Revoke(job);
        }
        inline const PluginHost::Config& Configuration() const
        {
            return (_config);
        }
        inline void Notification(const string& message)
        {
            _controller->Notification(message);
        }
        inline const string& ControllerName() const
        {
            return (_controllerName);
        }
        void Notify(const string& message) {
            _controller->Notification(message);
        }


    private:
        Core::NodeId _accessor;

        // Here we start dispatching to different threads for different requests if required and if we have a service
        // that can handle the request.
        WorkerPool& _dispatcher;

        // Create the server. This is a socket listening for incoming connections. Any connection comming in, will be
        // linked to this server and will forward the received requests to this server. This server will than handl it using a thread pool.
        ChannelMap _connections;

        // Remember the interesting and properly formatted part of the configuration.
        PluginHost::Config _config;

        // Maintain a list of all the loaded plugin servers. Here we can dispatch work to.
        ServiceMap _services;

        PluginHost::InputHandler _inputHandler;

        // Hold on to the controller that controls the PluginHost. Using this plugin, the
        // system can externally control the webbridge.
        Core::ProxyType<Service> _controller;
        string _controllerName;
    };
    }
}

#endif // __WEBPLUGINSERVER_H
