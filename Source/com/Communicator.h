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

#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include "Module.h"
#include "Ids.h"
#include "Administrator.h"
#include "IUnknown.h"
#include "ICOM.h"

#ifdef PROCESSCONTAINERS_ENABLED
#include "../processcontainers/ProcessContainer.h"
#endif

#include "../tracing/TraceUnit.h"

namespace WPEFramework {
namespace RPC {

    class EXTERNAL Object {
    public:
        enum class HostType {
            LOCAL,
            DISTRIBUTED,
            CONTAINER
        };

        Object()
            : _locator()
            , _className()
            , _callsign()
            , _interface(~0)
            , _version(~0)
            , _user()
            , _group()
            , _threads()
            , _priority()
            , _type(HostType::LOCAL)
            , _remoteAddress()
            , _configuration()
        {
        }
        Object(const Object& copy)
            : _locator(copy._locator)
            , _className(copy._className)
            , _callsign(copy._callsign)
            , _interface(copy._interface)
            , _version(copy._version)
            , _user(copy._user)
            , _group(copy._group)
            , _threads(copy._threads)
            , _priority(copy._priority)
            , _type(copy._type)
            , _remoteAddress(copy._remoteAddress)
            , _configuration(copy._configuration)
        {
        }
        Object(const string& locator,
            const string& className,
            const string& callsign,
            const uint32_t interface,
            const uint32_t version,
            const string& user,
            const string& group,
            const uint8_t threads,
            const int8_t priority,
            const HostType type,
            const string& remoteAddress,
            const string& configuration)
            : _locator(locator)
            , _className(className)
            , _callsign(callsign)
            , _interface(interface)
            , _version(version)
            , _user(user)
            , _group(group)
            , _threads(threads)
            , _priority(priority)
            , _type(type)
            , _remoteAddress(remoteAddress)
            , _configuration(configuration)
        {
        }
        ~Object()
        {
        }

        Object& operator=(const Object& RHS)
        {
            _locator = RHS._locator;
            _className = RHS._className;
            _callsign = RHS._callsign;
            _interface = RHS._interface;
            _version = RHS._version;
            _user = RHS._user;
            _group = RHS._group;
            _threads = RHS._threads;
            _priority = RHS._priority;
            _type = RHS._type;
            _remoteAddress = RHS._remoteAddress;
            _configuration = RHS._configuration;

            return (*this);
        }

    public:
        inline const string& Locator() const
        {
            return (_locator);
        }
        inline const string& ClassName() const
        {
            return (_className);
        }
        inline const string& Callsign() const
        {
            return (_callsign);
        }
        inline uint32_t Interface() const
        {
            return (_interface);
        }
        inline uint32_t Version() const
        {
            return (_version);
        }
        inline const string& User() const
        {
            return (_user);
        }
        inline const string& Group() const
        {
            return (_group);
        }
        inline uint8_t Threads() const
        {
            return (_threads);
        }
        inline int8_t Priority() const
        {
            return (_priority);
        }
        inline HostType Type() const
        {
            return (_type);
        }
        inline const Core::NodeId RemoteAddress() const
        {
            return (Core::NodeId(_remoteAddress.c_str()));
        }
        inline const string& Configuration() const
        {
            return (_configuration);
        }

    private:
        string _locator;
        string _className;
        string _callsign;
        uint32_t _interface;
        uint32_t _version;
        string _user;
        string _group;
        uint8_t _threads;
        int8_t _priority;
        HostType _type;
        string _remoteAddress;
        string _configuration;
    };

    class EXTERNAL Config {
    private:
        Config& operator=(const Config&);

    public:
        Config()
            : _connector()
            , _hostApplication()
            , _persistent()
            , _system()
            , _data()
            , _volatile()
            , _application()
            , _proxyStub()
            , _postMortem()
        {
        }
        Config(
            const string& connector,
            const string& hostApplication,
            const string& persistentPath,
            const string& systemPath,
            const string& dataPath,
            const string& volatilePath,
            const string& applicationPath,
            const string& proxyStubPath,
            const string& postMortem)
            : _connector(connector)
            , _hostApplication(hostApplication)
            , _persistent(persistentPath)
            , _system(systemPath)
            , _data(dataPath)
            , _volatile(volatilePath)
            , _application(applicationPath)
            , _proxyStub(proxyStubPath)
            , _postMortem(postMortem)
        {
        }
        Config(const Config& copy)
            : _connector(copy._connector)
            , _hostApplication(copy._hostApplication)
            , _persistent(copy._persistent)
            , _system(copy._system)
            , _data(copy._data)
            , _volatile(copy._volatile)
            , _application(copy._application)
            , _proxyStub(copy._proxyStub)
            , _postMortem(copy._postMortem)
        {
        }
        ~Config()
        {
        }

    public:
        inline const string& Connector() const
        {
            return (_connector);
        }
        inline const string& HostApplication() const
        {
            return (_hostApplication);
        }
        inline const string& PersistentPath() const
        {
            return (_persistent);
        }
        inline const string& SystemPath() const
        {
            return (_system);
        }
        inline const string& DataPath() const
        {
            return (_data);
        }
        inline const string& VolatilePath() const
        {
            return (_volatile);
        }
        inline const string& ApplicationPath() const
        {
            return (_application);
        }
        inline const string& ProxyStubPath() const
        {
            return (_proxyStub);
        }
        inline const string& PostMortemPath() const
        {
            return (_postMortem);
        }


    private:
        string _connector;
        string _hostApplication;
        string _persistent;
        string _system;
        string _data;
        string _volatile;
        string _application;
        string _proxyStub;
        string _postMortem;
    };

    class EXTERNAL Process {
    public:
        Process() = delete;
        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        Process(const uint32_t sequenceNumber, const Config& config, const Object& instance)
            : _options(config.HostApplication())
        {
            ASSERT(instance.Locator().empty() == false);
            ASSERT(instance.ClassName().empty() == false);
            ASSERT(config.Connector().empty() == false);

            _options.Add(_T("-l")).Add(instance.Locator());
            _options.Add(_T("-c")).Add(instance.ClassName());
            _options.Add(_T("-C")).Add(instance.Callsign());
            _options.Add(_T("-r")).Add(config.Connector());
            _options.Add(_T("-i")).Add(Core::NumberType<uint32_t>(instance.Interface()).Text());
            _options.Add(_T("-x")).Add(Core::NumberType<uint32_t>(sequenceNumber).Text());
            
            if (instance.Version() != static_cast<uint32_t>(~0)) {
                _options.Add(_T("-V")).Add(Core::NumberType<uint32_t>(instance.Version()).Text());
            }
            if (instance.User().empty() == false) {
                _options.Add(_T("-u")).Add(instance.User());
            }
            if (instance.Group().empty() == false) {
                _options.Add(_T("-g")).Add(instance.Group());
            }
            if (config.PersistentPath().empty() == false) {
                _options.Add(_T("-p")).Add('"' + config.PersistentPath() + '"');
            }
            if (config.SystemPath().empty() == false) {
                _options.Add(_T("-s")).Add('"' + config.SystemPath() + '"');
            }
            if (config.DataPath().empty() == false) {
                _options.Add(_T("-d")).Add('"' + config.DataPath() + '"');
            }
            if (config.ApplicationPath().empty() == false) {
                _options.Add(_T("-a")).Add('"' + config.ApplicationPath() + '"');
            }
            if (config.VolatilePath().empty() == false) {
                _options.Add(_T("-v")).Add('"' + config.VolatilePath() + '"');
            }
            if (config.ProxyStubPath().empty() == false) {
                _options.Add(_T("-m")).Add('"' + config.ProxyStubPath() + '"');
            }
            if (config.PostMortemPath().empty() == false) {
                _options.Add(_T("-P")).Add('"' + config.PostMortemPath() + '"');
            }
            if (instance.Threads() > 1) {
                _options.Add(_T("-t")).Add(Core::NumberType<uint8_t>(instance.Threads()).Text());
            }

            _priority = instance.Priority();
        }
        const string& Command() const
        {
            return (_options.Command());
        }
        Core::Process::Options::Iterator Options() const
        {
            return (_options.Get());
        }
        uint32_t Launch(uint32_t& id)
        {
            uint32_t loggingSettings = (Logging::LoggingType<Logging::Startup>::IsEnabled() ? 0x01 : 0) | (Logging::LoggingType<Logging::Shutdown>::IsEnabled() ? 0x02 : 0) | (Logging::LoggingType<Logging::Notification>::IsEnabled() ? 0x04 : 0);
            _options.Add(_T("-e")).Add(Core::NumberType<uint32_t>(loggingSettings).Text());

            // Start the external process launch..
            Core::Process fork(false);

            uint32_t result = fork.Launch(_options, &id);

            if ((result == Core::ERROR_NONE) && (_priority != 0)) {
                Core::ProcessInfo newProcess(id);
                newProcess.Priority(newProcess.Priority() + _priority);
            }

            return (result);
        }

    private:
        Core::Process::Options _options;
        int8_t _priority;
    };

    struct EXTERNAL IMonitorableProcess : public virtual Core::IUnknown {
        enum { ID = ID_MONITORABLE_PROCESS };

        virtual ~IMonitorableProcess() {}

        virtual string Callsign() const = 0;
    };

    class EXTERNAL Communicator {
    protected:
        class ChannelLink;

        class EXTERNAL RemoteConnection : public IRemoteConnection {
        private:
            friend class RemoteConnectionMap;

            RemoteConnection(const RemoteConnection&) = delete;
            RemoteConnection& operator=(const RemoteConnection&) = delete;

        protected:
            RemoteConnection()
                : _channel()
                , _id(_sequenceId++)
                , _remoteId(0)
            {
            }
            RemoteConnection(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const uint32_t remoteId)
                : _channel(channel)
                , _id(_sequenceId++)
                , _remoteId(remoteId)
            {
            }

        public:
            ~RemoteConnection()
            {
            }

        public:
            uint32_t Id() const override;
            uint32_t RemoteId() const override;
            void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) override;
            void Terminate() override;
            void PostMortem() override
            {
                // This is really something that needs to be done by the specific implementations.
            }
            uint32_t Launch() override
            {
                return (Core::ERROR_NONE);
            }

            inline bool IsOperational() const
            {
                return (_channel.IsValid() == true);
            }
            inline Core::ProxyType<Core::IPCChannel> Channel()
            {
                ASSERT(_channel.IsValid() == true);

                return (Core::ProxyType<Core::IPCChannel>(_channel));
            }
            void Open(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const uint32_t id)
            {
                ASSERT(_channel.IsValid() == false);

                // Seems we received an interface from the otherside. Prepare the actual stub around it.
                TRACE_L1("Link announced. All up and running %d, has announced itself.", Id());

                _channel = channel;
                _remoteId = id;
            }
            void Close()
            {
                if (_channel.IsValid() == true) {
                    _channel->Source().Close(0);
                }
            }

            BEGIN_INTERFACE_MAP(RemoteConnection)
                INTERFACE_ENTRY(IRemoteConnection)
            END_INTERFACE_MAP

        private:
            Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>> _channel;
            uint32_t _id;
            uint32_t _remoteId;
            static std::atomic<uint32_t> _sequenceId;
        };

    private:
        class EXTERNAL LocalProcess : public RemoteConnection, public IMonitorableProcess {
        public:
            friend class Core::Service<LocalProcess>;

            LocalProcess() = delete;
            LocalProcess(const LocalProcess&) = delete;
            LocalProcess& operator=(const LocalProcess&) = delete;

            LocalProcess(const Config& config, const Object& instance)
                : _callsign(instance.Callsign())
                , _id(0)
                , _process(RemoteConnection::Id(), config, instance)
            {
            }
            ~LocalProcess() override = default;

        public:
            string Callsign() const override
            {
                return (_callsign);
            }
            uint32_t Launch() override
            {
                return (_process.Launch(_id));
            }
            const string& Command() const
            {
                return (_process.Command());
            }
            Core::Process::Options::Iterator Options() const
            {
                return (_process.Options());
            }
            void PostMortem() override;

        private:
            BEGIN_INTERFACE_MAP(LocalProcess)
                INTERFACE_ENTRY(IRemoteConnection)
                INTERFACE_ENTRY(IMonitorableProcess)
            END_INTERFACE_MAP

            void Terminate() override;
            uint32_t RemoteId() const override;

        private:
            string _callsign;
            uint32_t _id;
            Process _process;
        };
#ifdef PROCESSCONTAINERS_ENABLED

        class EXTERNAL ContainerProcess  : public RemoteConnection, public IMonitorableProcess {
        private:
            class ContainerConfig : public Core::JSON::Container {
            public:
                ContainerConfig(const ContainerConfig&) = delete;
                ContainerConfig& operator=(const ContainerConfig&) = delete;

                ContainerConfig()
                    : Core::JSON::Container()
#ifdef __DEBUG__
                    , ContainerPath()
#endif
                {
#ifdef __DEBUG__
                    Add(_T("containerpath"), &ContainerPath);
#endif
                }
                ~ContainerConfig() = default;

#ifdef __DEBUG__
                Core::JSON::String ContainerPath;
#endif
            };

        public:
            friend class Core::Service<ContainerProcess>;

            ContainerProcess(const ContainerProcess&) = delete;
            ContainerProcess& operator=(const ContainerProcess&) = delete;

            ContainerProcess(const Config& baseConfig, const Object& instance)
                : _callsign(instance.Callsign())
                , _id(0)
                , _process(RemoteConnection::Id(), baseConfig, instance)
            {
                ProcessContainers::IContainerAdministrator& admin = ProcessContainers::IContainerAdministrator::Instance();

                std::vector<string> searchpaths(3);
                searchpaths[0] = baseConfig.VolatilePath();
                searchpaths[1] = baseConfig.PersistentPath();
                searchpaths[2] = baseConfig.DataPath();

#ifdef __DEBUG__
                ContainerConfig config;
                config.FromString(instance.Configuration());

                if (config.ContainerPath.IsSet() == true) {
                    searchpaths.emplace(searchpaths.cbegin(), config.ContainerPath.Value());
                }

#endif

                Core::IteratorType<std::vector<string>, const string> searchpathsit(searchpaths);

                string volatilecallsignpath(baseConfig.VolatilePath() + instance.Callsign() + _T('/'));
                _container = admin.Container(instance.Callsign(), searchpathsit, volatilecallsignpath, instance.Configuration());
            }

            ~ContainerProcess() override
            {
                if (_container != nullptr) {
                    _container->Release();
                }
            }

            string Callsign() const override
            {
                return (_callsign);
            }

            uint32_t Launch() override
            {
                uint32_t result = Core::ERROR_GENERAL;

                if (_container != nullptr) {

                    // Note: replace below code with something more efficient when Iterators redesigned
                    Core::Process::Options::Iterator it(_process.Options());

                    std::vector<string> params;
                    while (it.Next() == true) {
                        params.emplace_back(it.Current());
                    }

                    Core::IteratorType<std::vector<string>, const string> temp(params);
                    if (_container->Start(_process.Command(), temp) == true) {
                        result = Core::ERROR_NONE;
                    } 
                } 

                return result;
            }
            void PostMortem() override;

        private:
            BEGIN_INTERFACE_MAP(ContainerProcess)
                INTERFACE_ENTRY(IRemoteConnection)
                INTERFACE_ENTRY(IMonitorableProcess)
            END_INTERFACE_MAP
            void Terminate() override;

            uint32_t RemoteId() const override
            {
                return _container->Pid();
            }

        private:
            ProcessContainers::IContainer* _container;
            string _callsign;
            uint32_t _id;
            Process _process;
        };

#endif

    protected:
        virtual RemoteConnection* CreateStarter(const Config& config, const Object& instance)
        {
            RemoteConnection* result = nullptr;

            if (instance.Type() == Object::HostType::LOCAL) {
                result = Core::Service<LocalProcess>::Create<RemoteConnection>(config, instance);
            }
            else if (instance.Type() == Object::HostType::CONTAINER) {
#ifdef PROCESSCONTAINERS_ENABLED
                result = Core::Service<ContainerProcess>::Create<RemoteConnection>(config, instance);
#else
                SYSLOG(Trace::Error, (_T("Cannot create Container process for %s, this version was not build with Container support"), instance.ClassName().c_str()));
#endif
            }

            return result;
        }

    private:
        class EXTERNAL RemoteConnectionMap {
        private:
            RemoteConnectionMap(const RemoteConnectionMap&) = delete;
            RemoteConnectionMap& operator=(const RemoteConnectionMap&) = delete;

            class Info {
            public:
                Info () = delete;
                Info (const Info&) = delete;
                Info (Core::Event& event, const uint32_t id) 
                    : _event(event) 
                    , _id(id)
                    , _interface(nullptr) {
                }
                ~Info() {
                }

            public:
                inline void Implementation(const Core::ProxyType<Core::IPCChannel>& channel, const instance_id& implementation) {

                    ASSERT (_interface == nullptr);

                    // Get the interface pointer that was stored during the triggering of the event...
                    // It is reference counted so it has to be dereferenced by the caller.
                    RPC::Administrator::Instance().ProxyInstance(channel, implementation, true, _id, _interface);

                    _event.SetEvent();
                }
                inline void* Interface() {
                    return (_interface);
                }

            private:
                Core::Event& _event;
                uint32_t _id;
                void* _interface;
            };

        public:
            RemoteConnectionMap(Communicator& parent)
                : _adminLock()
                , _announcements()
                , _connections()
                , _parent(parent)
            {
            }
            virtual ~RemoteConnectionMap()
            {
                // All observers should have unregistered before this map get's destroyed !!!
                ASSERT(_observers.size() == 0);

                while (_observers.size() != 0) {
                    _observers.front()->Release();
                    _observers.pop_front();
                }

                // All connections must be terminated if we end up here :-)
                ASSERT(_connections.size() == 0);

                Destroy();
            }

        public:
            inline void Register(RPC::IRemoteConnection::INotification* sink)
            {
                ASSERT(sink != nullptr);

                if (sink != nullptr) {

                    _adminLock.Lock();

                    ASSERT(std::find(_observers.begin(), _observers.end(), sink) == _observers.end());

                    sink->AddRef();
                    _observers.push_back(sink);

                    std::map<uint32_t, RemoteConnection*>::iterator index(_connections.begin());

                    // Report all Active Processes..
                    while (index != _connections.end()) {
                        if (index->second->IsOperational() == true) {
                            sink->Activated(&(*(index->second)));
                        }
                        index++;
                    }

                    _adminLock.Unlock();
                }
            }
            inline void Unregister(RPC::IRemoteConnection::INotification* sink)
            {
                ASSERT(sink != nullptr);

                if (sink != nullptr) {

                    _adminLock.Lock();

                    std::list<RPC::IRemoteConnection::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), sink));

                    ASSERT(index != _observers.end());

                    if (index != _observers.end()) {
                        (*index)->Release();
                        _observers.erase(index);
                    }

                    _adminLock.Unlock();
                }
            }
            inline void* Create(uint32_t& id, const Object& instance, const Config& config, const uint32_t waitTime)
            {
                void* interfaceReturned = nullptr;

                id = 0;

                _adminLock.Lock();

                RemoteConnection* result = _parent.CreateStarter(config, instance);

                ASSERT(result != nullptr);

                if (result != nullptr) {

                    Core::Event trigger(false, true);

                    // A reference for putting it in the list...
                    result->AddRef();

                    // We expect an announce interface message now...
                    _connections.insert(std::pair<uint32_t, RemoteConnection*>(result->Id(), result));
                    auto locator = _announcements.emplace(std::piecewise_construct,
                        std::forward_as_tuple(result->Id()),
                        std::forward_as_tuple(trigger, instance.Interface()));

                    _adminLock.Unlock();

                    // Start the process, and....
                    result->Launch();

                    // wait for the announce message to be exchanged
                    if (trigger.Lock(waitTime) == Core::ERROR_NONE) {

                        interfaceReturned = locator.first->second.Interface();

                        if (interfaceReturned != nullptr) {

                            id = result->Id();
                        }

                    } else {

                        // Seems we could not start the application. Cleanout
                        result->Terminate();
                    }

                    _adminLock.Lock();

                    // Kill the Event registration. We are no longer interested in what will be hapening..
                    _announcements.erase(locator.first);

                    result->Release();
                }
                _adminLock.Unlock();

                return (interfaceReturned);
            }
            inline void Closed(const uint32_t id)
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteConnection*>::iterator index(_connections.find(id));

                if (index == _connections.end()) {

                    _adminLock.Unlock();

                } else {
                    Communicator::RemoteConnection* connection = index->second;
                    connection->AddRef();

                    // Remove any channel associated, we had.
                    Core::ProxyType<Core::IPCChannel> destructed = index->second->Channel();
                    index->second->Close();

                    std::list<RPC::IRemoteConnection::INotification*>::iterator observer(_observers.begin());

                    while (observer != _observers.end()) {
                        (*observer)->Deactivated(index->second);
                        observer++;
                    }

                    // Don't forget to close on our side as well, if it is not already closed....
                    index->second->Terminate();

                    // Release this entry, do not wait till it get's overwritten.
                    index->second->Release();
                    _connections.erase(index);
                    _adminLock.Unlock();

                    _parent.Closed(destructed);

                    connection->Release();
                }
            }
            inline Communicator::RemoteConnection* Connection(const uint32_t id)
            {
                Communicator::RemoteConnection* result = nullptr;

                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteConnection*>::iterator index(_connections.find(id));

                if (index != _connections.end()) {
                    result = index->second;
                    result->AddRef();
                }

                _adminLock.Unlock();

                return (result);
            }
            inline void Destroy()
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                while (_connections.size() > 0) {
                    TRACE_L1("Forcefully closing open RPC Server connection: %d", _connections.begin()->second->Id());
                    _connections.begin()->second->Terminate();
                    _connections.begin()->second->Release();
                    _connections.erase(_connections.begin());
                }

                _adminLock.Unlock();
            }
            void* Announce(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const Data::Init& info)
            {
                void* result = nullptr;

                _adminLock.Lock();

                if (info.IsRequested() == true) {

                    Request(channel, info);
                } else {

                    if (channel->Extension().IsRegistered() == false) {

                        // This is an announce message from a process that wasn't created by us. So typically this is
                        // An RPC client reaching out to an RPC server. The RPCServer does not spawn processes it just
                        // listens for clients requesting service.
                        Communicator::RemoteConnection* remoteConnection = Core::Service<RemoteConnection>::Create<RemoteConnection>(channel, info.Id());

                        channel->Extension().Link(*this, remoteConnection->Id());
                        ASSERT(remoteConnection != nullptr);

                        // Add ref is done during the creation, no need to take another reference unless we also would release it after
                        // insertion :-)
                        _connections.insert(std::pair<uint32_t, Communicator::RemoteConnection*>(remoteConnection->Id(), remoteConnection));

                        Activated(remoteConnection);
                    }

                    result = Handle(channel, info);
                }

                _adminLock.Unlock();

                return (result);
            }

        private:
            void Activated(RPC::IRemoteConnection* connection)
            {
                std::list<RPC::IRemoteConnection::INotification*>::iterator index(_observers.begin());
                while (index != _observers.end()) {
                    (*index)->Activated(connection);
                    index++;
                }
            }
            void Request(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const Data::Init& info)
            {
                std::map<uint32_t, Communicator::RemoteConnection*>::iterator index(_connections.find(info.ExchangeId()));

                ASSERT(index != _connections.end());
                ASSERT(index->second->IsOperational() == false);

                // This is when we requested this interface/object to be created, there must be already an
                // administration, it is just not complete.... yet!!!!
                index->second->Open(channel, info.Id());
                channel->Extension().Link(*this, index->second->Id());

                Activated(index->second);

                auto processConnection = _announcements.find(index->second->Id());

                if (processConnection != _announcements.end()) {

                    Core::ProxyType<Core::IPCChannel> baseChannel(channel);

                    ASSERT(info.Implementation());

                    processConnection->second.Implementation(baseChannel, info.Implementation());
                } else {
                    // No one picks it up, release it..
                    // TODO: Release an object that will never be used...
                }
            }

            void* Handle(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const Data::Init& info)
            {
                Core::ProxyType<Core::IPCChannel> baseChannel(channel);

                instance_id implementation = info.Implementation();
                void* realIF = nullptr;
                void* result = nullptr;

                ASSERT(baseChannel.IsValid() == true);

                if (info.IsOffer() == true) {

                    ASSERT(implementation);

                    ProxyStub::UnknownProxy* base = Administrator::Instance().ProxyInstance(baseChannel, implementation, true, info.InterfaceId(), realIF);

                    if (base != nullptr) {
                        Core::IUnknown* realIFbase = base->Parent();
                        ASSERT(realIFbase != nullptr);
                        _parent.Offer(realIFbase, info.InterfaceId());
                    }

                } else if (info.IsRevoke() == true) {

                    ASSERT(implementation);

                    ProxyStub::UnknownProxy* base = Administrator::Instance().ProxyFind(baseChannel, implementation, info.InterfaceId(), realIF);

                    if (base != nullptr) {
                        Core::IUnknown* realIFbase = base->Parent();
                        _parent.Revoke(realIFbase, info.InterfaceId());
                    }

                } else if (info.InterfaceId() != static_cast<uint32_t>(~0)) {

                    // See if we have something we can return right away, if it has been requested..
                    result = _parent.Aquire(info.ClassName(), info.InterfaceId(), info.VersionId());

                    if (result != nullptr) {
                        Administrator::Instance().RegisterInterface(baseChannel, result, info.InterfaceId());
                    }
                }

                return (result);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<uint32_t, Info> _announcements;
            std::map<uint32_t, RemoteConnection*> _connections;
            std::list<RPC::IRemoteConnection::INotification*> _observers;
            Communicator& _parent;
        };

    protected:
        class EXTERNAL ChannelLink {
        private:
            ChannelLink() = delete;
            ChannelLink(const ChannelLink&) = delete;
            ChannelLink& operator=(const ChannelLink&) = delete;

        public:
            ChannelLink(Core::IPCChannelType<Core::SocketPort, ChannelLink>* channel)
                : _channel(channel->Source())
                , _connectionMap(nullptr)
                , _id(0)
            {
                // We are a composit of the Channel, no need (and do not for cyclic references) not maintain a reference...
                ASSERT(channel != nullptr);
            }
            ~ChannelLink()
            {
            }

        public:
            void Link(RemoteConnectionMap& connectionMap, const uint32_t id)
            {
                _connectionMap = &connectionMap;
                _id = id;
            }
            void StateChange()
            {
                // If the connection closes, we need to clean up....
                if ((_channel.IsOpen() == false) && (_connectionMap != nullptr)) {
                    _connectionMap->Closed(_id);
                }
            }
            bool IsRegistered() const
            {
                return (_connectionMap != nullptr);
            }
            const uint32_t Id() const
            {
                return _id;
            }

        private:
            // Non ref-counted reference to our parent, of which we are a composit :-)
            Core::SocketPort& _channel;
            RemoteConnectionMap* _connectionMap;
            uint32_t _id;
        };

    private:
        class EXTERNAL ChannelServer : public Core::IPCChannelServerType<ChannelLink, true> {
        private:
            ChannelServer(const ChannelServer&) = delete;
            ChannelServer& operator=(const ChannelServer&) = delete;

            typedef Core::IPCChannelServerType<ChannelLink, true> BaseClass;
            typedef Core::IPCChannelType<Core::SocketPort, ChannelLink> Client;

            class EXTERNAL AnnounceHandlerImplementation : public Core::IIPCServer {
            private:
                AnnounceHandlerImplementation() = delete;
                AnnounceHandlerImplementation(const AnnounceHandlerImplementation&) = delete;
                AnnounceHandlerImplementation& operator=(const AnnounceHandlerImplementation&) = delete;

            public:
                AnnounceHandlerImplementation(ChannelServer* parent)
                    : _parent(*parent)
                {

                    ASSERT(parent != nullptr);
                }

                virtual ~AnnounceHandlerImplementation()
                {
                }

            public:
                virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
                {
                    Core::ProxyType<AnnounceMessage> message(Core::proxy_cast<AnnounceMessage>(data));

                    ASSERT(message.IsValid() == true);
                    ASSERT(dynamic_cast<Client*>(&channel) != nullptr);

                    Core::ProxyType<Client> proxyChannel(static_cast<Client&>(channel));

                    // Anounce the interface as completed
                    string jsonDefaultCategories(Trace::TraceUnit::Instance().Defaults());
                    void* result = _parent.Announce(proxyChannel, message->Parameters());

                    message->Response().Set(instance_cast<void*>(result), proxyChannel->Extension().Id(), _parent.ProxyStubPath(), jsonDefaultCategories);

                    // We are done, report completion
                    channel.ReportResponse(data);
                }

            private:
                ChannelServer& _parent;
            };
            class EXTERNAL InvokeHandlerImplementation : public Core::IIPCServer {
            private:
                InvokeHandlerImplementation(const InvokeHandlerImplementation&) = delete;
                InvokeHandlerImplementation& operator=(const InvokeHandlerImplementation&) = delete;

            public:
                InvokeHandlerImplementation()
                {
                }
                virtual ~InvokeHandlerImplementation()
                {
                }

            public:
                virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
                {
                    Core::ProxyType<Core::IPCChannel> proxyChannel(channel);
                    Job::Invoke(proxyChannel, data);
                }
            };

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            ChannelServer(
                const Core::NodeId& remoteNode,
                RemoteConnectionMap& processes,
                const string& proxyStubPath)
                : BaseClass(remoteNode, CommunicationBufferSize)
                , _proxyStubPath(proxyStubPath)
                , _connections(processes)
                , _announceHandler(this)
            {
                BaseClass::Register(InvokeMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<InvokeHandlerImplementation>::Create()));
                BaseClass::Register(AnnounceMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<AnnounceHandlerImplementation>::Create(this)));
            }
            ChannelServer(
                const Core::NodeId& remoteNode,
                RemoteConnectionMap& processes,
                const string& proxyStubPath,
                const Core::ProxyType<Core::IIPCServer>& handler)
                : BaseClass(remoteNode, CommunicationBufferSize)
                , _proxyStubPath(proxyStubPath)
                , _connections(processes)
                , _announceHandler(this)
            {
                BaseClass::Register(InvokeMessage::Id(), handler);
                BaseClass::Register(AnnounceMessage::Id(), handler);
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

            ~ChannelServer()
            {
                BaseClass::Unregister(AnnounceMessage::Id());
                BaseClass::Unregister(InvokeMessage::Id());
            }

        public:
            inline const string& ProxyStubPath() const
            {
                return (_proxyStubPath);
            }
            inline Core::IIPCServer* Announcement()
            {
                return (&_announceHandler);
            }

        private:
            inline void* Announce(Core::ProxyType<Client>& channel, const Data::Init& info)
            {
                // We are in business, register the process with this channel.
                return (_connections.Announce(channel, info));
            }

        private:
            const string _proxyStubPath;
            RemoteConnectionMap& _connections;
            AnnounceHandlerImplementation _announceHandler;
        };

    private:
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator& operator=(const Communicator&) = delete;

    public:
        Communicator(
            const Core::NodeId& node,
            const string& proxyStubPath);
        Communicator(
            const Core::NodeId& node,
            const string& proxyStubPath,
            const Core::ProxyType<Core::IIPCServer>& handler);
        virtual ~Communicator();

    public:
        inline Core::IIPCServer* Announcement()
        {
            return (_ipcServer.Announcement());
        }
        inline bool IsListening() const
        {
            return (_ipcServer.IsListening());
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_ipcServer.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_ipcServer.Close(waitTime));
        }
        inline const string& Connector() const
        {
            return (_ipcServer.Connector());
        }
        inline void Register(RPC::IRemoteConnection::INotification* sink)
        {
            _connectionMap.Register(sink);
        }
        inline void Unregister(RPC::IRemoteConnection::INotification* sink)
        {
            _connectionMap.Unregister(sink);
        }
        inline IRemoteConnection* Connection(const uint32_t id)
        {
            return (_connectionMap.Connection(id));
        }
        inline void* Create(uint32_t& pid, const Object& instance, const Config& config, const uint32_t waitTime)
        {
            return (_connectionMap.Create(pid, instance, config, waitTime));
        }
        void Destroy()
        {
            _connectionMap.Destroy();
        }

    private:
        void Closed(const Core::ProxyType<Core::IPCChannel>& channel)
        {
            std::list<ProxyStub::UnknownProxy*> deadProxies;

            RPC::Administrator::Instance().DeleteChannel(channel, deadProxies);

            std::list<ProxyStub::UnknownProxy*>::const_iterator loop(deadProxies.begin());
            while (loop != deadProxies.end()) {
                Revoke((*loop)->Parent(), (*loop)->InterfaceId());
                // To avoid race conditions, the creation of the deadProxies took a reference
                // on the interfaces, we presented here. Do not forget to release this reference.
                (*loop)->Release();
                loop++;
            }
        }
        virtual void* Aquire(const string& /* className */, const uint32_t /* interfaceId */, const uint32_t /* version */)
        {
            return (nullptr);
        }
        virtual void Offer(Core::IUnknown* /* remote */, const uint32_t /* interfaceId */)
        {
        }
        // note: do NOT do a QueryInterface on the IUnknown pointer (or any other method for that matter), the object it points to might already be destroyed
        virtual void Revoke(const Core::IUnknown* /* remote */, const uint32_t /* interfaceId */)
        {
        }
        virtual void Cleanup(const Core::IUnknown* /* source */, const uint32_t /* interfaceid */)
        {
        }

    private:
        RemoteConnectionMap _connectionMap;
        ChannelServer _ipcServer;
    };

    class EXTERNAL CommunicatorClient : public Core::IPCChannelClientType<Core::Void, false, true>, public Core::IDispatchType<Core::IIPC> {
    private:
        typedef Core::IPCChannelClientType<Core::Void, false, true> BaseClass;

        class EXTERNAL AnnounceHandlerImplementation : public Core::IIPCServer {
        public:
            AnnounceHandlerImplementation() = delete;
            AnnounceHandlerImplementation(const AnnounceHandlerImplementation&) = delete;
            AnnounceHandlerImplementation& operator=(const AnnounceHandlerImplementation&) = delete;

            AnnounceHandlerImplementation(CommunicatorClient* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~AnnounceHandlerImplementation()
            {
            }

        public:
            void Procedure(IPCChannel& channel, Core::ProxyType<Core::IIPC>& data) override
            {
                // Oke, see if we can reference count the IPCChannel
                Core::ProxyType<Core::IPCChannel> refChannel(channel);
                Core::ProxyType<RPC::AnnounceMessage> message(data);

                ASSERT(refChannel.IsValid());

                const string className(message->Parameters().ClassName());
                const uint32_t interfaceId(message->Parameters().InterfaceId());
                const uint32_t versionId(message->Parameters().VersionId());

                instance_id implementation = instance_cast<void*>(_parent.Aquire(className, interfaceId, versionId));
                message->Response().Implementation(implementation);

                channel.ReportResponse(data);
            }

        private:
            CommunicatorClient& _parent;
        };
        class EXTERNAL InvokeHandlerImplementation : public Core::IIPCServer {
        public:
            InvokeHandlerImplementation(const InvokeHandlerImplementation&) = delete;
            InvokeHandlerImplementation& operator=(const InvokeHandlerImplementation&) = delete;

            InvokeHandlerImplementation()
            {
            }
            virtual ~InvokeHandlerImplementation()
            {
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<Core::IIPC>& data)
            {
                Core::ProxyType<Core::IPCChannel> proxyChannel(channel);
                Job::Invoke(proxyChannel, data);
            }
        };

    public:
        CommunicatorClient() = delete;
        CommunicatorClient(const CommunicatorClient&) = delete;
        CommunicatorClient& operator=(const CommunicatorClient&) = delete;

        CommunicatorClient(
            const Core::NodeId& remoteNode);
        CommunicatorClient(
            const Core::NodeId& remoteNode,
            const Core::ProxyType<Core::IIPCServer>& handler);
        ~CommunicatorClient();

    public:
        inline Core::IIPCServer* Announcement()
        {
            return (&_handler);
        }

        inline uint32_t ConnectionId() const
        {
            return _connectionId;
        }

        // Open a communication channel with this process, no need for an initial exchange
        uint32_t Open(const uint32_t waitTime);

        template <typename INTERFACE>
        inline INTERFACE* Open(const string& className, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            INTERFACE* result = nullptr;

            if (Open(waitTime, className, INTERFACE::ID, version) == Core::ERROR_NONE) {
                // Oke we could open the channel, lets get the interface
                result = WaitForCompletion<INTERFACE>(waitTime);
            }

            return (result);
        }

        // Open and offer the requested interface (Applicable if the WPEProcess starts the RPCClient)
        uint32_t Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation, const uint32_t exchangeId);

        template <typename INTERFACE>
        INTERFACE* Aquire(const uint32_t waitTime, const string& className, const uint32_t versionId)
        {
            INTERFACE* result(nullptr);

            ASSERT(className.empty() == false);

            if (BaseClass::IsOpen() == true) {

                _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), className, INTERFACE::ID, versionId);

                // Lock event until Dispatch() sets it.
                if (BaseClass::Invoke(_announceMessage, waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() == 0);

                    instance_id implementation(_announceMessage->Response().Implementation());

                    if (implementation) {
                        Core::ProxyType<Core::IPCChannel> baseChannel(*this);

                        ASSERT(baseChannel.IsValid() == true);

                        Administrator::Instance().ProxyInstance(baseChannel, implementation, true, result);
                    }
                }
            }

            return (result);
        }
        template <typename INTERFACE>
        inline uint32_t Offer(INTERFACE* offer, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            uint32_t result(Core::ERROR_NONE);

            if (BaseClass::IsOpen() == true) {

                _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), INTERFACE::ID, instance_cast<void*>(offer), Data::Init::OFFER);

                BaseClass::Invoke(_announceMessage, waitTime);

                // Lock event until Dispatch() sets it.
                if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() != 0);

                } else {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }

            return (result);
        }
        template <typename INTERFACE>
        inline uint32_t Revoke(INTERFACE* offer, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            uint32_t result(Core::ERROR_NONE);

            if (BaseClass::IsOpen() == true) {

                _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), INTERFACE::ID, instance_cast<void*>(offer), Data::Init::REVOKE);

                BaseClass::Invoke(_announceMessage, waitTime);

                // Lock event until Dispatch() sets it.
                if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() != 0);
                } else {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }

            return (result);
        }

        uint32_t Close(const uint32_t waitTime);

        virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
        {
            Core::Library emptyLibrary;
            // Allright, respond with the interface.
            void* result = Core::ServiceAdministrator::Instance().Instantiate(emptyLibrary, className.c_str(), versionId, interfaceId);

            if (result != nullptr) {
                Core::ProxyType<Core::IPCChannel> baseChannel(*this);
                Administrator::Instance().RegisterInterface(baseChannel, result, interfaceId);
            }

            return (result);
        }

    private:
        // Open and request an interface from the other side on the announce message (Any RPC client uses this)
        uint32_t Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version);

        // If the Open, with a request was made, this method waits for the requested interface.
        template <typename INTERFACE>
        inline INTERFACE* WaitForCompletion(const uint32_t waitTime)
        {
            INTERFACE* result = nullptr;

            ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
            ASSERT(_announceMessage->Parameters().Implementation() == 0);

            // Lock event until Dispatch() sets it.
            if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                instance_id implementation(_announceMessage->Response().Implementation());

                if (implementation) {
                    Core::ProxyType<Core::IPCChannel> baseChannel(*this);

                    ASSERT(baseChannel.IsValid() == true);

                    Administrator::Instance().ProxyInstance(baseChannel, implementation, true, result);
                }
            }

            return (result);
        }
        inline bool WaitForCompletion(const uint32_t waitTime)
        {
            // Lock event until Dispatch() sets it.
            return (_announceEvent.Lock(waitTime) == Core::ERROR_NONE);
        }
        virtual void Dispatch(Core::IIPC& element);

    protected:
        virtual void StateChange();

    private:
        Core::ProxyType<RPC::AnnounceMessage> _announceMessage;
        Core::Event _announceEvent;
        AnnounceHandlerImplementation _handler;
        uint32_t _connectionId;
    };
}
}

#endif // __COM_PROCESSLAUNCH_H
