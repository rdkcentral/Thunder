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

#ifndef __WEBBRIDGEPLUGINSERVER_H
#define __WEBBRIDGEPLUGINSERVER_H

#include "Module.h"
#include "SystemInfo.h"
#include "Config.h"
#include "IRemoteInstantiation.h"
#include "WarningReportingCategories.h"

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
        typedef const IServiceMetadata* (*ModuleServiceMetadataImpl)();
        }
    }
}

namespace Plugin {
    class Controller;
}

namespace PluginHost {

    EXTERNAL string ChannelIdentifier (const Core::SocketPort& input);

    class Server {
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
        class WorkerPoolImplementation : public Core::WorkerPool, public Core::ThreadPool::ICallback {
        private:
            class Dispatcher : public Core::ThreadPool::IDispatcher {
            public:
                Dispatcher(const Dispatcher&) = delete;
                Dispatcher& operator=(const Dispatcher&) = delete;

                Dispatcher() = default;
                ~Dispatcher() override = default;

            private:
                void Initialize() override {
                }
                void Deinitialize() override {
                }
                void Dispatch(Core::IDispatch* job) override;
            };

        public:
            WorkerPoolImplementation() = delete;
            WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
            WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

            WorkerPoolImplementation(const uint32_t stackSize)
                : Core::WorkerPool(THREADPOOL_COUNT, stackSize, 16, &_dispatch, this)
                , _dispatch()
            {
                Run();
            }
            ~WorkerPoolImplementation() override = default;

        public:
            void Idle() {
                // Could be that we can now drop the dynamic library...
                Core::ServiceAdministrator::Instance().FlushLibraries();
            }
            void Snapshot(PluginHost::MetaData::Server& data) const
            {
                const Core::WorkerPool::Metadata& snapshot = Core::WorkerPool::Snapshot();

                for (const string& jobInfo : snapshot.Pending) {
                    data.PendingRequests.Add() = jobInfo;
                }

                for (uint8_t teller = 0; teller < snapshot.Slots; teller++) {
                    // Example of why copy-constructor and assignment constructor should be equal...
                    Core::JSON::DecUInt32 newElement;
                    data.ThreadPoolRuns.Add() = snapshot.Slot[teller];
                }
            }

        private:
            Dispatcher _dispatch;
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
            void Statistics(uint32_t& requests, uint32_t& responses, uint32_t& fileBodies, uint32_t& jsonrpc) const {
                requests = _requestFactory.Count();
                responses = _responseFactory.Count();
                fileBodies = _fileBodyFactory.Count();
                jsonrpc = _jsonRPCFactory.Count();
            }
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
                return (Core::ProxyType< Web::JSONBodyType<Core::JSONRPC::Message> >(_jsonRPCFactory.Element()));
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
                Core::Format(_text, formatter, ap);
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
            WebFlow(const Core::ProxyType<Request>& request)
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

        class CompositPlugin : public PluginHost::ICompositPlugin::INotification {
        public:
            static constexpr TCHAR RemotePluginDelimiter = '/';

        private:
            class ShellProxy : public PluginHost::IShell {
            public:
                ShellProxy() = delete;
                ShellProxy(ShellProxy&&) = delete;
                ShellProxy(const ShellProxy&) = delete;
                ShellProxy& operator= (const ShellProxy&) = delete;

                ShellProxy(PluginHost::IShell* real, const string callsign, const string& hostPlugin)
                    : _adminLock()
                    , _shell(real)
                    , _callsign(hostPlugin + RemotePluginDelimiter + callsign) {
                    _shell->AddRef();
                }
                ~ShellProxy() override {
                    _adminLock.Lock();
                    if (_shell != nullptr) {
                        _shell->Release();
                        _shell = nullptr;
                    }
                    _adminLock.Unlock();
                }

            public:
                bool IsActive() const {
                    return (_shell != nullptr);
                }
                void EnableWebServer(const string& URLPath, const string& fileSystemPath) override {
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        source->EnableWebServer(URLPath, fileSystemPath);
                        source->Release();
                    }
                }
                void DisableWebServer() override {
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        source->DisableWebServer();
                        source->Release();
                    }
                }
                //! Model: Returns a Human Readable name for the platform it is running on.
                string Model() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Model();
                        source->Release();
                    }
                    return (result);
                }
                //! Background: If enabled, the PluginHost is running in daemon mode
                bool Background() const override {
                    bool result = true;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Background();
                        source->Release();
                    }
                    return (result);
                }
                //! Accessor: Identifier that can be used for Core:NodeId to connect to the webbridge.
                string Accessor() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Accessor();
                        source->Release();
                    }
                    return (result);
                }
                //! WebPrefix: First part of the pathname in the HTTP request to select the webbridge components.
                string WebPrefix() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->WebPrefix();
                        source->Release();
                    }
                    return (result);
                }
                //! Locator: The name of the binary (so) that holds the given ClassName code.
                string Locator() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Locator();
                        source->Release();
                    }
                    return (result);
                }
                //! ClassName: Name of the class to be instantiated for this IShell
                string ClassName() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->ClassName();
                        source->Release();
                    }
                    return (result);
                }
                //! Versions: Returns a JSON Array of versions (JSONRPC interfaces) supported by this plugin.
                string Versions() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Versions();
                        source->Release();
                    }
                    return (result);
                }
                //! Callsign: Instantiation name of this specific plugin. It is the name given in the config for the classname.
                string Callsign() const override {
                    return (_callsign);
                }
                //! PersistentPath: <config:persistentpath>/<plugin:callsign>/
                string PersistentPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->PersistentPath();
                        source->Release();
                    }
                    return (result);
                }
                //! VolatilePath: <config:volatilepath>/<plugin:callsign>/
                string VolatilePath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->VolatilePath();
                        source->Release();
                    }
                    return (result);
                }
                //! DataPath: <config:datapath>/<plugin:classname>/
                string DataPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->DataPath();
                        source->Release();
                    }
                    return (result);
                }
                //! ProxyStubPath: <config:proxystubpath>/
                string ProxyStubPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->ProxyStubPath();
                        source->Release();
                    }
                    return (result);
                }
                //! SystemPath: <config:systempath>/
                string SystemPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->SystemPath();
                        source->Release();
                    }
                    return (result);
                }
                //! SystemPath: <config:apppath>/Plugins/
                string PluginPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->PluginPath();
                        source->Release();
                    }
                    return (result);
                }
                //! SystemPath: <config:systemrootpath>/
                string SystemRootPath() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->SystemRootPath();
                        source->Release();
                    }
                    return (result);
                }
                //! SystemRootPath: Set <config:systemrootpath>/
                uint32_t SystemRootPath(const string& systemRootPath) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->SystemRootPath(systemRootPath);
                        source->Release();
                    }
                    return (result);
                }
                //! Startup: <config:startup>/
                PluginHost::IShell::startup Startup() const override {
                    PluginHost::IShell::startup result = PluginHost::IShell::startup::UNAVAILABLE;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Startup();
                        source->Release();
                    }
                    return (result);
                }
                //! Startup: Set<startup,autostart,resumed states>/
                uint32_t Startup(const startup value) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Startup(value);
                        source->Release();
                    }
                    return (result);
                }
                //! Substituted Config value
                string Substitute(const string& input) const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Substitute(input);
                        source->Release();
                    }
                    return (result);
                }
                bool Resumed() const override {
                    bool result = true;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Resumed();
                        source->Release();
                    }
                    return (result);
                }
                uint32_t Resumed(const bool value) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Resumed(value);
                        source->Release();
                    }
                    return (result);
                }
                string HashKey() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->HashKey();
                        source->Release();
                    }
                    return (result);
                }
                string ConfigLine() const override {
                    string result;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->SystemRootPath();
                        source->Release();
                    }
                    return (result);
                }
                uint32_t ConfigLine(const string& config) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->ConfigLine(config);
                        source->Release();
                    }
                    return (result);
                }
                //! Return whether the given version is supported by this IShell instance.
                bool IsSupported(const uint8_t version) const override {
                    bool result = true;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->IsSupported(version);
                        source->Release();
                    }
                    return (result);
                }
                // Get access to the SubSystems and their corrresponding information. Information can be set or get to see what the
                // status of the sub systems is.
                PluginHost::ISubSystem* SubSystems() override {
                    PluginHost::ISubSystem* result = nullptr;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->SubSystems();
                        source->Release();
                    }
                    return (result);
                }
                // Notify all subscribers of this service with the given string.
                // It is expected to be JSON formatted strings as it is assumed that this is for reaching websockets clients living in
                // the web world that have build in functionality to parse JSON structs.
                void Notify(const string& message) override {
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        source->Notify(message);
                        source->Release();
                    }
                }
                // Allow access to the Shells, configured for the different Plugins found in the configuration.
                // Calling the QueryInterfaceByCallsign with an empty callsign will query for interfaces located
                // on the controller.
                void Register(IPlugin::INotification* sink) override {
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        source->Register(sink);
                        source->Release();
                    }
                }
                void Unregister(IPlugin::INotification* sink) override {
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        source->Unregister(sink);
                        source->Release();
                    }
                }
                state State() const override {
                    state result = state::DEACTIVATED;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->State();
                        source->Release();
                    }
                    return (result);
                }
                void* /* @interface:id */ QueryInterfaceByCallsign(const uint32_t id, const string& name) override {
                    void* result = nullptr;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->QueryInterfaceByCallsign(id, name);
                        source->Release();
                    }
                    return (result);
                }
                // Methods to Activate/Deactivate and Unavailable the aggregated Plugin to this shell.
                // NOTE: These are Blocking calls!!!!!
                uint32_t Activate(const reason why) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Activate(why);
                        source->Release();
                    }
                    return (result);
                }
                uint32_t Deactivate(const reason why) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Deactivate(why);
                        source->Release();
                    }
                    return (result);
                }
                uint32_t Unavailable(const reason why) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Unavailable(why);
                        source->Release();
                    }
                    return (result);
                }
                uint32_t Hibernate(const uint32_t timeout) override {
                    uint32_t result = Core::ERROR_UNAVAILABLE;
                    PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Hibernate(timeout);
                        source->Release();
                    }
                    return (result);
                }
                reason Reason() const override {
                    reason result = reason::FAILURE;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Reason();
                        source->Release();
                    }
                    return (result);
                }
                Core::hresult Metadata(string& info /* @out */) const {
                    Core::hresult result = Core::ERROR_UNAVAILABLE;
                    const PluginHost::IShell* source = Source();
                    if (source != nullptr) {
                        result = source->Metadata(info);
                        source->Release();
                    }
                    return (result);
                }

                // Method to access, in the main process space, the channel factory to submit JSON objects to be send.
                // This method will return a error if it is NOT in the main process.
                /* @stubgen:stub */
                uint32_t Submit(const uint32_t Id VARIABLE_IS_NOT_USED, const Core::ProxyType<Core::JSON::IElement>& response VARIABLE_IS_NOT_USED) override {
                    return (Core::ERROR_NOT_SUPPORTED);
                }

                // Method to access, in the main space, a COM factory to instantiate objects out-of-process.
                // This method will return a nullptr if it is NOT in the main process.
                /* @stubgen:stub */
                ICOMLink* COMLink() override {
                    return (nullptr);
                }

                BEGIN_INTERFACE_MAP(ShellProxy)
                    INTERFACE_ENTRY(PluginHost::IShell)
                END_INTERFACE_MAP

            private:
                PluginHost::IShell* Source() {
                    _adminLock.Lock();
                    PluginHost::IShell* result = _shell;
                    if (result != nullptr) {
                        result->AddRef();
                    }
                    _adminLock.Unlock();
                    return (result);
                }
                const PluginHost::IShell* Source() const {
                    _adminLock.Lock();
                    const PluginHost::IShell* result = _shell;
                    if (result != nullptr) {
                        result->AddRef();
                    }
                    _adminLock.Unlock();
                    return (result);
                }

            private:
                mutable Core::CriticalSection _adminLock;
                PluginHost::IShell* _shell;
                const string _callsign;
            };
            using CompositPlugins = std::unordered_map<string, ShellProxy*>;

        public:
            CompositPlugin(ServiceMap& parent, const string& linkPlugin)
                : _parent(parent)
                , _adminLock()
                , _linkPlugin(linkPlugin)
                , _plugins() {
            }
            ~CompositPlugin() override {
                ASSERT(_plugins.empty() == true);
            }

        public:
            const string& Callsign() const {
                return (_linkPlugin);
            }
            template<typename ACTION>
            void Visit(ACTION&& action) {
                _adminLock.Lock();
                for (std::pair<const string, ShellProxy*>& entry : _plugins) {
                    action(entry.first, entry.second);
                }
                _adminLock.Unlock();
            }
            template<typename ACTION>
            void Visit(ACTION&& action) const {
                _adminLock.Lock();
                for (const std::pair<const string, const ShellProxy*>& entry : _plugins) {
                    action(entry.first, entry.second);
                }
                _adminLock.Unlock();
            }
            void* QueryInterfaceByCallsign(const uint32_t id, const string& name) {
                void* result = nullptr;

                _adminLock.Lock();
                CompositPlugins::iterator index(_plugins.find(name));
                if (index != _plugins.end()) {
                    result = index->second->QueryInterfaceByCallsign(id, name);
                }
                _adminLock.Unlock();

                return (result);
            }
            template<typename INTERFACE>
            INTERFACE* QueryInterfaceByCallsign(const string& name) {
                return (reinterpret_cast<INTERFACE*>(QueryInterfaceByCallsign(INTERFACE::ID, name)));
            }
            uint32_t Activated(const string& callsign, PluginHost::IShell* plugin) override {
                ShellProxy* entry = nullptr;
                _adminLock.Lock();
                CompositPlugins::iterator index(_plugins.find(callsign));
                if (index == _plugins.end()) {
                    Core::ProxyType<ShellProxy> object = Core::ProxyType<ShellProxy>::Create(plugin, callsign, _linkPlugin);
                    entry = object.operator->();
                    entry->AddRef();
                    TRACE(Activity, (_T("Activated composit plugin [%s]"), entry->Callsign().c_str()));
                    _plugins.emplace(std::piecewise_construct,
                        std::make_tuple(callsign),
                        std::make_tuple(entry));
                }
                _adminLock.Unlock();

                if (entry != nullptr) {
                    _parent.Activated(entry->Callsign(), entry);
                }

                return (Core::ERROR_NONE);
            }
            uint32_t Deactivated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED) override {
                ShellProxy* entry = nullptr;

                _adminLock.Lock();
                CompositPlugins::iterator index(_plugins.find(callsign));
                if (index != _plugins.end()) {
                    entry = index->second;
                    _plugins.erase(index);
                }
                _adminLock.Unlock();

                if (entry != nullptr) {
                    _parent.Deactivated(entry->Callsign(), entry);
                    TRACE(Activity, (_T("Deactivated composit plugin [%s]"), entry->Callsign().c_str()));
                    entry->Release();
                }

                return (Core::ERROR_NONE);
            }
            void Clear() {
                std::vector<ShellProxy*> entries;

                _adminLock.Lock();
                for (auto& entry : _plugins) {
                    entries.push_back(entry.second);
                }
                _plugins.clear();
                _adminLock.Unlock();

                for (auto& element : entries) {
                    _parent.Deactivated(element->Callsign(), element);
                    element->Release();
                }
            }

            BEGIN_INTERFACE_MAP(RemoteLink)
                INTERFACE_ENTRY(PluginHost::ICompositPlugin::INotification)
            END_INTERFACE_MAP

        private:
            ServiceMap& _parent;
            mutable Core::CriticalSection _adminLock;
            const string _linkPlugin;
            CompositPlugins _plugins;
        };
        class Service : public IShell::ICOMLink, public PluginHost::Service {
        public:
            enum mode {
                CONFIGURED,
                CLONED,
                DYNAMIC
            };

        private:
            class Condition {
            private:
                enum state : uint8_t {
                    STATE_AND   = 0x00,
                    STATE_OR    = 0x01,
                    STATE_ERROR = 0x02,
                    STATE_OK    = 0x80
                };

            public:
                Condition() = delete;
                Condition(const Condition&) = delete;
                Condition& operator=(const Condition&) = delete;

                Condition(const bool ANDOperation, const Core::JSON::ArrayType<Core::JSON::EnumType<ISubSystem::subsystem>>& input)
                    : _events(0)
                    , _mask(0)
                    , _state(ANDOperation ? state::STATE_AND : state::STATE_OR)
                {
                    Core::JSON::ArrayType<Core::JSON::EnumType<ISubSystem::subsystem>>::ConstIterator index(input.Elements());

                    while (index.Next() == true) {
                        AddBit(static_cast<uint32_t>(index.Current()));
                    }

                    if (_mask == 0) {
                        _state = static_cast<state>(_state | state::STATE_OK);
                    }
                }
                ~Condition() = default;

            public:
                bool IsValid() const {
                    return (_state != state::STATE_ERROR);
                }
                bool IsMet() const
                {
                    return ((_state & state::STATE_OK) != 0);
                }
                bool Load(const std::vector<Plugin::subsystem>& input) {
                    for (const Plugin::subsystem& entry : input) {
                        AddBit(static_cast<uint32_t>(entry));
                    }

                    _state = static_cast<state>(_mask == 0 ? (_state | state::STATE_OK) : (_state & (~state::STATE_OK)));

                    return (IsValid());
                }
                inline bool Evaluate(const uint32_t subsystems)
                {
                    bool changed = false;

                    if (_mask != 0) {
                        bool compliant;

                        switch (_state & 0x03) {
                        case state::STATE_AND: compliant = (((subsystems & _mask) ^ _events) == 0);       break;
                        case state::STATE_OR:  compliant = (((subsystems & _mask) ^ _events) != _events); break;
                        default: compliant = false; break;
                        }

                        if (compliant ^ IsMet()) {
                            changed = true;
                            _state = static_cast<state>(_state ^ state::STATE_OK);
                        }
                    }

                    return (changed);
                }
                inline uint32_t Delta(const uint32_t currentSet)
                {
                    return ((currentSet & _mask) ^ _events);
                }
            private:
                void AddBit(const uint32_t input) {

                    uint32_t bitNr = input;

                    if (bitNr >= ISubSystem::NEGATIVE_START) {
                        // Its a NOT value, so this bit should *not* be set and this 0
                        bitNr -= ISubSystem::NEGATIVE_START;

                        // Make sure the event is only set once (POSITIVE or NEGATIVE)
                        if (((_mask & (1 << bitNr)) != 0) && ((_events & (1 << bitNr)) != 0)) {
                            _state = STATE_ERROR;
                        }
                    }
                    else {
                        // Make sure the event is only set once (POSITIVE or NEGATIVE)
                        if (((_mask & (1 << bitNr)) != 0) && ((_events & (1 << bitNr)) == 0)) {
                            _state = STATE_ERROR;
                        }
                        else {
                            _events |= (1 << bitNr);
                        }
                    }

                    // This bit should be taken into account if we check the condition
                    _mask |= 1 << bitNr;
                }

            private:
                uint32_t _events;
                uint32_t _mask;
                state _state;
            };
            class ControlData {
            public:
                ControlData(const ControlData&) = delete;
                ControlData& operator=(const ControlData&) = delete;
                ControlData()
                    : _isExtended(false)
                    , _state(0)
                    , _major(~0)
                    , _minor(~0)
                    , _patch(~0)
                    , _module()
                    , _precondition()
                    , _termination()
                    , _control()
                    , _versionHash() {
                }
                ~ControlData() = default;

                ControlData& operator= (const Core::IServiceMetadata* info) {
                    if (info != nullptr) {
                        const Plugin::IMetadata* extended = dynamic_cast<const Plugin::IMetadata*>(info);

                        _major = info->Major();
                        _minor = info->Minor();
                        _patch = info->Patch();
                        _module = info->Module();

                        if (extended == nullptr) {
                            _precondition.clear();
                            _termination.clear();
                            _control.clear();
                        }
                        else {
                            _isExtended = true;
                            _precondition = extended->Precondition();
                            _termination = extended->Termination();
                            _control = extended->Control();
                        }
                    }
                    return (*this);
                }

            public:
                bool IsExtended() const {
                    return (_isExtended);
                }
                bool IsValid() const {
                    return ((_major != static_cast<uint8_t>(~0)) && (_minor != static_cast<uint8_t>(~0)) && (_patch != static_cast<uint8_t>(~0)));
                }
                uint8_t Major() const {
                    return (_major);
                }
                uint8_t Minor() const {
                    return (_minor);
                }
                uint8_t Patch() const {
                    return (_patch);
                }
                const string&  Module() const {
                    return (_module);
                }
                const std::vector<PluginHost::ISubSystem::subsystem>& Precondition() const {
                    return (_precondition);
                }
                const std::vector<PluginHost::ISubSystem::subsystem>& Termination() const {
                    return (_termination);
                }
                const std::vector<PluginHost::ISubSystem::subsystem>& Control() const {
                    return (_control);
                }
                const string& Hash() const {
                    return (_versionHash);
                }
                void Hash(const string& hash) {
                    _versionHash = hash;
                }

            private:
                bool _isExtended;
                uint8_t _state;
                uint8_t _major;
                uint8_t _minor;
                uint8_t _patch;
                string _module;
                std::vector<PluginHost::ISubSystem::subsystem> _precondition;
                std::vector<PluginHost::ISubSystem::subsystem> _termination;
                std::vector<PluginHost::ISubSystem::subsystem> _control;
                string _versionHash;
            };

        public:
            Service() = delete;
            Service(const Service&) = delete;
            Service& operator=(const Service&) = delete;

            Service(const PluginHost::Config& server, const Plugin::Config& plugin, ServiceMap& administrator, const mode type)
                : PluginHost::Service(plugin, server.WebPrefix(), server.PersistentPath(), server.DataPath(), server.VolatilePath())
                , _mode(type)
                , _pluginHandling()
                , _handler(nullptr)
                , _extended(nullptr)
                , _webRequest(nullptr)
                , _webSocket(nullptr)
                , _textSocket(nullptr)
                , _rawSocket(nullptr)
                , _webSecurity(nullptr)
                , _jsonrpc(nullptr)
                , _composit(nullptr)
                , _reason(IShell::reason::SHUTDOWN)
                , _precondition(true, plugin.Precondition)
                , _termination(false, plugin.Termination)
                , _activity(0)
                , _connection(nullptr)
                , _lastId(0)
                , _metadata()
                , _library()
                , _administrator(administrator)
            {
            }
            ~Service() override
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
            inline const std::vector<PluginHost::ISubSystem::subsystem>& SubSystemControl() const {
                return (_metadata.Control());
            }
            inline const string& VersionHash() const
            {
                return (_metadata.Hash());
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
                    FileToServe(request.Path, *result, false);
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
            Core::ProxyType<Core::JSONRPC::Message> Invoke(const uint32_t channelId, const string& token, const Core::JSONRPC::Message& message)
            {
                Core::ProxyType<Core::JSONRPC::Message> response;

                Lock();

                if ( (_jsonrpc == nullptr) || (IsActive() == false) ) {
                    Unlock();

                    response = Core::ProxyType<Core::JSONRPC::Message>(IFactories::Instance().JSONRPC());
                    if(IsHibernated() == true)
                    {
                        response->Error.SetError(Core::ERROR_HIBERNATED);
                        response->Error.Text = _T("Service is hibernated");
                    }
                    else
                    {
                        response->Error.SetError(Core::ERROR_UNAVAILABLE);
                        response->Error.Text = _T("Service is not active");
                    }
                    response->Id = message.Id;
                }
                else {
                    Unlock();

#if THUNDER_RUNTIME_STATISTICS
                    IncrementProcessedRequests();
#endif
                    Core::InterlockedIncrement(_activity);
                    uint32_t result;
                    string method(message.Designator.Value());

                    if (message.Id.IsSet() == true) {
                        response = Core::ProxyType<Core::JSONRPC::Message>(IFactories::Instance().JSONRPC());
                        response->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
                        response->Id = message.Id.Value();
                    }

                    if ((result = _jsonrpc->Validate(token, method, message.Parameters.Value())) == Core::ERROR_PRIVILIGED_REQUEST) {
                        if (response.IsValid() == true) {
                            response->Error.SetError(Core::ERROR_PRIVILIGED_REQUEST);
                            response->Error.Text = _T("method invokation not allowed.");
                        }
                    }
                    else if (result == Core::ERROR_UNAVAILABLE) {
                        if (response.IsValid() == true) {
                            response->Error.SetError(Core::ERROR_PRIVILIGED_REQUEST);
                            response->Error.Text = _T("method invokation is deferred, Currently not allowed.");
                        }
                    }
                    else if (result != Core::ERROR_NONE) {
                        if (response.IsValid() == true) {
                            response->Error.SetError(Core::ERROR_PRIVILIGED_REQUEST);
                            response->Error.Text = _T("method invokation could not be validated.");
                        }
                    }
                    else {
                        string output;
                        result = _jsonrpc->Invoke(channelId, message.Id.Value(), token, method, message.Parameters.Value(), output);

                        if (response.IsValid() == true) {
                            switch (result) {
                            case Core::ERROR_NONE:
                                if (output.empty() == true) {
                                    response->Result.Null(true);
                                }
                                else {
                                    response->Result = output;
                                }
                                break;
                            case Core::ERROR_INVALID_RANGE:
                                response->Error.SetError(Core::ERROR_INVALID_RANGE);
                                response->Error.Text = _T("Requested version is not supported.");
                                break;
                            case Core::ERROR_INCORRECT_URL:
                                response->Error.SetError(Core::ERROR_INVALID_DESIGNATOR);
                                response->Error.Text = _T("Dessignator is invalid.");
                                break;
                            case Core::ERROR_BAD_REQUEST:
                                response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                                response->Error.Text = _T("Unknown method.");
                                break;
                            case Core::ERROR_FAILED_REGISTERED:
                                response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                                response->Error.Text = _T("Registration already done!!!.");
                                break;
                            case Core::ERROR_FAILED_UNREGISTERED:
                                response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                                response->Error.Text = _T("Unregister was already done!!!.");
                                break;
                            case Core::ERROR_HIBERNATED:
                                response->Error.SetError(Core::ERROR_HIBERNATED);
                                response->Error.Text = _T("The service is in an Hibernated state!!!.");
                                break;
                            case Core::ERROR_ILLEGAL_STATE:
                                response->Error.SetError(Core::ERROR_ILLEGAL_STATE);
                                response->Error.Text = _T("The service is in an illegal state!!!.");
                                break;
                            case static_cast<uint32_t>(~0):
                                response.Release();
                                break;
                            default:
                                response->Error.Code = result;
                                response->Error.Text = Core::ErrorToString(result);
                                break;
                            }
                        }
                    }
                    Core::InterlockedDecrement(_activity);
                }

                return (response);
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
                _pluginHandling.Lock();

                if (_metadata.Major() != static_cast<uint8_t>(~0)) {
                    metaData.ServiceVersion.Major = _metadata.Major();
                }
                if (_metadata.Minor() != static_cast<uint8_t>(~0)) {
                    metaData.ServiceVersion.Minor = _metadata.Minor();
                }
                if (_metadata.Patch() != static_cast<uint8_t>(~0)) {
                    metaData.ServiceVersion.Patch = _metadata.Patch();
                }
                if (_metadata.Hash().empty() == false) {
                    metaData.ServiceVersion.Hash = _metadata.Hash();
                }
                if (_metadata.IsValid() == true) {
                    metaData.Module = string(_metadata.Module());
                }

                _pluginHandling.Unlock();

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
                    if (_termination.IsMet() == false) {

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
            string SystemPath() const override {
                return (_administrator.Configuration().SystemPath());
            }
            string PluginPath() const override {
                return (_administrator.Configuration().AppPath() + _T("Plugins/"));
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
            Core::hresult Metadata(string& info /* @out */) const {
                MetaData::Service result;
                GetMetaData(result);
                result.ToString(info);
                return (Core::ERROR_NONE);
            }
            // Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
            IShell::ICOMLink* COMLink() override {
                return (this);
            }
            void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& sessionId) override
            {
                ASSERT(_connection == nullptr);

                void* result(_administrator.Instantiate(object, waitTime, sessionId, DataPath(), PersistentPath(), VolatilePath()));

                _connection = _administrator.RemoteConnection(sessionId);

                return (result);
            }
            void Register(RPC::IRemoteConnection::INotification* sink) override
            {
                _administrator.Register(sink);
            }
            void Unregister(const RPC::IRemoteConnection::INotification* sink) override
            {
                _administrator.Unregister(sink);
            }
            void Register(IShell::ICOMLink::INotification* sink)
            {
                _administrator.Register(sink);
            }
            void Unregister(IShell::ICOMLink::INotification* sink)
            {
                _administrator.Unregister(sink);
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId) override
            {
                return (_administrator.RemoteConnection(connectionId));
            }

            void Closed(const uint32_t id)
            {
                IDispatcher* dispatcher = nullptr;

                _pluginHandling.Lock();
                if (_handler != nullptr) {
                    dispatcher = _handler->QueryInterface<IDispatcher>();
                    if (dispatcher != nullptr) {
                        ILocalDispatcher* localDispatcher = dispatcher->Local();

                        if (localDispatcher) {
                            localDispatcher->Dropped(id);
                        }
                        dispatcher->Release();
                    }
                }
                _pluginHandling.Unlock();

            }

            // Methods to Activate and Deactivate the aggregated Plugin to this shell.
            // These are Blocking calls!!!!!
            Core::hresult Activate(const reason) override;
            Core::hresult Deactivate(const reason) override;
            Core::hresult Unavailable(const reason) override;

            Core::hresult Hibernate(const uint32_t timeout = 10000 /*ms*/) override;

            uint32_t Suspend(const reason why);
            uint32_t Resume(const reason why);

            reason Reason() const override
            {
                return (_reason);
            }

            bool HasVersionSupport(const string& number) const
            {
                return (number.length() > 0) && (std::all_of(number.begin(), number.end(), [](TCHAR item) { return std::isdigit(item); })) && (Service::IsSupported(static_cast<uint8_t>(atoi(number.c_str()))));
            }

            void LoadMetadata() {
                const string locator(PluginHost::Service::Configuration().Locator.Value());
                if (locator.empty() == false) {
                    Core::Library loadedLib = LoadLibrary(locator);
                    if (loadedLib.IsLoaded() == true) {
                        Core::ServiceAdministrator::Instance().ReleaseLibrary(std::move(loadedLib));
                    }
                }
            }

        private:
            uint32_t Wakeup(const uint32_t timeout);
#ifdef HIBERNATE_SUPPORT_ENABLED
            uint32_t HibernateChildren(const Core::process_t parentPID, const uint32_t timeout);
            uint32_t WakeupChildren(const Core::process_t parentPID, const uint32_t timeout);
#endif
            virtual std::vector<string> GetLibrarySearchPaths(const string& locator) const override
            {
                std::vector<string> all_paths;

                const std::vector<string> temp = _administrator.Configuration().LinkerPluginPaths();
                string rootPath(PluginHost::Service::Configuration().SystemRootPath.Value());

                if (rootPath.empty() == false) {
                    rootPath = Core::Directory::Normalize(rootPath);
                }

                if (!temp.empty())
                {
                    // additionaly defined user paths
                    for (const string& s : temp) {
                        if (rootPath.empty() == true) {
                            all_paths.push_back(Core::Directory::Normalize(s) + locator);
                        }
                        else {
                            all_paths.push_back(rootPath + Core::Directory::Normalize(s) + locator);
                        }
                    }
                }
                else if (rootPath.empty() == false)
                {
                    // system configured paths
                    all_paths.push_back(rootPath + DataPath() + locator);
                    all_paths.push_back(rootPath + PersistentPath() + locator);
                    all_paths.push_back(rootPath + SystemPath() + locator);
                    all_paths.push_back(rootPath + PluginPath() + locator);
                }
                else {
                    // system configured paths
                    all_paths.push_back(DataPath() + locator);
                    all_paths.push_back(PersistentPath() + locator);
                    all_paths.push_back(SystemPath() + locator);
                    all_paths.push_back(PluginPath() + locator);
                }

                return all_paths;
            }

            Core::Library LoadLibrary(const string& name) {
                uint8_t progressedState = 0;
                Core::Library result;

                std::vector<string> all_paths = GetLibrarySearchPaths(name);
                std::vector<string>::const_iterator iter = std::begin(all_paths);

                while ( (iter != std::end(all_paths)) && (progressedState <= 2) ) {
                    Core::File libraryToLoad(*iter);

                    if (libraryToLoad.Exists() == true) {
                        if (progressedState == 0) {
                            progressedState = 1;
                        }

                        // Loading a library, in the static initializers, might register Service::MetaData structures. As
                        // the dlopen has a process wide system lock, make sure that the, during open used lock of the 
                        // ServiceAdministrator, is already taken before entering the dlopen. This can only be achieved
                        // by forwarding this call to the ServiceAdministrator, so please so...
                        Core::Library newLib = Core::ServiceAdministrator::Instance().LoadLibrary(iter->c_str());

                        if (newLib.IsLoaded() == true) {
                            if (progressedState == 1) {
                                progressedState = 2;
                            }

                            Core::System::ModuleBuildRefImpl moduleBuildRef = reinterpret_cast<Core::System::ModuleBuildRefImpl>(newLib.LoadFunction(_T("ModuleBuildRef")));
                            Core::System::ModuleServiceMetadataImpl moduleServiceMetadata = reinterpret_cast<Core::System::ModuleServiceMetadataImpl>(newLib.LoadFunction(_T("ModuleServiceMetadata")));
                            if ((moduleBuildRef != nullptr) && (moduleServiceMetadata != nullptr)) {
                                result = newLib;
                                progressedState = 3;
                                if (_metadata.IsValid() == false) {
                                    _metadata = moduleServiceMetadata();
                                    if (_metadata.IsValid() == true) {
                                        _precondition.Load(_metadata.Precondition());
                                        _termination.Load(_metadata.Termination());
                                    }
                                    _metadata.Hash(moduleBuildRef());
                                }
                            }
                        }
                    }
                    ++iter;
                }

                if (HasError() == false) {
                    if (progressedState == 0) {
                        ErrorMessage(_T("library does not exist"));
                    }
                    else if (progressedState == 2) {
                        ErrorMessage(_T("library could not be loaded"));
                    }
                    else if (progressedState == 3) {
                        ErrorMessage(_T("library does not contain the right methods"));
                    }
                }

                return (result);
            }
            void AcquireInterfaces()
            {
                ASSERT((State() == DEACTIVATED) || (State() == PRECONDITION));

                IPlugin* newIF = nullptr;
                const string locator(PluginHost::Service::Configuration().Locator.Value());
                const string classNameString(PluginHost::Service::Configuration().ClassName.Value());
                const TCHAR* className(classNameString.c_str());
                uint32_t version(static_cast<uint32_t>(~0));

                if (locator.empty() == true) {
                    Core::ServiceAdministrator& admin(Core::ServiceAdministrator::Instance());
                    newIF = admin.Instantiate<IPlugin>(Core::Library(), className, version);
                } else {
                    _library = LoadLibrary(locator);
                    if (_library.IsLoaded() == true) {
                        if ((newIF = Core::ServiceAdministrator::Instance().Instantiate<IPlugin>(_library, className, version)) == nullptr) {
                            ErrorMessage(_T("class definitions does not exist"));
                            _library = Core::Library();
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
                    IDispatcher* jsonrpc = newIF->QueryInterface<IDispatcher>();
                    if (jsonrpc != nullptr) {
                        _jsonrpc = jsonrpc->Local();
                    }
                    _composit = newIF->QueryInterface<ICompositPlugin>();
                    if (_composit != nullptr) {
                        _administrator.AddComposit(Callsign(), _composit);
                    }
                    if (_webSecurity == nullptr) {
                        _webSecurity = _administrator.Configuration().Security();
                        _webSecurity->AddRef();
                    }

                    _pluginHandling.Lock();
                    _handler = newIF;

                    if (_metadata.IsValid() == false) {
                        _metadata = dynamic_cast<Core::IServiceMetadata*>(newIF);
                    }

                    uint32_t events = _administrator.SubSystemInfo();

                    _precondition.Evaluate(events);
                    _termination.Evaluate(events);

                    _pluginHandling.Unlock();
                }
            }
            void ReleaseInterfaces()
            {
                _pluginHandling.Lock();

                ASSERT(State() != ACTIVATED);
  
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
                if (_composit != nullptr) {
                    _administrator.RemoveComposit(Callsign(), _composit);
                    _composit->Release();
                    _composit = nullptr;
                }
                if (_connection != nullptr) {
                    // Lets record the ID associated with this connection.
                    // If the other end of this connection (indicated by the
                    // ID) is not destructed the next time we start this plugin
                    // again, we will forcefully kill it !!!
                    _lastId = _connection->Id();
                    _connection->Terminate();
                    _connection->Release();
                    _connection = nullptr;
                }

                _handler = nullptr;

                _pluginHandling.Unlock();

                if (currentIF != nullptr) {

                    currentIF->Release();
                }

                if (_library.IsLoaded() == true) {
                    Core::ServiceAdministrator::Instance().ReleaseLibrary(std::move(_library));
                }
            }

        private:
            const mode _mode;
            mutable Core::CriticalSection _pluginHandling;

            // The handlers that implement the actual logic behind the service
            IPlugin* _handler;
            IPluginExtended* _extended;
            IWeb* _webRequest;
            IWebSocket* _webSocket;
            ITextSocket* _textSocket;
            IChannel* _rawSocket;
            ISecurity* _webSecurity;
            ILocalDispatcher* _jsonrpc;
            ICompositPlugin* _composit;
            reason _reason;
            Condition _precondition;
            Condition _termination;
            uint32_t _activity;
            RPC::IRemoteConnection* _connection;
            uint32_t _lastId;
            ControlData _metadata;
            Core::Library _library;
            void* _hibernateStorage;
            ServiceMap& _administrator;
            static Core::ProxyType<Web::Response> _unavailableHandler;
            static Core::ProxyType<Web::Response> _missingHandler;
        };
        class Override : public Core::JSON::Container {
        private:
            class Plugin : public Core::JSON::Container {
            public:
                Plugin& operator=(Plugin const& other) = delete;

                Plugin()
                    : Core::JSON::Container()
                    , Configuration(_T("{}"), false)
                    , SystemRootPath()
                    , Startup()
                    , Resumed()
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startup"), &Startup);
                    Add(_T("resumed"), &Resumed);
                }
                Plugin(const string& config, const string& systemRootPath, const PluginHost::IShell::startup value, const bool resumed)
                    : Core::JSON::Container()
                    , Configuration(config, false)
                    , SystemRootPath(systemRootPath)
                    , Startup(value)
                    , Resumed(resumed)
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startup"), &Startup);
                    Add(_T("resumed"), &Resumed);
                }
                Plugin(Plugin const& copy)
                    : Core::JSON::Container()
                    , Configuration(copy.Configuration)
                    , SystemRootPath(copy.SystemRootPath)
                    , Startup(copy.Startup)
                    , Resumed(copy.Resumed)
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startup"), &Startup);
                    Add(_T("resumed"), &Resumed);
                }

                ~Plugin() override = default;

            public:
                Core::JSON::String Configuration;
                Core::JSON::String SystemRootPath;
                Core::JSON::EnumType<PluginHost::IShell::startup> Startup;
                Core::JSON::Boolean Resumed;
            };

            using Callsigns = std::unordered_map<string, Plugin>;

        public:
            Override(const Override&) = delete;
            Override& operator=(const Override&) = delete;

            Override(PluginHost::Config& serverconfig, ServiceMap& services, const string& persitentFile)
                : Services()
                , Prefix(serverconfig.Prefix())
                , IdleTime(serverconfig.IdleTime())
                , _services(services)
                , _serverconfig(serverconfig)
                , _fileName(persitentFile)
                , _callsigns()
            {
                Add(_T("Services"), &Services);

                // Add all service names (callsigns) that are not yet in there...
                ServiceMap::Iterator service(services.Services());

                while (service.Next() == true) {
                    const string& name(service->Callsign());

                    // Create an element for this service with its callsign.
                    Callsigns::iterator index(_callsigns.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(name),
                        std::forward_as_tuple(_T("{}"), "", PluginHost::IShell::startup::UNAVAILABLE, false)).first);

                    // Store the override config in the JSON String created in the map
                    Services.Add(index->first.c_str(), &(index->second));
                }

                Add(_T("prefix"), &Prefix);
                Add(_T("idletime"), &IdleTime);

            }
            ~Override() = default;

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

                    _serverconfig.SetPrefix(Prefix.Value());
                    _serverconfig.SetIdleTime(IdleTime.Value());
                    // Convey the real JSON struct information into the specific services.
                    ServiceMap::Iterator index(_services.Services());

                    while (index.Next() == true) {

                        Callsigns::const_iterator current(_callsigns.find(index->Callsign()));

                        // ServiceMap should *NOT* change runtime...
                        ASSERT(current != _callsigns.end());

                        if (current->second.IsSet() == true) {
                            if (current->second.Configuration.IsSet() == true) {
                                (*index)->Configuration(current->second.Configuration.Value());
                            }
                            if (current->second.SystemRootPath.IsSet() == true) {
                                (*index)->SystemRootPath(current->second.SystemRootPath.Value());
                            }
                            if (current->second.Startup.IsSet() == true) {
                                (*index)->Startup(current->second.Startup.Value());
                            }
                            if (current->second.Resumed.IsSet() == true) {
                                (*index)->Resumed(current->second.Resumed.Value());
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

                    Prefix    = _serverconfig.Prefix();
                    IdleTime  = _serverconfig.IdleTime();

                    // Convey the real information from he specific services into the JSON struct.
                    ServiceMap::Iterator index(_services.Services());

                    while (index.Next() == true) {

                        Callsigns::iterator current(_callsigns.find(index->Callsign()));

                        // ServiceMap should *NOT* change runtime...
                        ASSERT(current != _callsigns.end());

                        string config((*index)->Configuration());

                        if (config.empty() == true) {
                            current->second.Configuration = _T("{}");
                        } else {
                            current->second.Configuration = config;
                        }
                        current->second.SystemRootPath = (index)->SystemRootPath();
                        current->second.Startup = (index)->Startup();
                        current->second.Resumed = (index)->Resumed();
                    }

                    // Persist the currently set information
                    IElement::ToFile(storage);

                    storage.Close();
                } else {
                    result = storage.ErrorCode();
                }

                return (result);
            }

            Core::JSON::Container Services;

            Core::JSON::String Prefix;
            Core::JSON::DecUInt16 IdleTime;

        private:
            ServiceMap& _services;
            PluginHost::Config& _serverconfig;
            string _fileName;
            Callsigns _callsigns;
        };

        class ServiceMap {
        public:
            using ServiceContainer = std::map<string, Core::ProxyType<Service>>;
            using Notifiers = std::vector<PluginHost::IPlugin::INotification*>;
            using Iterator = Core::IteratorMapType<ServiceContainer, Core::ProxyType<Service>, const string&>;
            using RemoteInstantiators = std::unordered_map<string, IRemoteInstantiation*>;
            using CompositPlugins = std::list< Core::Sink<CompositPlugin> >;

        private:
            class CommunicatorServer : public RPC::Communicator {
            private:
                using Observers = std::vector<IShell::ICOMLink::INotification*>;

                class RemoteHost : public RPC::Communicator::RemoteConnection {
                private:
                    friend class Core::Service<RemoteHost>;

                public:
                    RemoteHost(RemoteHost&&) = delete;
                    RemoteHost(const RemoteHost&) = delete;
                    RemoteHost& operator=(const RemoteHost&) = delete;

                    RemoteHost(const RPC::Object& instance, const RPC::Config& config)
                        : RemoteConnection()
                        , _object(instance)
                        , _config(config)
                    {
                    }
                    ~RemoteHost() override
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
                                    _object.SystemRootPath(),
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
                class ProxyStubObserver : public Core::FileSystemMonitor::ICallback {
                public:
                    ProxyStubObserver() = delete;
                    ProxyStubObserver(ProxyStubObserver&&) = delete;
                    ProxyStubObserver(const ProxyStubObserver&) = delete;
                    ProxyStubObserver& operator= (const ProxyStubObserver&) = delete;

                    ProxyStubObserver(CommunicatorServer& parent,const string& observableProxyStubPath)
                        : _parent(parent)
                        , _observerPath(observableProxyStubPath)  {
                        if (_observerPath.empty() == false) {
                            if (Core::FileSystemMonitor::Instance().Register(this, _observerPath) == false) {
                                _observerPath.clear();
                            }
                        }
                    }
                    ~ProxyStubObserver() override {
                        if (_observerPath.empty() == false) {
                            Core::FileSystemMonitor::Instance().Unregister(this, _observerPath);
                        }
                    }

                public:
                    bool IsValid() const {
                        return (_observerPath.empty() == false);
                    }
                    void Updated() override {
                        _parent.Reload(_observerPath);
                    }

                private:
                    CommunicatorServer& _parent;
                    string _observerPath;
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
                    const string& observableProxyStubPath,
                    const string& postMortemPath,
                    const uint8_t softKillCheckWaitTime,
                    const uint8_t hardKillCheckWaitTime,
                    const Core::ProxyType<RPC::InvokeServer>& handler)
                    : RPC::Communicator(node, ProxyStubPathCreator(proxyStubPath, observableProxyStubPath), Core::ProxyType<Core::IIPCServer>(handler))
                    , _parent(parent)
                    , _persistentPath(persistentPath)
                    , _systemPath(systemPath)
                    , _dataPath(dataPath)
                    , _volatilePath(volatilePath)
                    , _appPath(appPath)
                    , _postMortemPath(postMortemPath)
#ifdef __WINDOWS__
                    , _application(_systemPath + EXPAND_AND_QUOTE(HOSTING_COMPROCESS))
#else
                    , _application(EXPAND_AND_QUOTE(HOSTING_COMPROCESS))
#endif
                    , _adminLock()
                    , _requestObservers()
                    , _proxyStubObserver(*this, observableProxyStubPath)
                {
                    // Make sure the engine knows how to call the Announcment handler..
                    handler->Announcements(Announcement());

                    if (RPC::Communicator::Open(RPC::CommunicationTimeOut) != Core::ERROR_NONE) {
                        TRACE_L1("We can not open the RPC server. No out-of-process communication available. %d", __LINE__);
                    } else {
                        // We need to pass the communication channel NodeId via an environment variable, for process,
                        // not being started by the rpcprocess...
                        Core::SystemInfo::SetEnvironment(string(CommunicatorConnector), RPC::Communicator::Connector());
                        RPC::Communicator::ForcedDestructionTimes(softKillCheckWaitTime, hardKillCheckWaitTime);
                    }

                    if (observableProxyStubPath.empty() == true) {
                        SYSLOG(Logging::Startup, (_T("Dynamic COMRPC disabled.")));
                    } else if (_proxyStubObserver.IsValid() == false) {
                        SYSLOG(Logging::Startup, (_T("Dynamic COMRPC failed. Can not observe: [%s]"), observableProxyStubPath.c_str()));
                    }
                }
                virtual ~CommunicatorServer()
                {
                    ASSERT(_requestObservers.size() == 0 && "Sink for ICOMLink::INotifications not unregistered!");
                    Observers::iterator index(_requestObservers.begin());
                    while (index != _requestObservers.end()) {
                        (*index)->Release();
                        index++;
                }
                    _requestObservers.clear();
                }

            public:
                void* Create(uint32_t& connectionId, const RPC::Object& instance, const uint32_t waitTime, const string& dataPath, const string& persistentPath, const string& volatilePath)
                {
                    return (RPC::Communicator::Create(connectionId, instance, RPC::Config(RPC::Communicator::Connector(), _application, persistentPath, _systemPath, dataPath, volatilePath, _appPath, RPC::Communicator::ProxyStubPath(), _postMortemPath), waitTime));
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
                    return (RPC::Communicator::ProxyStubPath());
                }
                const string& PostMortemPath() const
                {
                    return (_postMortemPath);
                }
                const string& Application() const
                {
                    return (_application);
                }
                void Register(RPC::IRemoteConnection::INotification* sink)
                {
                    RPC::Communicator::Register(sink);
                }
                void Unregister(const RPC::IRemoteConnection::INotification* sink)
                {
                    RPC::Communicator::Unregister(sink);
                }
                void Register(IShell::ICOMLink::INotification* sink)
                {
                    ASSERT(sink != nullptr);

                    if (sink != nullptr) {

                        _adminLock.Lock();

                        Observers::iterator index = std::find(_requestObservers.begin(), _requestObservers.end(), sink);

                        ASSERT(index == _requestObservers.end());

                        if (index == _requestObservers.end()) {
                            sink->AddRef();
                            _requestObservers.push_back(sink);
                        }

                        _adminLock.Unlock();
                    }
                }
                void Unregister(IShell::ICOMLink::INotification* sink)
                {
                    ASSERT(sink != nullptr);

                    if (sink != nullptr) {

                        _adminLock.Lock();

                        Observers::iterator index = std::find(_requestObservers.begin(), _requestObservers.end(), sink);

                        ASSERT(index != _requestObservers.end());

                        if (index != _requestObservers.end()) {
                            (*index)->Release();
                            _requestObservers.erase(index);
                        }

                        _adminLock.Unlock();
                    }
                }

            private:
                void Reload(const string& path) {
                    TRACE(Activity, (Core::Format(_T("Reloading ProxyStubs from %s."), path.c_str())));
                    RPC::Communicator::LoadProxyStubs(path);
                }
                string ProxyStubPathCreator(const string& proxyStubPath, const string& observableProxyStubPath) {
                    string concatenatedPath;

                    if (proxyStubPath.empty() == false) {
                        concatenatedPath = proxyStubPath;
                    }
                    if (observableProxyStubPath.empty() ==false) {
                        if (concatenatedPath.empty() == true) {
                            concatenatedPath = observableProxyStubPath;
                        }
                        else {
                            concatenatedPath = concatenatedPath + '|' + observableProxyStubPath;
                        }
                    }
                    return (concatenatedPath);
                }
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

                void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t version) override
                {
                    return (_parent.Acquire(interfaceId, className, version));
                }

                void Dangling(const Core::IUnknown* source, const uint32_t interfaceId) override
                {
                    _adminLock.Lock();

                    _parent.Dangling(source, interfaceId);

                    for (auto& observer : _requestObservers) {
                        observer->Dangling(source, interfaceId);
                    }

                    _adminLock.Unlock();

                    TRACE(Activity, (_T("Dangling resource cleanup of interface: 0x%X"), interfaceId));
                }

                void Revoke(const Core::IUnknown* remote, const uint32_t interfaceId) override
                {
                    if (interfaceId == PluginHost::IPlugin::INotification::ID) {
                        const PluginHost::IPlugin::INotification* notification = remote->QueryInterface<const PluginHost::IPlugin::INotification>();

                        ASSERT(notification != nullptr);

                        _parent.Unregister(notification);
                        notification->Release();
                    }

                    _adminLock.Lock();

                    for (auto& observer : _requestObservers) {
                        observer->Revoked(remote, interfaceId);
                    }

                    _adminLock.Unlock();
                }

            private:
                ServiceMap& _parent;
                const string _persistentPath;
                const string _systemPath;
                const string _dataPath;
                const string _volatilePath;
                const string _appPath;
                const string _observableProxyStubPath;
                const string _postMortemPath;
                const string _application;
                mutable Core::CriticalSection _adminLock;
                Observers _requestObservers;
                ProxyStubObserver _proxyStubObserver;
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
                    const string& systemRootPath,
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
                    RPC::Object instance(libraryName, className, callsign, interfaceId, version, user, group, threads, priority, RPC::Object::HostType::LOCAL, systemRootPath, _T(""), configuration);

                    RPC::Communicator::Process process(requestId, config, instance);

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
            class SubSystems : public Core::IDispatch, public SystemInfo {
            private:
                class Job {
                public:
                    Job() = delete;
                    Job(const Job&) = delete;
                    Job& operator=(const Job&) = delete;

                    Job(SubSystems& parent)
                        : _parent(parent)
                    {
                    }
                    ~Job() = default;

                public:
                    void Dispatch() {
                        _parent.Evaluate();
                    }
                    string JobIdentifier() const {
                        return(_T("PluginServer::SubSystems::Notification"));
                    }

                private:
                    SubSystems& _parent;
                };

            public:
                SubSystems() = delete;
                SubSystems(const SubSystems&) = delete;
                SubSystems& operator=(const SubSystems&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
                SubSystems(ServiceMap* parent)
                    : SystemInfo(parent->Configuration(), this)
                    , _parent(*parent)
                    , _job(*this)
                {
                }
POP_WARNING()
                ~SubSystems() override
                {
                    Core::ProxyType<Core::IDispatch> job(_job.Revoke());

                    if (job.IsValid()) {
                        _parent.WorkerPool().Revoke(job);
                        _job.Revoked();
                    }
                }

            private:
                void Dispatch() override
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

                    Core::ProxyType<Core::IDispatch> job(_job.Submit());
                    if (job.IsValid() == true) {
                        _parent.WorkerPool().Submit(job);
                    }
                }
                inline void Evaluate()
                {
                    _parent.Evaluate();
                }

            private:
                ServiceMap& _parent;
                Core::ThreadPool::JobType<Job> _job;
            };
            class ConfigObserver : public Core::FileSystemMonitor::ICallback {
            public:
                ConfigObserver() = delete;
                ConfigObserver(ConfigObserver&&) = delete;
                ConfigObserver(const ConfigObserver&) = delete;
                ConfigObserver& operator= (const ConfigObserver&) = delete;

                ConfigObserver(ServiceMap& parent, const string& observableConfigPath)
                    : _parent(parent)
                    , _observerPath(observableConfigPath) {
                    if (_observerPath.empty() == false) {
                        if (Core::FileSystemMonitor::Instance().Register(this, _observerPath) == false) {
                            _observerPath.clear();
                        }
                    }
                }
                ~ConfigObserver() override {
                    if (_observerPath.empty() == false) {
                        Core::FileSystemMonitor::Instance().Unregister(this, _observerPath);
                    }
                }

            public:
                bool IsValid() const {
                    return (_observerPath.empty() == false);
                }
                void Updated() override {
                    _parent.ConfigReload(_observerPath);
                }

            private:
                ServiceMap& _parent;
                string _observerPath;
            };

        public:
            ServiceMap() = delete;
            ServiceMap(const ServiceMap&) = delete;
            ServiceMap& operator=(const ServiceMap&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST);
            ServiceMap(Server& server)
                : _server(server)
                , _adminLock()
                , _notificationLock()
                , _services()
                , _notifiers()
                , _engine(Core::ProxyType<RPC::InvokeServer>::Create(&(server._dispatcher)))
                , _processAdministrator(
                    *this,
                    server._config.Communicator(),
                    server._config.PersistentPath(),
                    server._config.SystemPath(),
                    server._config.DataPath(),
                    server._config.VolatilePath(),
                    server._config.AppPath(),
                    server._config.ProxyStubPath(),
                    server._config.ObservableProxyStubPath(),
                    server._config.PostMortemPath(),
                    server._config.SoftKillCheckWaitTime(),
                    server._config.HardKillCheckWaitTime(),
                    _engine)
                , _subSystems(this)
                , _authenticationHandler(nullptr)
                , _configObserver(*this, server._config.PluginConfigPath())
                , _compositPlugins()
            {
                if (server._config.PluginConfigPath().empty() == true) {
                    SYSLOG(Logging::Startup, (_T("Dynamic configs disabled.")));
                } else if (_configObserver.IsValid() == false) {
                    SYSLOG(Logging::Startup, (_T("Dynamic configs failed. Can not observe: [%s]"), server._config.PluginConfigPath().c_str()));
                }
            }
            POP_WARNING();
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
                        _server.Dispatcher().SecurityRevoke(Configuration().Security());
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
                    result = Configuration().Security();
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
            void Initialize(const string& callsign, PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(_notifiers.begin());

                while (index != _notifiers.end()) {
                    PluginHost::IPlugin::ILifeTime* lifetime = (*index)->QueryInterface<PluginHost::IPlugin::ILifeTime>();
                    if (lifetime != nullptr) {
                        lifetime->Initialize(callsign, entry);
                        lifetime->Release();
                    }
                    index++;
                }

                _notificationLock.Unlock();
            }
            void Activated(const string& callsign, PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(_notifiers.begin());

                while (index != _notifiers.end()) {
                    (*index)->Activated(callsign, entry);
                    index++;
                }

                _notificationLock.Unlock();
            }
            void Deactivated(const string& callsign, PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(_notifiers.begin());

                while (index != _notifiers.end()) {
                    (*index)->Deactivated(callsign, entry);
                    index++;
                }

                _notificationLock.Unlock();
            }
            void Deinitialized(const string& callsign, PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(_notifiers.begin());

                while (index != _notifiers.end()) {
                    PluginHost::IPlugin::ILifeTime* lifetime = (*index)->QueryInterface<PluginHost::IPlugin::ILifeTime>();
                    if (lifetime != nullptr) {
                        lifetime->Deinitialized(callsign, entry);
                        lifetime->Release();
                    }
                    index++;
                }

                _notificationLock.Unlock();
            }
            void Unavailable(const string& callsign, PluginHost::IShell* entry)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(_notifiers.begin());

                while (index != _notifiers.end()) {
                    (*index)->Unavailable(callsign, entry);
                    index++;
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
                ServiceContainer::iterator index(_services.begin());

                // Notifty all plugins that we have sofar..
                while (index != _services.end()) {
                    ASSERT(index->second.IsValid());

                    Core::ProxyType<Service> service(index->second);

                    ASSERT(service.IsValid());

                    if ( (service.IsValid() == true) && (service->State() == IShell::ACTIVATED) ) {
                        sink->Activated(service->Callsign(), &(service.operator*()));
                    }

                    index++;
                }
                for (Core::Sink<CompositPlugin>& entry : _compositPlugins) {
                    entry.Visit([&](const string& callsign, IShell* proxy) {
                        sink->Activated(callsign, proxy);
                    });
                }

                _notificationLock.Unlock();
            }
            void Unregister(const PluginHost::IPlugin::INotification* sink)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(std::find(_notifiers.begin(), _notifiers.end(), sink));

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

                Core::ProxyType<IShell> service;

                FromIdentifier(callsign, service);

                if (service.IsValid() == true) {

                    result = service->QueryInterface(id);
                }

                return (result);
            }

            void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& sessionId, const string& dataPath, const string& persistentPath, const string& volatilePath)
            {
                return (_processAdministrator.Create(sessionId, object, waitTime, dataPath, persistentPath, volatilePath));
            }
            void Destroy(const uint32_t id) {
                _processAdministrator.Destroy(id);
            }
            void Register(RPC::IRemoteConnection::INotification* sink)
            {
                _processAdministrator.Register(sink);
            }
            void Unregister(const RPC::IRemoteConnection::INotification* sink)
            {
                _processAdministrator.Unregister(sink);
            }
            void Register(IShell::ICOMLink::INotification* sink)
            {
                _processAdministrator.Register(sink);
            }
            void Unregister(IShell::ICOMLink::INotification* sink)
            {
                _processAdministrator.Unregister(sink);
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId)
            {
                return (connectionId != 0 ? _processAdministrator.Connection(connectionId) : nullptr);
            }
            void Closed(const uint32_t id) {
                _adminLock.Lock();

                // First stop all services running ...
                ServiceContainer::iterator index(_services.begin());

                while (index != _services.end()) {
                    index->second->Closed(id);
                    ++index;
                }

                _adminLock.Unlock();
            }
            inline Core::ProxyType<Service> Insert(const Plugin::Config& configuration, const Service::mode mode)
            {
                // Whatever plugin is needse, we at least have our MetaData plugin available (as the first entry :-).
                Core::ProxyType<Service> newService(Core::ProxyType<Service>::Create(Configuration(), configuration, *this, mode));

                if (newService.IsValid() == true) {
                    _adminLock.Lock();

                    // Fire up the interface. Let it handle the messages.
                    _services.insert(std::pair<const string, Core::ProxyType<Service>>(configuration.Callsign.Value(), newService));

                    _adminLock.Unlock();
                }

                return (newService);
            }

            inline uint32_t Clone(const Core::ProxyType<IShell>& originalShell, const string& newCallsign, Core::ProxyType<IShell>& newService)
            {
                uint32_t result = Core::ERROR_GENERAL;
                const Core::ProxyType<Service> original = Core::ProxyType<Service>(originalShell);

                ASSERT(original.IsValid());

                _adminLock.Lock();

                if ((original.IsValid() == true) && (_services.find(newCallsign) == _services.end())) {
                    // Copy original configuration
                    Plugin::Config newConfiguration;
                    newConfiguration.FromString(original->Configuration());
                    newConfiguration.Callsign = newCallsign;

                    Core::ProxyType<Service> clone = Core::ProxyType<Service>::Create(Configuration(), newConfiguration, *this, Service::mode::CLONED);

                    if (newService.IsValid() == true) {
                        // Fire up the interface. Let it handle the messages.
                        _services.emplace(
                            std::piecewise_construct,
                            std::forward_as_tuple(newConfiguration.Callsign.Value()),
                            std::forward_as_tuple(clone));

                        clone->Evaluate();
                        newService = Core::ProxyType<IShell>(clone);

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
                ServiceContainer::iterator index(_services.find(callSign));

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

                std::vector<Core::ProxyType<Service>> duplicates;
                duplicates.reserve(_services.size());
                ServiceContainer::const_iterator index(_services.begin());

                while (index != _services.end()) {
                    duplicates.emplace_back(index->second);
                    index++;
                }

                _adminLock.Unlock();

                for (Core::ProxyType<Service> info : duplicates) {
                    string data;
                    info->Metadata(data);
                    MetaData::Service&  entry = metaData.Add();
                    entry.FromString(data);
                }

                for (const Core::Sink<CompositPlugin>& entry : _compositPlugins) {
                    entry.Visit([&](const string& callsign, const IShell* proxy)
                        {
                            string data;
                            proxy->Metadata(data);
                            MetaData::Service& entry = metaData.Add();
                            entry.FromString(data);
                            entry.Callsign = callsign;
                    });
                }
            }
            void GetMetaData(Core::JSON::ArrayType<MetaData::Channel>& metaData) const
            {
                _adminLock.Lock();
                _processAdministrator.Visit([&](const RPC::Communicator::Client& element)
                    {
                        MetaData::Channel& entry = metaData.Add();
                        entry.ID = element.Extension().Id();
                        
                        entry.Activity = element.Source().IsOpen();
                        entry.JSONState = MetaData::Channel::state::COMRPC;
                        entry.Name = string(EXPAND_AND_QUOTE(APPLICATION_NAME) "::Communicator");

                        string identifier = ChannelIdentifier(element.Source());

                        if (identifier.empty() == false) {
                            entry.Remote = identifier;
                        }
                    });
                _adminLock.Unlock();
            }
            uint32_t FromIdentifier(const string& callSign, Core::ProxyType<IShell>& service)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;
                size_t pos;

                if ((pos = callSign.find_first_of(CompositPlugin::RemotePluginDelimiter)) != string::npos) {
                    // This is a Composit plugin identifier..
                    string linkingPin = callSign.substr(0, pos);
                    _adminLock.Lock();
                    CompositPlugins::iterator index(_compositPlugins.begin());
                    while ((index != _compositPlugins.end()) && (index->Callsign() != linkingPin)) {
                        index++;
                    }
                    if (index != _compositPlugins.end()) {
                        IShell* found = index->QueryInterfaceByCallsign<IShell>(callSign.substr(pos + 1));

                        if (found != nullptr) {
                            service = Core::ProxyType<IShell>(static_cast<Core::IReferenceCounted&>(*found), *found);
                        }
                    }
                    _adminLock.Unlock();
                }
                else {
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
                            }
                            else if (callSign[length] == '.') {
                                // Requested specific version of a plugin
                                if (index.second->HasVersionSupport(callSign.substr(length + 1)) == true) {
                                    // Requested version of service is supported!
                                    service = index.second;
                                    result = Core::ERROR_NONE;
                                }
                                else {
                                    // Requested version is not supported
                                    result = Core::ERROR_INVALID_SIGNATURE;
                                }
                                break;
                            }
                        }
                    }

                    _adminLock.Unlock();
                }

                return (result);
            }

            uint32_t FromLocator(const string& identifier, Core::ProxyType<Service>& service, bool& serviceCall);

            void Destroy();

            inline const PluginHost::Config& Configuration() const
            {
                return _server.Configuration();
            }
            void AddComposit(const string& callsign, ICompositPlugin* composit) {
                _adminLock.Lock();
                // Seems this is a plugin that can be a composition of more plugins..
                _compositPlugins.emplace_back(*this, callsign);
                composit->Register(&_compositPlugins.back());
                _adminLock.Unlock();
            }
            void RemoveComposit(const string& callsign, ICompositPlugin* composit) {
                // Seems this is a plugin that can be a composition of more plugins..
                _adminLock.Lock();
                CompositPlugins::iterator index(_compositPlugins.begin());
                while ((index != _compositPlugins.end()) && (callsign != index->Callsign())) {
                    index++;
                }

                if (index != _compositPlugins.end()) {
                    index->Clear();
                    composit->Unregister(&(*index));
                    _compositPlugins.erase(index);
                }

                _adminLock.Unlock();
            }

        private:
            void Dangling(const Core::IUnknown* source, const uint32_t interfaceId) {
                if (interfaceId == RPC::IRemoteConnection::INotification::ID)
                {
                    const RPC::IRemoteConnection::INotification* base = source->QueryInterface<RPC::IRemoteConnection::INotification>();

                    ASSERT(base != nullptr);

                    if (base != nullptr) {
                        TRACE(Activity, (_T("Unregistered the dangling: RPC::IRemoteConnection::INotification")));
                        _processAdministrator.Unregister(base);
                        base->Release();
                    }
                }
                else if (interfaceId == PluginHost::IPlugin::INotification::ID) {
                    const PluginHost::IPlugin::INotification* base = source->QueryInterface<PluginHost::IPlugin::INotification>();

                    ASSERT(base != nullptr);

                    if (base != nullptr) {
                        _notificationLock.Lock();

                        Notifiers::iterator index(std::find(_notifiers.begin(), _notifiers.end(), base));

                        if (index != _notifiers.end()) {
                            (*index)->Release();
                            _notifiers.erase(index);
                            TRACE(Activity, (_T("Unregistered the dangling: PluginHost::IPlugin::INotification")));
                        }
                        _notificationLock.Unlock();
                    }
                }
            }
            void ConfigReload(const string& configs) {
                // Oke lets check the configs we are observing :-)
                Core::Directory pluginDirectory(configs.c_str(), _T("*.json"));

                while (pluginDirectory.Next() == true) {
                    Core::File file(pluginDirectory.Current());

                    if (file.IsDirectory() == false) { 
                        if (file.Open(true) == false) {
                            SYSLOG_GLOBAL(Logging::Fatal, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
                        }
                        else {
                            Plugin::Config pluginConfig;
                            Core::OptionalType<Core::JSON::Error> error;
                            pluginConfig.IElement::FromFile(file, error);
                            if (error.IsSet() == true) {
                                SYSLOG_GLOBAL(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                            }
                            else if ((pluginConfig.ClassName.Value().empty() == true) || (pluginConfig.Locator.Value().empty() == true)) {
                                SYSLOG_GLOBAL(Logging::Fatal, (_T("Plugin config file [%s] does not contain classname or locator."), file.Name().c_str()));
                            }
                            else {
                                if (pluginConfig.Callsign.Value().empty() == true) {
                                    pluginConfig.Callsign = Core::File::FileName(file.FileName());
                                }

                                Insert(pluginConfig, Service::mode::DYNAMIC);
                            }
                            file.Close();
                        }
                    }
                }
            }
            void Remove(const string& connector) const
            {
                // This is already locked by the callee, so safe to operate on the map..
                RemoteInstantiators::const_iterator index(_instantiators.find(connector));

                if (index == _instantiators.cend()) {
                    _instantiators.erase(index);
                }
            }
            void* Acquire(const uint32_t interfaceId, const string& className, const uint32_t version)
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
            void RecursiveNotification(ServiceContainer::iterator& index)
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
                ServiceContainer::iterator index(_services.begin());

                RecursiveNotification(index);
            }
            inline Core::WorkerPool& WorkerPool()
            {
                return (_server.WorkerPool());
            }

        private:
            Server& _server;
            mutable Core::CriticalSection _adminLock;
            Core::CriticalSection _notificationLock;
            ServiceContainer _services;
            mutable RemoteInstantiators _instantiators;
            Notifiers _notifiers;
            Core::ProxyType<RPC::InvokeServer> _engine;
            CommunicatorServer _processAdministrator;
            Core::Sink<SubSystems> _subSystems;
            IAuthenticate* _authenticationHandler;
            ConfigObserver _configObserver;
            CompositPlugins _compositPlugins;
        };

        // Connection handler is the listening socket and keeps track of all open
        // Links. A Channel is identified by an ID, this way, whenever a link dies
        // (is closed) during the service process, the ChannelMap will
        // not find it and just "flush" the presented work.
        class Channel : public PluginHost::Channel {
        public:
            class Job : public Core::IDispatch {
            public:
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;

                Job()
                    : _ID(~0)
                    , _server(nullptr)
                    , _service()
                {
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
                const Service& GetService() const
                {
                    ASSERT(HasService() == true);
                    return *_service;
                }
                Service& GetService()
                {
                    ASSERT(HasService() == true);
                    return *_service;
                }
                void Clear()
                {
                    _ID = ~0;
                    if (_service.IsValid() == true) {
                        _service.Release();
                    }
                }
                void Set(const uint32_t id, Server* server, Core::ProxyType<Service>& service)
                {
                    ASSERT(_service.IsValid() == false);
                    ASSERT(_ID == static_cast<uint32_t>(~0));
                    ASSERT(id != static_cast<uint32_t>(~0));

                    _ID = id;
                    _service = service;
                    _server = server;
                }
                string Process(const string& message)
                {
                    string result;
                    REPORT_DURATION_WARNING( { result = _service->Inbound(_ID, message); }, WarningReporting::TooLongInvokeMessage, message);  
                    return result;
                }
                Core::ProxyType<Core::JSONRPC::Message> Process(const string& token, const Core::ProxyType<Core::JSONRPC::Message>& message)
                {
                    Core::ProxyType<Core::JSONRPC::Message> result;
                    REPORT_DURATION_WARNING( { result = _service->Invoke(_ID, token, *message); }, WarningReporting::TooLongInvokeMessage, *message);  
                    return result;
                }
                Core::ProxyType<Web::Response> Process(const Core::ProxyType<Web::Request>& message)
                {
                    Core::ProxyType<Web::Response> result;
                    REPORT_DURATION_WARNING( { result = _service->Process(*message); }, WarningReporting::TooLongInvokeMessage, *message);  
                    return result;
                }
                Core::ProxyType<Core::JSON::IElement> Process(const Core::ProxyType<Core::JSON::IElement>& message)
                {
                    Core::ProxyType<Core::JSON::IElement> result;
                    REPORT_DURATION_WARNING( { result = _service->Inbound(_ID, message); }, WarningReporting::TooLongInvokeMessage, *message);  
                    return result;
                }
                template <typename PACKAGE>
                void Submit(PACKAGE package)
                {
                    ASSERT(_server != nullptr);
                    _server->Dispatcher().Submit(_ID, package);
                }
                void RequestClose() {
                    ASSERT(_server != nullptr);
                    _server->Dispatcher().RequestClose(_ID);
                }
                string Callsign() const {
                    ASSERT(_service.IsValid() == true);
                    return _service->Callsign();
                }
            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
            };
            class WebRequestJob : public Job {
            public:
                WebRequestJob(const WebRequestJob&) = delete;
                WebRequestJob& operator=(const WebRequestJob&) = delete;

                WebRequestJob()
                    : Job()
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
                void Set(const uint32_t id, Server* server, Core::ProxyType<Service>& service, Core::ProxyType<Web::Request>& request, const string& token, const bool JSONRPC)
                {
                    Job::Set(id, server, service);

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
                        if(_request->Verb == Request::HTTP_POST){
                            Core::ProxyType<Core::JSONRPC::Message> message(_request->Body<Core::JSONRPC::Message>());
                            if (HasService() == true) {
                                message->ImplicitCallsign(GetService().Callsign());
                            }

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
                            response = IFactories::Instance().Response();
                            response->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;
                            response->Message = _T("JSON-RPC only supported via POST request");
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
                string Identifier() const override {
                    string identifier;
                    if (_jsonrpc == false) {
                        identifier = _T("{ \"type\": \"JSON\",  }");
                    }
                    else {
                        Core::ProxyType<Core::JSONRPC::Message> message(_request->Body<Core::JSONRPC::Message>());

                        identifier = Core::Format(_T("{ \"type\": \"JSONRPC\", \"id\": %d, \"method\": \"%s\", \"parameters\": %s }"), message->Id.Value(), message->Designator.Value(), message->Parameters.Value());
                    }
                    return (identifier);
                }

            private:
                Core::ProxyType<Web::Request> _request;
                string _token;
                bool _jsonrpc;

                static Core::ProxyType<Web::Response> _missingResponse;
            };
            class JSONElementJob : public Job {
            public:
                JSONElementJob(const JSONElementJob&) = delete;
                JSONElementJob& operator=(const JSONElementJob&) = delete;

                JSONElementJob()
                    : Job()
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
                void Set(const uint32_t id, Server* server, Core::ProxyType<Service>& service, Core::ProxyType<Core::JSON::IElement>& element, const string& token, const bool JSONRPC)
                {
                    Job::Set(id, server, service);

                    ASSERT(_element.IsValid() == false);

                    _element = element;
                    _jsonrpc = JSONRPC;
                    _token = token;
                }
                void Dispatch() override
                {
                    ASSERT(Job::HasService() == true);
                    ASSERT(_element.IsValid() == true);

                    if (_jsonrpc == true) {
#if THUNDER_PERFORMANCE
                        Core::ProxyType<TrackingJSONRPC> tracking (_element);
                        ASSERT (tracking.IsValid() == true);
                                    tracking->Dispatch();
#endif
                        Core::ProxyType<Core::JSONRPC::Message> message(_element);
                        ASSERT(message.IsValid() == true);
                        if (HasService() == true) {
                            message->ImplicitCallsign(GetService().Callsign());
                        }

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
                string Identifier() const override {
                    return(_T("PluginServer::Channel::JSONElementJob::") + Callsign());
                }

            private:
                Core::ProxyType<Core::JSON::IElement> _element;
                string _token;
                bool _jsonrpc;
            };
            class TextJob : public Job {
            public:
                TextJob(const TextJob&) = delete;
                TextJob& operator=(const TextJob&) = delete;

                TextJob()
                    : Job()
                    , _text()
                {
                }
                ~TextJob() override
                {
                }

            public:
                void Set(const uint32_t id, Server* server, Core::ProxyType<Service>& service, const string& text)
                {
                    Job::Set(id, server, service);
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
                string Identifier() const override {
                    return(_T("PluginServer::Channel::TextJob::") + Callsign());
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
            void LinkBody(Core::ProxyType<Request>& request) override
            {
                // This is the time where we determine what body is needed for the incoming request.
                TRACE(WebFlow, (request));

                // Remember the path and options..
                Core::ProxyType<Service> service;
                bool serviceCall;

                uint32_t status = _parent.Services().FromLocator(request->Path, service, serviceCall);

                request->Service(status, Core::ProxyType<PluginHost::Service>(service), serviceCall);

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
            void Received(Core::ProxyType<Request>& request) override
            {
                ISecurity* security = nullptr;

                TRACE(WebFlow, (request));

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

                        request->Service(status, Core::ProxyType<PluginHost::Service>(service), serviceCall);
                    } else if ((request->State() == Request::COMPLETE) && (request->HasBody() == true)) {
                        Core::ProxyType<Core::JSONRPC::Message> message(request->Body<Core::JSONRPC::Message>());
                        if (message.IsValid() == true) {
                            ASSERT(request->Service().IsValid() == true);
                            message->ImplicitCallsign(request->Service()->Callsign());
                            if (security->Allowed(*message) == false) {
                                request->Unauthorized();
                        }
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
                    Core::ProxyType<Service> service(request->Service());

                    ASSERT(service.IsValid());

                    Core::ProxyType<Web::Response> response;
                    
                    if (request->ServiceCall() == true) {
                        response = service->Evaluate(*request);
                    }

                    if (response.IsValid() == true) {
                        // Report that the calls sign could not be found !!
                        Submit(response);
                    } else {
                        // Send the Request object out to be handled.
                        // By definition, we can issue it on a rental thread..
                        Core::ProxyType<WebRequestJob> job(_webJobs.Element());

                        ASSERT(job.IsValid() == true);

                        if (job.IsValid() == true) {
                            Core::ProxyType<Web::Request> baseRequest(request);
                            job->Set(Id(), &_parent, service, baseRequest, _security->Token(), !request->ServiceCall());
                            _parent.Submit(Core::ProxyType<Core::IDispatch>(job));
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
            void Send(const Core::ProxyType<Web::Response>& response) override
            {
                if (_requestClose == true) {
                    PluginHost::Channel::Close(0);
                }
                TRACE(WebFlow, (response));
            }

            // Handle the JSON structs flowing over the WebSocket.
            // [INBOUND]  Completed deserialized JSON objects that are Received, will trigger the Received.
            // [OUTBOUND] Completed serialized JSON objects that are send out, will trigger the Send.
            Core::ProxyType<Core::JSON::IElement> Element(const string& identifier) override
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
            void Send(const Core::ProxyType<Core::JSON::IElement>& element) override
            {
                TRACE(SocketFlow, (element));
            }
            void Received(Core::ProxyType<Core::JSON::IElement>& element) override
            {
                bool securityClearance = ((State() & Channel::JSONRPC) == 0);

                ASSERT(_service.IsValid() == true);

                TRACE(SocketFlow, (element));

                if (securityClearance == false) {
                    Core::ProxyType<Core::JSONRPC::Message> message(element);
                    if (message.IsValid()) {

                        message->ImplicitCallsign(_service->Callsign());

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
                    Core::ProxyType<JSONElementJob> job(_jsonJobs.Element());

                    ASSERT(job.IsValid() == true);

                    if ((_service.IsValid() == true) && (job.IsValid() == true)) {
                        job->Set(Id(), &_parent, _service, element, _security->Token(), ((State() & Channel::JSONRPC) != 0));
                        _parent.Submit(Core::ProxyType<Core::IDispatch>(job));
                    }
                }
            }
            void Received(const string& value) override
            {
                ASSERT(_service.IsValid() == true);

                TRACE(TextFlow, (value));

                // Send the JSON object out to be handled.
                // By definition, we can issue it on a rental thread..
                Core::ProxyType<TextJob> job(_textJobs.Element());

                ASSERT(job.IsValid() == true);

                if ((_service.IsValid() == true) && (job.IsValid() == true)) {
                    job->Set(Id(), &_parent, _service, value);
                    _parent.Submit(Core::ProxyType<Core::IDispatch>(job));
                }
            }

            // We are in an upgraded mode, we are a websocket. Time to "deserialize and serialize
            // INBOUND and OUTBOUND information.
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                uint16_t result = 0;

                if (State() == RAW) {
                    result = _service->Outbound(Id(), dataFrame, maxSendSize);
                } else {
                    result = PluginHost::Channel::Serialize(dataFrame, maxSendSize);
                }

                return (result);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                uint16_t result = receivedSize;

                if (State() == RAW) {
                    result = _service->Inbound(Id(), dataFrame, receivedSize);
                } else {
                    result = PluginHost::Channel::Deserialize(dataFrame, receivedSize);
                }

                return (result);
            }

            // Whenever there is  a state change on the link, it is reported here.
            void StateChange()
            {
                TRACE(Activity, (_T("State change on [%d] to [%s]"), Id(), (IsSuspended() ? _T("SUSPENDED") : (IsUpgrading() ? _T("UPGRADING") : (IsWebSocket() ? _T("WEBSOCKET") : _T("WEBSERVER"))))));

                // If we are closing (or closed) do the clean up
                if (IsOpen() == false) {
                    if (_service.IsValid() == true) {
                        _service->Unsubscribe(*this);

                        _service.Release();
                    }

                    State(CLOSED, false);
                    _parent.Closed(Id());
                    _parent.Dispatcher().TriggerCleanup();

                } else if (IsUpgrading() == true) {

                    ASSERT(_service.IsValid() == false);
                    bool serviceCall;
                    // see if we need to subscribe...
                    _parent.Services().FromLocator(Path(), _service, serviceCall);

                    if (_service.IsValid() == false) {
                        AbortUpgrade(Web::STATUS_SERVICE_UNAVAILABLE, _T("Could not find a correct service for this socket."));
                    } else if (Allowed(Path(), Query()) == false) {
                        AbortUpgrade(Web::STATUS_FORBIDDEN, _T("Security prohibites this connection."));
                    } else {
                        //select supported protocol and let know which one was choosen
                        auto protocol = SelectSupportedProtocol(Protocols());
                        if (protocol.empty() == false) {
                            // if protocol header is not set sending an empty protocol header is not allowed (at least by chrome)
                            Protocols(Web::ProtocolsArray(protocol));
                        }

                        if (serviceCall == false) {
                            const string& JSONRPCHeader(_parent._config.JSONRPCPrefix());
                            if (Name().length() > (JSONRPCHeader.length() + 1)) {
                                Properties(static_cast<uint32_t>(JSONRPCHeader.length()) + 1);
                            }
                            State(JSONRPC, false);
                        } else {
                            const string& serviceHeader(_parent._config.WebPrefix());
                            if (Name().length() > (serviceHeader.length() + 1)) {
                                Properties(static_cast<uint32_t>(serviceHeader.length()) + 1);
                            }
                        }
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
            inline string SelectSupportedProtocol(const Web::ProtocolsArray& protocols)
            {
                for (const auto& protocol : protocols) {
                    if (protocol == _T("notification")) {
                        State(TEXT, true);
                        return protocol;
                    } else if (protocol == _T("json")) {
                        State(JSON, false);
                        return protocol;
                    } else if (protocol == _T("text")) {
                        State(TEXT, false);
                        return protocol;
                    } else if (protocol == _T("jsonrpc")) {
                        State(JSONRPC, false);
                        return protocol;
                    } else if (protocol == _T("raw")) {
                        State(RAW, false);
                        return protocol;
                    }
                }

                State(RAW, false);
                return _T("");
            }

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
            using BaseClass = Core::SocketServerType<Channel>;

        public:
            ChannelMap() = delete;
            ChannelMap(const ChannelMap&) = delete;
            ChannelMap& operator=(const ChannelMap&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST);
            ChannelMap(Server& parent, const Core::NodeId& listeningNode, const uint16_t connectionCheckTimer)
                : Core::SocketServerType<Channel>(listeningNode)
                , _parent(parent)
                , _connectionCheckTimer(connectionCheckTimer * 1000)
                , _job(*this)
            {
                if (connectionCheckTimer != 0) {
                    Core::Time NextTick = Core::Time::Now();

                    NextTick.Add(_connectionCheckTimer);

                    Core::ProxyType<Core::IDispatch> job(_job.Submit());
                    if (job.IsValid() == true) {
                        _parent.Schedule(NextTick.Ticks(), job);
                    }
                }
            }
            POP_WARNING();
            ~ChannelMap()
            {
                Core::ProxyType<Core::IDispatch> job(_job.Revoke());
                if (job.IsValid() == true) {
                    _parent.Revoke(job);
                    _job.Revoked();
                }

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
            void TriggerCleanup()
            {
                if (_connectionCheckTimer == 0) {
                    Core::ProxyType<Core::IDispatch> job(_job.Reschedule(Core::Time::Now()));
                    if (job.IsValid() == true) {
                        _parent.Submit(job);
                    }
                }
            }
            void GetMetaData(Core::JSON::ArrayType<MetaData::Channel>& metaData) const;

        private:
            friend class Core::ThreadPool::JobType<ChannelMap&>;

            string JobIdentifier() const {
                return (_T("PluginServer::ChannelMap::Cleanup"));
            }
            void Dispatch()
            {
                TRACE(Activity, (string(_T("Cleanup job running..\n"))));

                // First clear all shit from last time..
                Cleanup();

                if (_connectionCheckTimer != 0) {
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

                    Core::Time NextTick(Core::Time::Now());
                    NextTick.Add(_connectionCheckTimer);

                    _job.Reschedule(NextTick);
                }
            }

        private:
            Server& _parent;
            const uint32_t _connectionCheckTimer;
            Core::ThreadPool::JobType<ChannelMap&> _job;
        };

    public:
        Server() = delete;
        Server(Server&&) = delete;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        Server(Config& configuration, const bool background);
        virtual ~Server();

    public:
        static void PostMortem(Service& service, const IShell::reason why, RPC::IRemoteConnection* connection);

        inline void Statistics(uint32_t& requests, uint32_t& responses, uint32_t& fileBodies, uint32_t& jsonrpc) const {
            _factoriesImplementation.Statistics(requests, responses, fileBodies, jsonrpc);
        }
        inline ChannelMap& Dispatcher()
        {
            return (_connections);
        }
        inline void DumpMetadata() {
            MetaData::Server data;
            _dispatcher.Snapshot(data);

            // Drop the workerpool info (wht is currently running and what is pending) to a file..
            Core::File dumpFile(_config.PostMortemPath() + "ThunderInternals.json");
            if (dumpFile.Create(false) == true) {
                data.IElement::ToFile(dumpFile);
            }
        }
        inline ServiceMap& Services()
        {
            return (_services);
        }
        inline void Metadata(PluginHost::MetaData::Version& data)
        {
            data.Major = PluginHost::Major;
            data.Minor = PluginHost::Minor;
            data.Patch = PluginHost::Patch;
            data.Hash = string(Core::System::ModuleBuildRef());
        }
        inline Server::WorkerPoolImplementation& WorkerPool()
        {
            return (_dispatcher);
        }
        inline void Submit(const Core::ProxyType<Core::IDispatch>& job)
        {
            _dispatcher.Submit(job);
        }
        inline void Schedule(const uint64_t time, const Core::ProxyType<Core::IDispatch>& job)
        {
            _dispatcher.Schedule(time, job);
        }
        inline void Revoke(const Core::ProxyType<Core::IDispatch> job)
        {
            _dispatcher.Revoke(job);
        }
        inline PluginHost::Config& Configuration()
        {
            return (_config);
        }  
        inline const PluginHost::Config& Configuration() const
        {
            return (_config);
        }

        void Notification(const ForwardMessage& message);
        void Open();
        void Close();

        uint32_t Persist()
        {
            Override infoBlob( _config, _services, Configuration().PersistentPath() + PluginOverrideFile);

            return (infoBlob.Save());
        }
        uint32_t Load()
        {
            Override infoBlob(_config, _services, Configuration().PersistentPath() + PluginOverrideFile);

            return (infoBlob.Load());
        }

    private:
        Core::ProxyType<Service> Controller()
        {
            return (_controller);
        }
        ISecurity* Officer(const string& token)
        {
            return (_services.Officer(token));
        }
        ISecurity* Officer()
        {
            return (_config.Security());
        }
        void Closed(const uint32_t id) {
            _services.Closed(id);
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
