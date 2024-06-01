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
#include "PostMortem.h"

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
        typedef const IService::IMetadata* (*ModuleServiceMetadataImpl)();
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

        using Shells = std::unordered_map<string, PluginHost::IShell*>;

    private:
        class ServiceMap;
        friend class Plugin::Controller;

        class WorkerPoolImplementation : public Core::WorkerPool, public Core::ThreadPool::ICallback {
        private:
            class Dispatcher : public Core::ThreadPool::IDispatcher {
            public:
                Dispatcher(Dispatcher&&) = delete;
                Dispatcher(const Dispatcher&) = delete;
                Dispatcher& operator=(Dispatcher&&) = delete;
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
            WorkerPoolImplementation(WorkerPoolImplementation&&) = delete;
            WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
            WorkerPoolImplementation& operator=(WorkerPoolImplementation&&) = delete;
            WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

            WorkerPoolImplementation(const uint32_t stackSize)
                : Core::WorkerPool(THREADPOOL_COUNT, stackSize, 8 * THREADPOOL_COUNT, &_dispatch, this)
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
            void Snapshot(PluginHost::Metadata::Server& data) const
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
            FactoriesImplementation(FactoriesImplementation&&) = delete;
            FactoriesImplementation(const FactoriesImplementation&) = delete;
            FactoriesImplementation& operator=(FactoriesImplementation&&) = delete;
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
            Core::ProxyType<Web::JSONRPC::Body> JSONRPC() override
            {
                return (Core::ProxyType< Web::JSONRPC::Body> (_jsonRPCFactory.Element()));
            }

        private:
            Core::ProxyPoolType<Web::Request> _requestFactory;
            Core::ProxyPoolType<Web::Response> _responseFactory;
            Core::ProxyPoolType<Web::FileBody> _fileBodyFactory;
            Core::ProxyPoolType<PluginHost::JSONRPCMessage> _jsonRPCFactory;
        };

        // Trace class for internal information of the PluginHost
        class Activity {
        public:
            Activity(Activity&&) = delete;
            Activity(const Activity&) = delete;
            Activity& operator=(Activity&&) = delete;
            Activity& operator=(const Activity& a_RHS) = delete;

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
            ~Activity() = default;

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
        public:
            WebFlow(WebFlow&&) = delete;
            WebFlow(const WebFlow&) = delete;
            WebFlow& operator=(WebFlow&&) = delete;
            WebFlow& operator=(const WebFlow&) = delete;

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
            ~WebFlow() = default;

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
        public:
            SocketFlow(SocketFlow&&) = delete;
            SocketFlow(const SocketFlow&) = delete;
            SocketFlow& operator=(SocketFlow&&) = delete;
            SocketFlow& operator=(const SocketFlow&) = delete;

            SocketFlow(const Core::ProxyType<Core::JSON::IElement>& object)
            {
                if (object.IsValid() == true) {
                    string text;

                    object->ToString(text);

                    _text = Core::ToString(text);
                }
            }
            ~SocketFlow() = default;

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
        public:
            TextFlow(TextFlow&&) = delete;
            TextFlow(const TextFlow&) = delete;
            TextFlow& operator=(TextFlow&&) = delete;
            TextFlow& operator=(const TextFlow&) = delete;

            TextFlow(const string& text)
                : _text(Core::ToString(text))
            {
            }
            ~TextFlow() = default;

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

        class Service : public IShell::ICOMLink, public IShell::IConnectionServer, public PluginHost::Service {
        public:
            enum mode {
                CONFIGURED,
                CLONED,
                DYNAMIC
            };

        private:
            class Composit : public PluginHost::ICompositPlugin::ICallback {
            public:
                Composit() = delete;
                Composit(Composit&&) = delete;
                Composit(const Composit&&) = delete;
                Composit& operator=(Composit&&) = delete;
                Composit& operator=(const Composit&&) = delete;

                Composit(Service& parent)
                    : _parent(parent)
                    , _adminLock()
                    , _composit(nullptr)
                    , _plugins() {
                }
                ~Composit() override {
                    ASSERT(_plugins.empty() == true);
                    ASSERT(_composit == nullptr);
                }

            public:
                void AquireInterfaces(IPlugin* plugin) {
                    _composit = plugin->QueryInterface<ICompositPlugin>();
                    if (_composit != nullptr) {
                        // Seems this is a plugin that can be a composition of more plugins..
                        _composit->Callback(this);
                    }
                }
                void ReleaseInterfaces() {
                    if (_composit != nullptr) {
                        _composit->Callback(nullptr);
                        _composit->Release();
                        _composit = nullptr;
                    }
                }
                bool IsEmpty() const {
                    return (_plugins.empty());
                }
                template<typename ACTION>
                void Visit(ACTION&& action) {
                    const string callsign = _parent.Callsign();
                    _adminLock.Lock();
                    for (std::pair<const string, PluginHost::IShell*>& entry : _plugins) {
                        action(callsign + PluginHost::ICompositPlugin::Delimiter + entry.first, entry.second);
                    }
                    _adminLock.Unlock();
                }
                template<typename ACTION>
                void Visit(ACTION&& action) const {
                    const string callsign = _parent.Callsign();
                    _adminLock.Lock();
                    for (const std::pair<const string, PluginHost::IShell*>& entry : _plugins) {
                        action(callsign + PluginHost::ICompositPlugin::Delimiter + entry.first, entry.second);
                    }
                    _adminLock.Unlock();
                }
                Core::ProxyType<PluginHost::IShell> Source(const string& name) {
                    Core::ProxyType<PluginHost::IShell> result;

                    _adminLock.Lock();
                    Shells::iterator index(_plugins.find(name));
                    if (index != _plugins.end()) {
                        result = Core::ProxyType<PluginHost::IShell>(static_cast<Core::IReferenceCounted&>(*(index->second)), *(index->second));
                    }
                    _adminLock.Unlock();

                    return (result);
                }
                void Created(const string& callsign, IShell* plugin) override {
                    const string link = _parent.Callsign();

                    _adminLock.Lock();

                    Shells::iterator index(_plugins.find(callsign));

                    ASSERT(index == _plugins.end());

                    bool report = (index == _plugins.end());

                    if (report == true) {
                        _plugins.emplace(std::piecewise_construct,
                            std::make_tuple(callsign),
                            std::make_tuple(plugin));
                        TRACE(Activity, (_T("LinkPlugin [%s], added composit plugin [%s]"), link.c_str(), callsign.c_str()));
                    }
                    _adminLock.Unlock();

                    if (report == true) {
                        _parent._administrator.Created(link + PluginHost::ICompositPlugin::Delimiter + callsign, plugin);
                    }
                }
                void Destroy(const string& callsign, IShell* plugin) override {
                    const string link = _parent.Callsign();

                    _adminLock.Lock();

                    Shells::iterator index(_plugins.find(callsign));

                    ASSERT(index != _plugins.end());

                    bool report = (index != _plugins.end());

                    if (report == true) {
                        _plugins.erase(index);
                        TRACE(Activity, (_T("LinkPlugin [%s], removed composit plugin [%s]"), link.c_str(), callsign.c_str()));
                    }
                    _adminLock.Unlock();

                    if (report == true) {
                        _parent._administrator.Destroy(link + PluginHost::ICompositPlugin::Delimiter + callsign, plugin);
                    }
                }
                void Activated(const string& callsign, PluginHost::IShell* plugin) override {
                    const string link = _parent.Callsign();
                    ASSERT(_plugins.find(callsign) != _plugins.end());
                    ASSERT(plugin != nullptr);
                    _parent._administrator.Activated(link + PluginHost::ICompositPlugin::Delimiter + callsign, plugin);
                }
                void Deactivated(const string& callsign, PluginHost::IShell* plugin) override {
                    const string link = _parent.Callsign();
                    ASSERT(_plugins.find(callsign) != _plugins.end());
                    ASSERT(plugin != nullptr);
                    _parent._administrator.Deactivated(link + PluginHost::ICompositPlugin::Delimiter + callsign, plugin);
                }
                void Unavailable(const string& callsign, PluginHost::IShell* plugin) override {
                    const string link = _parent.Callsign();
                    ASSERT(_plugins.find(callsign) != _plugins.end());
                    ASSERT(plugin != nullptr);
                    _parent._administrator.Unavailable(link + PluginHost::ICompositPlugin::Delimiter + callsign, plugin);
                }

                BEGIN_INTERFACE_MAP(Composit)
                    INTERFACE_ENTRY(PluginHost::ICompositPlugin::ICallback)
                END_INTERFACE_MAP

            private:
                Service& _parent;
                mutable Core::CriticalSection _adminLock;
                ICompositPlugin* _composit;
                Shells _plugins;
            };
            class ExternalAccess : public RPC::Communicator {
            public:
                ExternalAccess() = delete;
                ExternalAccess(ExternalAccess&&) = delete;
                ExternalAccess(const ExternalAccess&) = delete;
                ExternalAccess& operator=(const ExternalAccess&) = delete;

                ExternalAccess(
                    const Core::NodeId& source,
                    const string& proxyStubPath,
                    const Core::ProxyType<RPC::InvokeServer>& handler)
                    : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                    , _plugin(nullptr) {
                }
                ~ExternalAccess() override = default;

            public:
                void SetInterface(Core::IUnknown* plugin) {
                    ASSERT((_plugin == nullptr) ^ (plugin == nullptr));
                    _plugin = plugin;
                }

            private:
                void* Acquire(const string& /* className */, const uint32_t interfaceId, const uint32_t /* versionId */) override {
                    ASSERT(interfaceId >= RPC::IDS::ID_EXTERNAL_INTERFACE_OFFSET);
                    return (interfaceId >= RPC::IDS::ID_EXTERNAL_INTERFACE_OFFSET ? _plugin->QueryInterface(interfaceId) : nullptr);
                }

            private:
                Core::IUnknown* _plugin;
            };
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

                        ASSERT_VERBOSE(((_state & 0x1) == state::STATE_OR), "Must not use NOT_ in preconditions (subsystem 0x%08x)", input);

                        // Make sure the event is only set once (POSITIVE or NEGATIVE)
                        if ((((_mask & (1 << bitNr)) != 0) && ((_events & (1 << bitNr)) != 0)) || ((_state & 0x1) != state::STATE_OR)) {
                            _state = STATE_ERROR;
                        }
                    }
                    else {
                        ASSERT_VERBOSE(((_state & 0x1) == state::STATE_AND), "Must only use NOT_ in terminations (subsystem 0x%08x)", input);

                        // Make sure the event is only set once (POSITIVE or NEGATIVE)
                        if ((((_mask & (1 << bitNr)) != 0) && ((_events & (1 << bitNr)) == 0)) || ((_state & 0x1) != state::STATE_AND)) {
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

                ControlData& operator=(const Core::IService::IMetadata* info) {
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
            static Core::NodeId PluginNodeId(const PluginHost::Config& config, const Plugin::Config& plugin) {
                Core::NodeId result;
                if (plugin.Communicator.IsSet() == true) {
                    if (plugin.Communicator.IsNull() == true) {
                        // It should be derived from the Thunder communicator port..
                        Core::NodeId masterNode = config.Communicator();

                        if (masterNode.Type() != Core::NodeId::enumType::TYPE_DOMAIN) {
                            // It's not a path name, just take the same address but with a different port
                            static uint32_t nextPortId = 0;

                            result = Core::NodeId(masterNode);
                            result.PortNumber(masterNode.PortNumber() + static_cast<uint8_t>(Core::InterlockedIncrement(nextPortId) & 0xFF));
                        }
                        else {
                            // Its a domain socket, move the callsign in between and make that path part..
                            string pathName = Core::Directory::Normalize(Core::File::PathName(masterNode.HostName())) + plugin.Callsign.Value();
                            string fullName = pathName + '/' + Core::File::FileName(masterNode.HostName());

                            Core::Directory base(pathName.c_str());
                            if ((base.Exists() == true) || (base.Create() == true)) {
                                result = Core::NodeId(fullName.c_str(), Core::NodeId::enumType::TYPE_DOMAIN);
                            }
                        }
                    }
                    else {
                        result = Core::NodeId(plugin.Communicator.Value().c_str());
                    }
                }
                return (result);
            }

        public:
            Service() = delete;
            Service(Service&&) = delete;
            Service(const Service&) = delete;
            Service& operator=(Service&&) = delete;
            Service& operator=(const Service&) = delete;

            Service(const PluginHost::Config& server, const Plugin::Config& plugin, ServiceMap& administrator, const mode type, const Core::ProxyType<RPC::InvokeServer>& handler)
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
                , _reason(IShell::reason::SHUTDOWN)
                , _precondition(true, plugin.Precondition)
                , _termination(false, plugin.Termination)
                , _activity(0)
                , _connection(nullptr)
                , _lastId(0)
                , _metadata()
                , _library()
                , _external(PluginNodeId(server, plugin), server.ProxyStubPath(), handler)
                , _administrator(administrator)
                , _composit(*this)
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
                    string output;
                    uint32_t result = _jsonrpc->Invoke(channelId, message.Id.Value(), token, message.Designator.Value(), message.Parameters.Value(), output);

                    if ( (result != static_cast<uint32_t>(~0)) && ( (message.Id.IsSet()) || (result != Core::ERROR_NONE) ) )  {

                        response = IFactories::Instance().JSONRPC();
                        
                        if (message.Id.IsSet()) {
                            response->Id = message.Id.Value();
                        }

                        if (result == Core::ERROR_NONE) {
                            if (output.empty() == true) {
                                response->Result.Null(true);;
                            }
                            else {
                                response->Result = output;
                            }
                        }
                        else {
                            response->Error.SetError(result);
                            if (output.empty() == false) {
                                response->Error.Text = output;
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
            inline void GetMetadata(Metadata::Service& metaData) const
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

                PluginHost::Service::GetMetadata(metaData);
            }
            inline void Evaluate()
            {
                Lock();

                uint32_t subsystems = _administrator.SubSystemInfo().Value();

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
            bool PostMortemAllowed(PluginHost::IShell::reason why) const {
                return (_administrator.Configuration().PostMortemAllowed(why));
            }
 
            uint32_t Submit(const uint32_t id, const Core::ProxyType<Core::JSON::IElement>& response) override;
            ISubSystem* SubSystems() override;
            void Notify(const string& message) override;
            void Notify(const string& event, const string& parameters) override;
            void* QueryInterface(const uint32_t id) override;
            void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override;
            template <typename REQUESTEDINTERFACE>
            REQUESTEDINTERFACE* QueryInterface()
            {
                void* baseInterface(Service::QueryInterface(REQUESTEDINTERFACE::ID));

                if (baseInterface != nullptr) {
                    return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
                }

                return (nullptr);
            }
            template <typename REQUESTEDINTERFACE>
            const REQUESTEDINTERFACE* QueryInterface() const
            {
                const void* baseInterface(const_cast<Service*>(this)->QueryInterface(REQUESTEDINTERFACE::ID));

                if (baseInterface != nullptr) {
                    return (reinterpret_cast<const REQUESTEDINTERFACE*>(baseInterface));
                }

                return (nullptr);
            }
 
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
           Core::hresult Metadata(string& info /* @out */) const override {
                Metadata::Service result;
                GetMetadata(result);
                result.ToString(info);
                return (Core::ERROR_NONE);
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
            void Register(IShell::IConnectionServer::INotification* sink) override
            {
                _administrator.Register(sink);
            }
            void Unregister(const IShell::IConnectionServer::INotification* sink) override
            {
                _administrator.Unregister(sink);
            }
            void Register(IShell::ICOMLink::INotification* sink) override
            {
                _administrator.Register(sink);
            }
            void Unregister(const IShell::ICOMLink::INotification* sink) override
            {
                _administrator.Unregister(sink);
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId) override
            {
                return (_administrator.RemoteConnection(connectionId));
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
            
            Composit& Composits() {
                return (_composit);
            }

            const Composit& Composits() const {
                return (_composit);
            }

        private:
            uint32_t Wakeup(const uint32_t timeout);

            #ifdef HIBERNATE_SUPPORT_ENABLED
            uint32_t HibernateChildren(const Core::process_t parentPID, const uint32_t timeout);
            uint32_t WakeupChildren(const Core::process_t parentPID, const uint32_t timeout);
            #endif

            std::vector<string> GetLibrarySearchPaths(const string& locator) const override
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

                        // Loading a library, in the static initializers, might register Service::Metadata structures. As
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
                    if (newIF == nullptr) {
                        ErrorMessage(_T("local class definitions/version does not exist"));
                    }
                } else {
                    _library = LoadLibrary(locator);
                    if (_library.IsLoaded() == false) {
                        ErrorMessage(_T("Library could not be loaded"));
                    }
                    else {
                        if ((newIF = Core::ServiceAdministrator::Instance().Instantiate<IPlugin>(_library, className, version)) == nullptr) {
                            ErrorMessage(_T("class definitions/version does not exist"));
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
                    _composit.AquireInterfaces(newIF);
                    if (_webSecurity == nullptr) {
                        _webSecurity = _administrator.Configuration().Security();
                        _webSecurity->AddRef();
                    }

                    _pluginHandling.Lock();
                    _handler = newIF;

                    if (_metadata.IsValid() == false) {
                        _metadata = dynamic_cast<Core::IService::IMetadata*>(newIF);
                    }

                    uint32_t events = _administrator.SubSystemInfo().Value();

                    _precondition.Evaluate(events);
                    _termination.Evaluate(events);

                    _external.SetInterface(newIF);

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
                _composit.ReleaseInterfaces();
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
                    _external.SetInterface(nullptr);
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
            reason _reason;
            Condition _precondition;
            Condition _termination;
            uint32_t _activity;
            RPC::IRemoteConnection* _connection;
            uint32_t _lastId;
            ControlData _metadata;
            Core::Library _library;
            void* _hibernateStorage;
            ExternalAccess _external;
            ServiceMap& _administrator;
            Core::SinkType<Composit> _composit;

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
                    , StartMode()
                    , Resumed()
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startmode"), &StartMode);
                    Add(_T("resumed"), &Resumed);
                }
                Plugin(const string& config, const string& systemRootPath, const PluginHost::IShell::startmode value, const bool resumed)
                    : Core::JSON::Container()
                    , Configuration(config, false)
                    , SystemRootPath(systemRootPath)
                    , StartMode(value)
                    , Resumed(resumed)
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startmode"), &StartMode);
                    Add(_T("resumed"), &Resumed);
                }
                Plugin(Plugin const& copy)
                    : Core::JSON::Container()
                    , Configuration(copy.Configuration)
                    , SystemRootPath(copy.SystemRootPath)
                    , StartMode(copy.StartMode)
                    , Resumed(copy.Resumed)
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startmode"), &StartMode);
                    Add(_T("resumed"), &Resumed);
                }
                Plugin(Plugin&& move)
                    : Core::JSON::Container()
                    , Configuration(std::move(move.Configuration))
                    , SystemRootPath(std::move(move.SystemRootPath))
                    , StartMode(std::move(move.StartMode))
                    , Resumed(std::move(move.Resumed))
                {
                    Add(_T("configuration"), &Configuration);
                    Add(_T("systemrootpath"), &SystemRootPath);
                    Add(_T("startmode"), &StartMode);
                    Add(_T("resumed"), &Resumed);
                }

                ~Plugin() override = default;

            public:
                Core::JSON::String Configuration;
                Core::JSON::String SystemRootPath;
                Core::JSON::EnumType<PluginHost::IShell::startmode> StartMode;
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
                        std::forward_as_tuple(_T("{}"), "", PluginHost::IShell::startmode::UNAVAILABLE, false)).first);

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
                                index->ConfigLine(current->second.Configuration.Value());
                            }
                            if (current->second.SystemRootPath.IsSet() == true) {
                                index->SystemRootPath(current->second.SystemRootPath.Value());
                            }
                            if (current->second.StartMode.IsSet() == true) {
                                index->StartMode(current->second.StartMode.Value());
                            }
                            if (current->second.Resumed.IsSet() == true) {
                                index->Resumed(current->second.Resumed.Value());
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

                        string config(index->ConfigLine());

                        if (config.empty() == true) {
                            current->second.Configuration = _T("{}");
                        } else {
                            current->second.Configuration = config;
                        }
                        current->second.SystemRootPath = index->SystemRootPath();
                        current->second.StartMode = index->StartMode();
                        current->second.Resumed = index->Resumed();
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
            using Plugins = std::unordered_map<string, Core::ProxyType<Service>>;
            using Notifiers = std::vector<PluginHost::IPlugin::INotification*>;
            using RemoteInstantiators = std::unordered_map<string, IRemoteInstantiation*>;
            using ShellNotifiers = std::vector< Exchange::Controller::IShells::INotification*>;
            using ChannelObservers = std::vector<IShell::IConnectionServer::INotification*>;

            class Iterator {
            public:
                Iterator()
                    : _container()
                    , _index()
                    , _position(0) {
                }
                Iterator(Shells&& services)
                    : _container(services)
                    , _index()
                    , _position(0) {
                }
                Iterator(Iterator&& move)
                    : _container(std::move(move._container))
                    , _index()
                    , _position(0) {
                }
                Iterator(const Iterator& copy)
                    : _container(copy._container)
                    , _index()
                    , _position(0) {
                }
                ~Iterator() {
                    for (const std::pair<const string, PluginHost::IShell*>& entry :  _container) {
                        entry.second->Release();
                    }
                }

                Iterator& operator=(Iterator&& move) {
                    if (this != &move) {
                        _container = std::move(move._container);
                        _index = std::move(move._index);
                        _position = move._position;
                    }
                    return (*this);
                }
                Iterator& operator=(const Iterator& copy) {
                    _container = copy._container;
                    _position = copy._position;
                    if (_position > 0) {
                        _index = _container.begin();
                        uint32_t steps = _position - 1;
                        while ((steps != 0) && (_index != _container.end())) {
                            _index++;
                            steps--;
                        }
                    }
                    return (*this);
                }

            public:
                bool IsValid() const {
                    return ((_position > 0) && (_index != _container.end()));
                }
                void Reset() {
                    _position = 0;
                }
                bool Next() {
                    if (_position == 0) {
                        _position = 1;
                        _index = _container.begin();
                    }
                    else if (_index != _container.end()) {
                        _position++;
                        _index++;
                    }
                    return (_index != _container.end());
                }
                Core::ProxyType<PluginHost::IShell> Current() {
                    ASSERT(IsValid());
                    return (Core::ProxyType<PluginHost::IShell>(static_cast<Core::IReferenceCounted&>(*_index->second), *_index->second));
                }
                uint32_t Count() const {
                    return (static_cast<uint32_t>(_container.size()));
                }
                Core::ProxyType<PluginHost::IShell> operator->()
                {
                    ASSERT(IsValid());

                    return (Core::ProxyType<PluginHost::IShell>(static_cast<Core::IReferenceCounted&>(*_index->second), *_index->second));
                }

                Core::ProxyType<const PluginHost::IShell> operator->() const
                {
                    ASSERT(IsValid());

                    return (Core::ProxyType<const PluginHost::IShell>(static_cast<Core::IReferenceCounted&>(*_index->second), *_index->second));
                }

            private:
                 Shells _container;
                 Shells::iterator _index;
                 uint32_t _position;
            }; 

        private:
           class CommunicatorServer : public RPC::Communicator {
            private:
                using Observers = std::vector<IShell::ICOMLink::INotification*>;
                using Proxy = std::pair<uint32_t, const Core::IUnknown*>;
                using Proxies = std::vector<Proxy>;

                class RemoteHost : public RPC::Communicator::RemoteConnection {
                private:
                    friend Core::ServiceType<RemoteHost>;

                public:
                    RemoteHost(RemoteHost&&) = delete;
                    RemoteHost(const RemoteHost&) = delete;
                    RemoteHost& operator=(RemoteHost&&) = delete;
                    RemoteHost& operator=(const RemoteHost&) = delete;

                    RemoteHost(const RPC::Object& instance, const RPC::Config& config)
                        : RemoteConnection()
                        , _object(instance)
                        , _config(config)
                    {
                    }
                    ~RemoteHost() override = default;

                public:
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
                    ProxyStubObserver& operator=(ProxyStubObserver&&) = delete;
                    ProxyStubObserver& operator=(const ProxyStubObserver&) = delete;

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
                CommunicatorServer(CommunicatorServer&&) = delete;
                CommunicatorServer(const CommunicatorServer&) = delete;
                CommunicatorServer& operator=(CommunicatorServer&&) = delete;
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
                    const bool delegatedReleases,
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
                    , _deadProxies()
                    , _job(*this)
                {
                    // Shall we enable the non-happy day functionality to cleanup Release on behalf of unexpected
                    // channel closes. Only for testing Buggy plugins, turn it off (false)!
                    // STRONG RECOMMENDATION TO HAVE THIS ACTIVE (TRUE)!!!
                    RPC::Administrator::Instance().DelegatedReleases(delegatedReleases);

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
                void Unregister(const IShell::ICOMLink::INotification* sink)
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
                void Dispatch() {
                    // Oke time to notify the destruction of some proxies...
                    _adminLock.Lock();

                    for (const Proxy& entry : _deadProxies) {
                        for (IShell::ICOMLink::INotification* observer : _requestObservers) {
                            observer->Dangling(entry.second, entry.first);
                            if (entry.second->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {
                                TRACE(Trace::Warning, (_T("Potentially a Proxy leak on interface %d"), entry.first));
                            }
                        }
                    }

                    _deadProxies.clear();

                    _adminLock.Unlock();
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
                void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t version) override
                {
                    return (_parent.Acquire(interfaceId, className, version));
                }
                void Dangling(const Core::IUnknown* source, const uint32_t interfaceId) override
                {
                    _adminLock.Lock();

                    _parent.Dangling(source, interfaceId);

                    if (_requestObservers.empty() == false) {
                        _deadProxies.emplace_back(interfaceId,source);
                        source->AddRef();
                        _job.Submit();
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
                RPC::Communicator::RemoteConnection* CreateStarter(const RPC::Config& config, const RPC::Object& instance) override
                {
                    RPC::Communicator::RemoteConnection* result = nullptr;

                    if (instance.Type() == RPC::Object::HostType::DISTRIBUTED) {
                        result = Core::ServiceType<RemoteHost>::Create<RPC::Communicator::RemoteConnection>(instance, config);
                    }
                    else {
                        result = RPC::Communicator::CreateStarter(config, instance);
                    }

                    return result;
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
                Proxies _deadProxies;
                Core::WorkerPool::JobType<CommunicatorServer&> _job;
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
                RemoteInstantiation(RemoteInstantiation&&) = delete;
                RemoteInstantiation(const RemoteInstantiation&) = delete;
                RemoteInstantiation& operator=(RemoteInstantiation&&) = delete;
                RemoteInstantiation& operator=(const RemoteInstantiation&) = delete;

                ~RemoteInstantiation() override = default;

            public:
                static IRemoteInstantiation* Create(ServiceMap& parent, const CommunicatorServer& comms, const string& connector)
                {
                    return (new RemoteInstantiation(parent, comms, connector));
                }
                uint32_t AddRef() const override
                {
                    Core::InterlockedIncrement(_refCount);
                    return (Core::ERROR_NONE);
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
                    Job(Job&&) = delete;
                    Job(const Job&) = delete;
                    Job& operator=(Job&&) = delete;
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
                SubSystems(SubSystems&&) = delete;
                SubSystems(const SubSystems&) = delete;
                SubSystems& operator=(SubSystems&&) = delete;
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
            class SubSystemsControlled : public ISubSystem {
            public:
                SubSystemsControlled() = delete;
                SubSystemsControlled(SubSystemsControlled&&) = delete;
                SubSystemsControlled(const SubSystemsControlled&) = delete;
                SubSystemsControlled& operator=(SubSystemsControlled&&) = delete;
                SubSystemsControlled& operator=(const SubSystemsControlled&) = delete;

                SubSystemsControlled(SystemInfo& systemInfo, Server& server, Service* service)
                    : _systemInfo(systemInfo)
                    , _server(server)
                    , _service(service)
                {
                }
                ~SubSystemsControlled() override = default;

            public:
                Core::hresult Set(const ISubSystem::subsystem type, Core::IUnknown* information) override
                {
                    // This override attempts to block subsystem toggle calls that are not in CONTROLLED list of metadata.

                    Core::hresult result = Core::ERROR_BAD_REQUEST;
                    bool allowed = false;

                    if ((_service == nullptr) || (_service->Callsign() == _server.Controller()->Callsign())) {
                        allowed = true;
                    }
                    else {
                        const ISubSystem::subsystem searchType = (type < ISubSystem::NEGATIVE_START ? type
                            : static_cast<ISubSystem::subsystem>(type - ISubSystem::NEGATIVE_START));

                        const std::vector<ISubSystem::subsystem>& controls = _service->SubSystemControl();

                        allowed = (std::find(controls.begin(), controls.end(), searchType) != controls.end());
                    }

                    if (allowed == false) {
                        SYSLOG(Logging::Error, (_T("Toggling a non-controlled state is not allowed")));
                        ASSERT(false && "Toggiling a non-controlled subsystem is not allowed");
                    }
                    else {
                        result = _systemInfo.Set(type, information);
                    }

                    return (result);
                }

            public:
                void Register(ISubSystem::INotification* notification) override
                {
                    _systemInfo.Register(notification);
                }
                void Unregister(ISubSystem::INotification* notification) override
                {
                    _systemInfo.Unregister(notification);
                }
                const Core::IUnknown* Get(const ISubSystem::subsystem type) const override
                {
                    return (_systemInfo.Get(type));
                }
                bool IsActive(const ISubSystem::subsystem type) const override
                {
                    return (_systemInfo.IsActive(type));
                }
                string BuildTreeHash() const override
                {
                    return (_systemInfo.BuildTreeHash());
                }
                string Version() const override
                {
                    return (_systemInfo.Version());
                }

            public:
                BEGIN_INTERFACE_MAP(SubSystemsControlled)
                    INTERFACE_ENTRY(PluginHost::ISubSystem)
                END_INTERFACE_MAP

            private:
                SystemInfo& _systemInfo;
                Server& _server;
                Service* _service;
            };
            class ConfigObserver : public Core::FileSystemMonitor::ICallback {
            public:
                ConfigObserver() = delete;
                ConfigObserver(ConfigObserver&&) = delete;
                ConfigObserver(const ConfigObserver&) = delete;
                ConfigObserver& operator=(ConfigObserver&&) = delete;
                ConfigObserver& operator=(const ConfigObserver&) = delete;

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
            ServiceMap(ServiceMap&&) = delete;
            ServiceMap(const ServiceMap&) = delete;
            ServiceMap& operator=(ServiceMap&&) = delete;
            ServiceMap& operator=(const ServiceMap&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
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
                    server._config.DelegatedReleases(),
                    _engine)
                , _subSystems(this)
                , _authenticationHandler(nullptr)
                , _configObserver(*this, server._config.PluginConfigPath())
                , _shellObservers()
                , _channelObservers()
            {
                if (server._config.PluginConfigPath().empty() == true) {
                    SYSLOG(Logging::Startup, (_T("Dynamic configs disabled.")));
                } else if (_configObserver.IsValid() == false) {
                    SYSLOG(Logging::Startup, (_T("Dynamic configs failed. Can not observe: [%s]"), server._config.PluginConfigPath().c_str()));
                }
            }
            POP_WARNING()
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
                        _server.SecurityRevoke(Configuration().Security());
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
                Core::ProxyType<Server::Channel> entry(_server.Connection(id));
                
                if (entry.IsValid() == true) {
                    entry->Submit(response);
                    return (Core::ERROR_NONE);
                }
                return (Core::ERROR_UNAVAILABLE);
            }
            inline SystemInfo& SubSystemInfo()
            {
                return (_subSystems);
            }
            inline const SystemInfo& SubSystemInfo() const
            {
                return (_subSystems);
            }
            inline ISubSystem* SubSystemsInterface(Service* service = nullptr)
            {
                return (Core::ServiceType<SubSystemsControlled>::Create<ISubSystem>(_subSystems, _server, service));
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
            void Created(const string& callsign, PluginHost::IShell* entry) {
                _notificationLock.Lock();
                for (auto observer : _shellObservers) {
                    observer->Created(callsign, entry);
                }
                _notificationLock.Unlock();
            }
            void Destroy(const string& callsign, PluginHost::IShell* entry) {
                _notificationLock.Lock();
                for (auto observer : _shellObservers) {
                    observer->Destroy(callsign, entry);
                }
                _notificationLock.Unlock();

            }
            void Register(PluginHost::IPlugin::INotification* sink)
            {
                _notificationLock.Lock();

                Notifiers::iterator it(std::find(_notifiers.begin(), _notifiers.end(), sink));

                ASSERT(it == _notifiers.end());

                if (it == _notifiers.end()) {
                    sink->AddRef();
                    _notifiers.push_back(sink);
                }

                // Tell this "new" sink all our actived plugins..
                Plugins::iterator index(_services.begin());

                // Notifty all plugins that we have sofar..
                while (index != _services.end()) {
                    ASSERT(index->second.IsValid());

                    Core::ProxyType<Service> service(index->second);

                    ASSERT(service.IsValid());

                    if ( (service.IsValid() == true) && (service->State() == IShell::ACTIVATED) ) {
                        sink->Activated(service->Callsign(), &(service.operator*()));

                        // Report any composit plugins that are active..
                        service->Composits().Visit([&](const string& callsign, IShell* proxy) {
                            if (proxy->State() == PluginHost::IShell::state::ACTIVATED) {
                                sink->Activated(callsign, proxy);
                            }
                        });
                    }

                    index++;
                }

                _notificationLock.Unlock();
            }
            void Unregister(const PluginHost::IPlugin::INotification* sink)
            {
                _notificationLock.Lock();

                Notifiers::iterator index(std::find(_notifiers.begin(), _notifiers.end(), sink));
                ASSERT(index != _notifiers.end());

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
            void Unregister(const IShell::ICOMLink::INotification* sink)
            {
                _processAdministrator.Unregister(sink);
            }
            void Register(IShell::IConnectionServer::INotification* sink)
            {
                _notificationLock.Lock();

                ASSERT(std::find(_channelObservers.begin(), _channelObservers.end(), sink) == _channelObservers.end());

                _channelObservers.push_back(sink);

                ASSERT(sink != nullptr);
                sink->AddRef();

                _server.Visit([sink](const Channel& channel) {
                        if (channel.IsOpen() == true) {
                            sink->Opened(channel.Id());
                        }
                    });

                _notificationLock.Unlock();
            }
            void Unregister(const IShell::IConnectionServer::INotification* sink)
            {
                _notificationLock.Lock();

                ChannelObservers::iterator index(std::find(_channelObservers.begin(), _channelObservers.end(), sink));

                ASSERT(index != _channelObservers.end());

                if (index != _channelObservers.end()) {
                    (*index)->Release();
                    _channelObservers.erase(index);
                }

                _notificationLock.Unlock();
            }
            void Register(Exchange::Controller::IShells::INotification* sink) {
                _notificationLock.Lock();

                // Make sure a sink is not registered multiple times.
                ShellNotifiers::iterator index(std::find(_shellObservers.begin(), _shellObservers.end(), sink));
                ASSERT(index == _shellObservers.end());

                if (index == _shellObservers.end()) {
                    _shellObservers.push_back(sink);
                    sink->AddRef();

                    for (const std::pair<const string, Core::ProxyType<Service>>& entry : _services) {
                        sink->Created(entry.first, entry.second.operator->());
                        // Report any composit plugins that are active..
                        entry.second->Composits().Visit([&](const string& callsign, IShell* proxy) {
                            sink->Created(callsign, proxy);
                        });
                    }
                }

                _notificationLock.Unlock();
            }
            void Unregister(Exchange::Controller::IShells::INotification* sink) {
                _notificationLock.Lock();

                ShellNotifiers::iterator index(std::find(_shellObservers.begin(), _shellObservers.end(), sink));

                // Make sure you do not unregister something you did not register !!!
                ASSERT(index != _shellObservers.end());

                if (index != _shellObservers.end()) {
                    (*index)->Release();
                    _shellObservers.erase(index);
                }

                _notificationLock.Unlock();
            }
            RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId)
            {
                return (connectionId != 0 ? _processAdministrator.Connection(connectionId) : nullptr);
            }
            inline Core::ProxyType<Service> Insert(const Plugin::Config& configuration, const Service::mode mode)
            {
                // Whatever plugin is needse, we at least have our Metadata plugin available (as the first entry :-).
                Core::ProxyType<Service> newService(Core::ProxyType<Service>::Create(Configuration(), configuration, *this, mode, _engine));

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
                    newConfiguration.FromString(original->ConfigLine());
                    newConfiguration.Callsign = newCallsign;

                    Core::ProxyType<Service> clone = Core::ProxyType<Service>::Create(Configuration(), newConfiguration, *this, Service::mode::CLONED, _engine);

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
                Plugins::iterator index(_services.find(callSign));

                if (index != _services.end()) {
                    index->second->Destroy();
                    _services.erase(index);
                }

                _adminLock.Unlock();
            }
            inline Iterator Services()
            {
                Shells workingList;

                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                workingList.reserve(_services.size());

                for (const std::pair<const string, Core::ProxyType<Service>>& entry : _services) {
                    workingList.emplace(std::piecewise_construct,
                        std::make_tuple(entry.first),
                        std::make_tuple(entry.second->QueryInterface<PluginHost::IShell>()));

                    // Report any composit plugins that are active..
                    entry.second->Composits().Visit([&](const string& callsign, IShell* proxy) {
                        proxy->AddRef();
                        workingList.emplace(std::piecewise_construct,
                            std::make_tuple(callsign),
                            std::make_tuple(proxy));
                    });
                }

                return (Iterator(std::move(workingList)));
            }
            inline void Notification(const string& callsign, const string& message)
            {
                _server.Notification(callsign, message);
            }
            inline void Notification(const string& callsign, const string& event, const string& message)
            {
                _server.Notification(callsign, event, message);
            }
            #if THUNDER_RESTFULL_API
            inline void Notification(const string& message)
            {
                _server.Controller()->Notification(message);
            }
            #endif
            void GetMetadata(Core::JSON::ArrayType<Metadata::Service>& metaData) const
            {
                std::vector<Core::ProxyType<Service>> workingList;

                _adminLock.Lock();

                workingList.reserve(_services.size());

                for (auto entry : _services) {

                    std::vector<Core::ProxyType<Service>>::iterator index = workingList.begin();

                    while ((index != workingList.end()) && ((*index)->Callsign() < entry.first)) {
                        index++;
                    }
                    workingList.insert(index, entry.second);
                }

                _adminLock.Unlock();

                for (const Core::ProxyType<Service>& service : workingList) {

                    service->GetMetadata(metaData.Add());
                    std::vector<std::pair<string, string>> locals;

                    // Report any composit plugins that are active..
                    service->Composits().Visit([&](const string& callsign, IShell* proxy) {
                        string metadata;
                        std::vector<std::pair<string, string>>::iterator index = locals.begin();
                        while ((index != locals.end()) && (index->first < callsign)) {
                            index++;
                        }

                        proxy->Metadata(metadata);
                        locals.insert(index, std::pair<string,string>(callsign, metadata));
                    });

                    for (auto entry : locals) {
                        metaData.Add().FromString(entry.second);
                    }
                }
            }
            void GetMetadata(Core::JSON::ArrayType<Metadata::Channel>& metaData) const
            {
                _adminLock.Lock();
                _processAdministrator.Visit([&](const RPC::Communicator::Client& element)
                    {
                        Metadata::Channel& entry = metaData.Add();
                        entry.ID = element.Extension().Id();

                        entry.Activity = element.Source().IsOpen();
                        entry.JSONState = Metadata::Channel::state::COMRPC;
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
                size_t pos;
                Core::ProxyType<Service> selected;
                uint32_t result = Core::ERROR_UNAVAILABLE;
                string baseName = callSign;

                if ((pos = callSign.find_first_of(PluginHost::ICompositPlugin::Delimiter)) != string::npos) {
                    // This is a Composit plugin identifier..
                    baseName = callSign.substr(0, pos);
               }

                _adminLock.Lock();

                for (auto index : _services) {
                    const string& source(index.first);
                    uint32_t length = static_cast<uint32_t>(source.length());

                    if (baseName.compare(0, source.length(), source) == 0) {
                        if (baseName.length() == length) {
                                // Service found, did not requested specific version
                                selected = index.second;
                                result = Core::ERROR_NONE;
                                break;
                        }
                        else if (baseName[length] == '.') {
                                // Requested specific version of a plugin
                                if (index.second->HasVersionSupport(baseName.substr(length + 1)) == true) {
                                    // Requested version of service is supported!
                                    selected = index.second;
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

                if (selected.IsValid() == true) {
                    if (pos == string::npos) {
                        service = Core::ProxyType<IShell>(selected);
                    }
                    else {
                        service = selected->Composits().Source(callSign.substr(pos + 1));
                        result = (service.IsValid() == false ? Core::ERROR_UNKNOWN_KEY : Core::ERROR_NONE);
                    }
                }

                return (result);
            }
            inline const PluginHost::Config& Configuration() const
            {
                return _server.Configuration();
            }
            void Evaluate()
            {
                _adminLock.Lock();

                Plugins::iterator index(_services.begin());

                RecursiveNotification(index);
            }

            uint32_t FromLocator(const string& identifier, Core::ProxyType<Service>& service, bool& serviceCall);

            void Open(std::vector<PluginHost::ISubSystem::subsystem>& externallyControlled);
            void Startup();
            void Close();
            void Destroy();

            void Opened(const uint32_t id)
            {
                _notificationLock.Lock();

                for (auto& sink : _channelObservers) {
                    sink->Opened(id);
                }

                _notificationLock.Unlock();
            }
            void Closed(const uint32_t id)
            {
                _notificationLock.Lock();

                for (auto& sink : _channelObservers) {
                    sink->Closed(id);
                }

                _notificationLock.Unlock();
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
                            SYSLOG(Logging::Fatal, (_T("Plugin config file [%s] could not be opened."), file.Name().c_str()));
                        }
                        else {
                            Plugin::Config pluginConfig;
                            Core::OptionalType<Core::JSON::Error> error;
                            pluginConfig.IElement::FromFile(file, error);
                            if (error.IsSet() == true) {
                                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                            }
                            else if ((pluginConfig.ClassName.Value().empty() == true) || (pluginConfig.Locator.Value().empty() == true)) {
                                SYSLOG(Logging::Fatal, (_T("Plugin config file [%s] does not contain classname or locator."), file.Name().c_str()));
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
            void RecursiveNotification(Plugins::iterator& index)
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
            inline Core::WorkerPool& WorkerPool()
            {
                return (_server.WorkerPool());
            }

        private:
            Server& _server;
            mutable Core::CriticalSection _adminLock;
            Core::CriticalSection _notificationLock;
            Plugins _services;
            mutable RemoteInstantiators _instantiators;
            Notifiers _notifiers;
            Core::ProxyType<RPC::InvokeServer> _engine;
            CommunicatorServer _processAdministrator;
            Core::SinkType<SubSystems> _subSystems;
            IAuthenticate* _authenticationHandler;
            ConfigObserver _configObserver;
            ShellNotifiers _shellObservers;
            ChannelObservers _channelObservers;
        };

        // Connection handler is the listening socket and keeps track of all open
        // Links. A Channel is identified by an ID, this way, whenever a link dies
        // (is closed) during the service process, the ChannelMap will
        // not find it and just "flush" the presented work.
        class Channel : public PluginHost::Channel {
        public:
            class Job : public Core::IDispatch {
            public:
                Job(Job&&) = delete;
                Job(const Job&) = delete;
                Job& operator=(Job&&) = delete;
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
                    ASSERT(_server != nullptr);
                    Core::ProxyType<Channel> channel(_server->Connection(_ID));
                    if (channel.IsValid() == true) {
                        channel->Close(0);
                    }
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
                    Core::ProxyType<Channel> channel(_server->Connection(_ID));
                    if (channel.IsValid() == true) {
                        channel->Submit(package);
                    }
                }
                void RequestClose() {
                    ASSERT(_server != nullptr);
                    Core::ProxyType<Channel> channel (_server->Connection(_ID));
                    if (channel.IsValid() == true) {
                        channel->RequestClose();
                    }
                }
                string Callsign() const {
                    ASSERT(_service.IsValid() == true);
                    return _service->Callsign();
                }
                void Completed() {
                    ASSERT(_ID != static_cast<uint32_t>(~0));
                    ASSERT(_server != nullptr);
                    Core::ProxyType<Channel> channel(_server->Connection(_ID));
                    if (channel.IsValid() == true) {
                        channel->Pop();
                    }
                    Clear();
                }
            private:
                uint32_t _ID;
                Server* _server;
                Core::ProxyType<Service> _service;
            };

        private:
            class WebRequestJob : public Job {
            public:
                WebRequestJob(WebRequestJob&&) = delete;
                WebRequestJob(const WebRequestJob&) = delete;
                WebRequestJob& operator=(WebRequestJob&&) = delete;
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
                        if(_request->Verb == Request::HTTP_POST) {
                            Core::ProxyType<Web::JSONRPC::Body> message(_request->Body<Web::JSONRPC::Body>());
                            if ( (message->Report().IsSet() == true) || (message->IsComplete() == false) ) {
                                // Looks like we have a corrupted message.. Respond if posisble, with an error
                                response = IFactories::Instance().Response();

                                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                                response->Message = _T("JSON was incorrectly formatted");

                                // If we also do not have an id, we can not return a suitable JSON message!
                                if (message->Recorded().IsSet() == true) {
                                    message->Id = message->Recorded().Value();
                                    message->Error.Text = message->Report().Value().Message();
                                }
                                else if (message->IsComplete() == true) {
                                    message->Id.Null(true);
                                    message->Error.Text = message->Report().Value().Message();
                                }
                                else if (message->Id.IsSet() == false) {
                                    message->Clear();
                                    message->Id.Null(true);
                                    message->Error.Text = _T("Incomplete JSON send");
                                }
                                else {
                                    uint32_t id = message->Id.Value();
                                    message->Clear();
                                    message->Id = id;
                                    message->Error.Text = _T("Incomplete JSON send");
                                }

                                message->Error.SetError(Core::ERROR_PARSE_FAILURE);
                                response->Body(Core::ProxyType<Web::IBody>(message));
                            }
                            else {
                                if (HasService() == true) {
                                    message->ImplicitCallsign(GetService().Callsign());
                                }

                                if (message->IsSet()) {
                                    Core::ProxyType<Core::JSONRPC::Message> body = Job::Process(_token, Core::ProxyType<Core::JSONRPC::Message>(message));

                                    // If we have no response body, it looks like an async-call...
                                    if (body.IsValid() == false) {
                                        // It's a a-synchronous call if the id was set but we do not yet have a resposne.
                                        // If the id of the originating message was not set, it is a Notification and no
                                        // response is expected at all, just report HTTP NO_CONTENT than
                                        if (message->Id.IsSet() == false) {
                                            response = IFactories::Instance().Response();
                                            response->ErrorCode = Web::STATUS_NO_CONTENT;
                                            response->Message = _T("A JSONRPC Notification was send to the server. Processed it..");
                                        }
                                    }
                                    else {
                                        response = IFactories::Instance().Response();
                                        response->Body(body);
                                        if (body->Error.IsSet() == false) {
                                            response->ErrorCode = Web::STATUS_OK;
                                            response->Message = _T("JSONRPC executed succesfully");
                                        }
                                        else {
                                            response->ErrorCode = Web::STATUS_ACCEPTED;
                                            response->Message = _T("Failure on JSONRPC: ") + Core::NumberType<int32_t>(body->Error.Code).Text();
                                        }
                                    }

                                    if (_request->Connection.Value() == Web::Request::CONNECTION_CLOSE) {
                                        Job::RequestClose();
                                    }
                                }
                                else {
                                    response = IFactories::Instance().Response();
                                    response->ErrorCode = Web::STATUS_ACCEPTED;
                                    response->Message = _T("Failed to parse JSONRPC message");
                                }
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

                    Job::Completed();
                }
                string Identifier() const override {
                    string identifier;
                    if (_jsonrpc == false) {
                        identifier = _T("{ \"type\": \"HTTP\",  }");
                    }
                    else {
                        Core::ProxyType<Core::JSONRPC::Message> message(_request->Body<Core::JSONRPC::Message>());

                        identifier = Core::Format(_T("{ \"type\": \"HTTP\", \"id\": %d, \"method\": \"%s\", \"parameters\": %s }"), message->Id.Value(), message->Designator.Value().c_str(), message->Parameters.Value().c_str());
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
                JSONElementJob(JSONElementJob&&) = delete;
                JSONElementJob(const JSONElementJob&) = delete;
                JSONElementJob& operator=(JSONElementJob&&) = delete;
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
                        Core::ProxyType<Web::JSONRPC::Body> message(_element);

                        ASSERT(message.IsValid() == true);

                        if (message->Report().IsSet() == true) {
                            // If we also do not have an id, we can not return a suitable JSON message!
                            if (message->Recorded().IsSet() == false) {
                                message->Id.Null(true);
                            }
                            else {
                                message->Id = message->Recorded().Value();
                            }

                            message->Error.SetError(Core::ERROR_PARSE_FAILURE);
                            message->Error.Text = message->Report().Value().Message();
                        }
                        else {
                            if (HasService() == true) {
                                message->ImplicitCallsign(GetService().Callsign());
                            }

                            _element = Core::ProxyType<Core::JSON::IElement>(Job::Process(_token, Core::ProxyType<Core::JSONRPC::Message>(message)));
                        }

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

                    Job::Completed();
                }
                string Identifier() const override {
                    if (_jsonrpc == true) {
                        Core::ProxyType<Core::JSONRPC::Message> message(_element);
                        if (message.IsValid() == true) {
                            return (Core::Format(_T("{ \"type\": \"WS\", \"id\": %d, \"method\": \"%s\", \"parameters\": %s }"), message->Id.Value(), message->Designator.Value().c_str(), message->Parameters.Value().c_str()));
                        }
                    }
                    string message;
                    _element->ToString(message);
                    return (Core::Format(_T("{ \"type\": \"WS\", \"callsign\": \"%s\", \"message\": %s }"), Callsign().c_str(), message.c_str()));
                }

            private:
                Core::ProxyType<Core::JSON::IElement> _element;
                string _token;
                bool _jsonrpc;
            };
            class TextJob : public Job {
            public:
                TextJob(TextJob&&) = delete;
                TextJob(const TextJob&) = delete;
                TextJob& operator=(TextJob&&) = delete;
                TextJob& operator=(const TextJob&) = delete;

                TextJob() = default;
                ~TextJob() override = default;

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

                    Job::Completed();
                }
                string Identifier() const override {
                    return(_T("PluginServer::Channel::TextJob::") + Callsign());
                }

            private:
                string _text;
            };

            using Jobs = std::vector< Core::ProxyType<Core::IDispatch> >;

        public:
            Channel() = delete;
            Channel(Channel&& copy) = delete;
            Channel(const Channel& copy) = delete;
            Channel& operator=(Channel&&) = delete;
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
            inline void Push(Core::ProxyType<Core::IDispatch>&& job) {
                _adminLock.Lock();
                if (_jobs.empty()) {
                    _parent.Submit(Core::ProxyType<Core::IDispatch>(job));
                }
                _jobs.emplace(_jobs.begin(), job);
                _adminLock.Unlock();
            }
            inline void Pop() {
                _adminLock.Lock();
                ASSERT(_jobs.empty() == false);
                if (_jobs.empty() == false) {
                    _jobs.pop_back();
                    if (_jobs.empty() == false) {
                        _parent.Submit(_jobs.back());
                    }
                }
                _adminLock.Unlock();
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
                            Push(Core::ProxyType<Core::IDispatch>(job));
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
                        Push(Core::ProxyType<Core::IDispatch>(job));
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
                    Push(Core::ProxyType<Core::IDispatch>(job));
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

                    _parent.Operational(Id(), false);

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
                else if ((IsOpen() == true) && (IsWebSocket() == false)) {
                    _parent.Operational(Id(), true);
                }
            }

            friend class Core::SocketServerType<Channel>;

            inline void Id(const uint32_t id)
            {
                SetId(id);
            }

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
            Core::CriticalSection _adminLock;
            PluginHost::ISecurity* _security;
            Core::ProxyType<Service> _service;
            bool _requestClose;
            Jobs _jobs;

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

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            ChannelMap(Server& parent, const Core::NodeId& listeningNode)
                : Core::SocketServerType<Channel>(listeningNode)
                , _parent(parent)
                , _connectionCheckTimer(0)
                , _job(*this)
            {
            }
            POP_WARNING()
            ~ChannelMap() = default;

        public:
            uint32_t Open(const uint32_t waitTime, const uint16_t connectionCheckTimer) {
                _connectionCheckTimer = connectionCheckTimer * 1000;
                if (connectionCheckTimer != 0) {
                    _job.Reschedule(Core::Time::Now().Add(_connectionCheckTimer).Ticks());
                }
                return(BaseClass::Open(waitTime));
            }
            uint32_t Close(const uint32_t waitTime) {
                _job.Revoke();

                // Start by closing the server thread..
                // Kill all open connections, we are shutting down !!!
                BaseClass::Close(waitTime);
                BaseClass::Cleanup();

                return (Core::ERROR_NONE);
            }
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
            inline Core::ProxyType<Channel> Connection(const uint32_t id) {
                return(BaseClass::Client(id));
            }
            void Cleanup()
            {
                if (_connectionCheckTimer == 0) {
                    _job.Submit();
                }
            }
            void GetMetadata(Core::JSON::ArrayType<Metadata::Channel>& metaData) const;

        private:
            friend class Core::ThreadPool::JobType<ChannelMap&>;

            string JobIdentifier() const {
                return (_T("PluginServer::ChannelMap::Cleanup"));
            }
            void Dispatch()
            {
                TRACE(Activity, (string(_T("Cleanup job running..\n"))));

                // Next Clean all Id's from JSONRPC nolonger available
                // 
                // First check and clear, closed connections
                BaseClass::Cleanup();

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
            uint32_t _connectionCheckTimer;
            Core::WorkerPool::JobType<ChannelMap&> _job;
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
        inline void SecurityRevoke(PluginHost::ISecurity* security)
        {
            return (_connections.SecurityRevoke(security));
        }
        inline void Cleanup()
        {
            return (_connections.Cleanup());
        }
        inline Core::ProxyType<Server::Channel> Connection(const uint32_t id)
        {
            return (_connections.Connection(id));
        }
        inline void DumpMetadata() {
            PostMortemData data;
            _dispatcher.Snapshot(data.WorkerPool);

            Core::JSON::ArrayType<Metadata::Server::Minion>::Iterator index(data.WorkerPool.ThreadPoolRuns.Elements());

            while (index.Next() == true) {

                std::list<Core::callstack_info> stackList;

                ::DumpCallStack((ThreadId)index.Current().Id.Value(), stackList);
                for(const Core::callstack_info& entry : stackList)
                {
                    std::string symbol = entry.function.empty() ? "Unknown symbol" : entry.function;
                    fprintf(stderr, "[%s]:[%s]:[%d]:[%p]\n",entry.module.c_str(), symbol.c_str(),entry.line,entry.address);
                }
                fflush(stderr);

                PostMortemData::Callstack dump;
                dump.Id = index.Current().Id.Value();

                for (const Core::callstack_info& info : stackList) {
                    dump.Data.Add() = CallstackData(info);
                }

                data.Callstacks.Add(dump);
            }

            // Drop the workerpool info (what is currently running and what is pending) to a file..
            Core::File dumpFile(_config.PostMortemPath() + "ThunderInternals.json");
            if (dumpFile.Create(false) == true) {
                data.IElement::ToFile(dumpFile);
            }
        }
        inline ServiceMap& Services()
        {
            return (_services);
        }
        inline void Metadata(PluginHost::Metadata::Version& data) const
        {
            data.Major = PluginHost::Major;
            data.Minor = PluginHost::Minor;
            data.Patch = PluginHost::Patch;
            data.Hash = string(Core::System::ModuleBuildRef());
        }
        inline void Metadata(Core::JSON::ArrayType<PluginHost::Metadata::Channel>& data) const
        {
            _connections.GetMetadata(data);
            _services.GetMetadata(data);
        }
        inline void Metadata(Core::JSON::ArrayType<PluginHost::Metadata::Service>& data) const {
            _services.GetMetadata(data);
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

        void Notification(const string& callsign, const string& message);
        void Notification(const string& callsign, const string& event, const string& message);
        void Open();
        void Close();

        uint32_t Persist()
        {
            Override infoBlob(_config, _services, Configuration().PersistentPath() + PluginOverrideFile);

            return (infoBlob.Save());
        }
        uint32_t Load()
        {
            Override infoBlob(_config, _services, Configuration().PersistentPath() + PluginOverrideFile);

            return (infoBlob.Load());
        }

        void Visit(const std::function<void(const Channel&)>& handler)
        {
            ChannelMap::Iterator it = _connections.Clients();

            while (it.Next() == true) {
                handler(*it.Client());
            }
        }

    private:
        void Operational(const uint32_t id, const bool upAndRunning)
        {
            if (upAndRunning == true) {
                Services().Opened(id);
            }
            else {
                Services().Closed(id);
            }
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
