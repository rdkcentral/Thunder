#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include "Administrator.h"
#include "ITracing.h"
#include "IUnknown.h"
#include "Module.h"

#ifdef PROCESSCONTAINERS_ENABLED 
    #include <ProcessContainer.h>
#endif

#include "../tracing/TraceUnit.h"

namespace WPEFramework {
namespace RPC {

    class EXTERNAL Object {
    public:
        Object()
            : _locator()
            , _className()
            , _interface(~0)
            , _version(~0)
            , _user()
            , _group()
            , _threads()
        {
        }
        Object(const Object& copy)
            : _locator(copy._locator)
            , _className(copy._className)
            , _interface(copy._interface)
            , _version(copy._version)
            , _user(copy._user)
            , _group(copy._group)
            , _threads(copy._threads)
        {
        }
        Object(const string& locator, const string& className, const uint32_t interface, const uint32_t version, const string& user, const string& group, const uint8_t threads)
            : _locator(locator)
            , _className(className)
            , _interface(interface)
            , _version(version)
            , _user(user)
            , _group(group)
            , _threads(threads)
        {
        }
        ~Object()
        {
        }

        Object& operator=(const Object& RHS)
        {
            _locator = RHS._locator;
            _className = RHS._className;
            _interface = RHS._interface;
            _version = RHS._version;
            _user = RHS._user;
            _group = RHS._group;
            _threads = RHS._threads;

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

    private:
        string _locator;
        string _className;
        uint32_t _interface;
        uint32_t _version;
        string _user;
        string _group;
        uint8_t _threads;
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
            , _application()
            , _proxyStub()
        {
        }
        Config(
            const string& connector,
            const string& hostApplication,
            const string& persistentPath,
            const string& systemPath,
            const string& dataPath,
            const string& applicationPath,
            const string& proxyStubPath)
            : _connector(connector)
            , _hostApplication(hostApplication)
            , _persistent(persistentPath)
            , _system(systemPath)
            , _data(dataPath)
            , _application(applicationPath)
            , _proxyStub(proxyStubPath)
        {
        }
        Config(const Config& copy)
            : _connector(copy._connector)
            , _hostApplication(copy._hostApplication)
            , _persistent(copy._persistent)
            , _system(copy._system)
            , _data(copy._data)
            , _application(copy._application)
            , _proxyStub(copy._proxyStub)
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
        inline const string& ApplicationPath() const
        {
            return (_application);
        }
        inline const string& ProxyStubPath() const
        {
            return (_proxyStub);
        }

    private:
        string _connector;
        string _hostApplication;
        string _persistent;
        string _system;
        string _data;
        string _application;
        string _proxyStub;
    };

    struct EXTERNAL IRemoteConnection : virtual public Core::IUnknown {
        enum { ID = ID_COMCONNECTION };

        virtual ~IRemoteConnection() {}

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_COMCONNECTION_NOTIFICATION };

            virtual ~INotification() {}
            virtual void Activated(IRemoteConnection* connection) = 0;
            virtual void Deactivated(IRemoteConnection* connection) = 0;
        };

        virtual uint32_t Id() const = 0;
        virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
        virtual void Terminate() = 0;

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Aquire(const uint32_t waitTime, const string& className, const uint32_t version)
        {
            void* baseInterface(Aquire(waitTime, className, REQUESTEDINTERFACE::ID, version));

            if (baseInterface != nullptr) {

                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }
    };

    class EXTERNAL Communicator {
    private:
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
            {
            }
            RemoteConnection(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel)
                : _channel(channel)
                , _id(_sequenceId++)
            {
            }

        public:
            ~RemoteConnection()
            {
            }

        public:
            virtual void* QueryInterface(const uint32_t id) override;
            virtual uint32_t Id() const override;
            virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) override;
            virtual void Terminate() override;

            inline bool IsOperational() const
            {
                return (_channel.IsValid() == true);
            }
            inline Core::ProxyType<Core::IPCChannel> Channel()
            {
                ASSERT(_channel.IsValid() == true);

                return (_channel);
            }
            void Open(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel)
            {
                ASSERT(_channel.IsValid() == false);

                // Seems we received an interface from the otherside. Prepare the actual stub around it.
                TRACE_L1("Link announced. All up and running %d, has announced itself.", Id());

                _channel = channel;
            }
            void Close()
            {
                if (_channel.IsValid() == true) {
                    _channel.Release();
                }
            }

        private:
            Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>> _channel;
            uint32_t _id;
            static std::atomic<uint32_t> _sequenceId;
        };
        class EXTERNAL RemoteProcess : public RemoteConnection {
        private:
            friend class Core::Service<RemoteProcess>;

            RemoteProcess(const RemoteProcess&) = delete;
            RemoteProcess& operator=(const RemoteProcess&) = delete;

        private:
            RemoteProcess()
                : RemoteConnection()
            {
            }

        public:
            virtual ~RemoteProcess()
            {
                TRACE_L1("Destructor for RemoteProcess process for %d", Id());
            }

        public:
            inline void Launch(const Object& instance, const Config& config)
            {
                Core::Process::Options options(config.HostApplication());
                uint32_t loggingSettings = (Logging::LoggingType<Logging::Startup>::IsEnabled()      ? 0x01 : 0) | 
					                       (Logging::LoggingType<Logging::Shutdown>::IsEnabled()     ? 0x02 : 0) | 
					                       (Logging::LoggingType<Logging::Notification>::IsEnabled() ? 0x04 : 0);

                ASSERT(instance.Locator().empty() == false);
                ASSERT(instance.ClassName().empty() == false);
                ASSERT(config.Connector().empty() == false);

                options[_T("-l")] = instance.Locator();
                options[_T("-c")] = instance.ClassName();
                options[_T("-r")] = config.Connector();
                options[_T("-i")] = Core::NumberType<uint32_t>(instance.Interface()).Text();
                options[_T("-e")] = Core::NumberType<uint32_t>(loggingSettings).Text();
                options[_T("-x")] = Core::NumberType<uint32_t>(Id()).Text();

                if (instance.Version() != static_cast<uint32_t>(~0)) {
                    options[_T("-v")] = Core::NumberType<uint32_t>(instance.Version()).Text();
                }
                if (instance.User().empty() == false) {
                    options[_T("-u")] = instance.User();
                }
                if (instance.Group().empty() == false) {
                    options[_T("-g")] = instance.Group();
                }
                if (config.PersistentPath().empty() == false) {
                    options[_T("-p")] = config.PersistentPath();
                }
                if (config.SystemPath().empty() == false) {
                    options[_T("-s")] = config.SystemPath();
                }
                if (config.DataPath().empty() == false) {
                    options[_T("-d")] = config.DataPath();
                }
                if (config.ApplicationPath().empty() == false) {
                    options[_T("-a")] = config.ApplicationPath();
                }
                if (config.ProxyStubPath().empty() == false) {
                    options[_T("-m")] = config.ProxyStubPath();
                }
                if (instance.Threads() > 1) {
                    options[_T("-t")] = Core::NumberType<uint8_t>(instance.Threads()).Text();
                }
                // Start the external process launch..

                DoLaunch();
            }

            virtual DoLaunch() = 0;

            virtual void Terminate() override;

        private:
            uint32_t _id;
        };

        class EXTERNAL RemoteProcess : public RemoteConnection {
        private:
            friend class Core::Service<RemoteProcess>;

            RemoteProcess(const RemoteProcess&) = delete;
            RemoteProcess& operator=(const RemoteProcess&) = delete;

        private:
            RemoteProcess()
                : RemoteConnection()
            {
            }

        public:
            virtual ~RemoteProcess()
            {
                TRACE_L1("Destructor for RemoteProcess process for %d", Id());
            }

        public:
            inline void Launch(const Object& instance, const Config& config)
            {
                Core::Process::Options options(config.HostApplication());
                uint32_t loggingSettings = (Trace::TraceType<Logging::Startup, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x01 : 0) | (Trace::TraceType<Logging::Shutdown, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x02 : 0) | (Trace::TraceType<Logging::Notification, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x04 : 0);

                ASSERT(instance.Locator().empty() == false);
                ASSERT(instance.ClassName().empty() == false);
                ASSERT(config.Connector().empty() == false);

                options[_T("-l")] = instance.Locator();
                options[_T("-c")] = instance.ClassName();
                options[_T("-r")] = config.Connector();
                options[_T("-i")] = Core::NumberType<uint32_t>(instance.Interface()).Text();
                options[_T("-e")] = Core::NumberType<uint32_t>(loggingSettings).Text();
                options[_T("-x")] = Core::NumberType<uint32_t>(Id()).Text();

                if (instance.Version() != static_cast<uint32_t>(~0)) {
                    options[_T("-v")] = Core::NumberType<uint32_t>(instance.Version()).Text();
                }
                if (instance.User().empty() == false) {
                    options[_T("-u")] = instance.User();
                }
                if (instance.Group().empty() == false) {
                    options[_T("-g")] = instance.Group();
                }
                if (config.PersistentPath().empty() == false) {
                    options[_T("-p")] = config.PersistentPath();
                }
                if (config.SystemPath().empty() == false) {
                    options[_T("-s")] = config.SystemPath();
                }
                if (config.DataPath().empty() == false) {
                    options[_T("-d")] = config.DataPath();
                }
                if (config.ApplicationPath().empty() == false) {
                    options[_T("-a")] = config.ApplicationPath();
                }
                if (config.ProxyStubPath().empty() == false) {
                    options[_T("-m")] = config.ProxyStubPath();
                }
                if (instance.Threads() > 1) {
                    options[_T("-t")] = Core::NumberType<uint8_t>(instance.Threads()).Text();
                }
                // Start the external process launch..
                Core::Process fork(false);

                fork.Launch(options, &_id);
            }

            virtual void Terminate() override;

        private:
            uint32_t _id;
        };

/*

        class EXTERNAL ContainerRemoteProcess : public RemoteConnection {
        private:
            friend class Core::Service<ContainerRemoteProcess>;

            ContainerRemoteProcess() = delete;
            ContainerRemoteProcess(const MasterRemoteProcess&) = delete;
            ContainerRemoteProcess& operator=(const ContainerRemoteProcess&) = delete;

        protected:
            ContainerRemoteProcess(const string& configuration)
                : RemoteProcess(parent)
                , _container(nullptr)
            {
                ProcessContainers::IContainerAdministrator& admin = ProcessContainers::IContainerAdministrator::Instance();

                _container = admin.Container("webserver");

                if( _container != nullptr ) {
                    _container->Start();
                }
            }

        public:

            

            inline static void ContainerInitialization() {
                ProcessContainers::IContainerAdministrator& admin = ProcessContainers::IContainerAdministrator::Instance();
                admin.ContainerDefinitionSearchPaths({string("/home/marcelf/.local/share/lxc/")}); //todo: replace by correct default searchpaths
            }
            inline static RemoteProcess* Create(RemoteProcessMap& parent, uint32_t& pid, const Object& instance, const Config& config)
            {

                uint32_t loggingSettings = (Trace::TraceType<Logging::Startup, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x01 : 0) | (Trace::TraceType<Logging::Shutdown, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x02 : 0) | (Trace::TraceType<Logging::Notification, &Logging::MODULE_LOGGING>::IsEnabled() ? 0x04 : 0);

                Core::Process::Options options(config.HostApplication());

                ASSERT(instance.Locator().empty() == false);
                ASSERT(instance.ClassName().empty() == false);
                ASSERT(config.Connector().empty() == false);

                options[_T("-l")] = instance.Locator();
                options[_T("-c")] = instance.ClassName();
                options[_T("-r")] = config.Connector();
                options[_T("-i")] = Core::NumberType<uint32_t>(instance.Interface()).Text();
                options[_T("-e")] = Core::NumberType<uint32_t>(loggingSettings).Text();
                if (instance.Version() != static_cast<uint32_t>(~0)) {
                    options[_T("-v")] = Core::NumberType<uint32_t>(instance.Version()).Text();
                }
                if (instance.User().empty() == false) {
                    options[_T("-u")] = instance.User();
                }
                if (instance.Group().empty() == false) {
                    options[_T("-g")] = instance.Group();
                }
                if (config.PersistentPath().empty() == false) {
                    options[_T("-p")] = config.PersistentPath();
                }
                if (config.SystemPath().empty() == false) {
                    options[_T("-s")] = config.SystemPath();
                }
                if (config.DataPath().empty() == false) {
                    options[_T("-d")] = config.DataPath();
                }
                if (config.ApplicationPath().empty() == false) {
                    options[_T("-a")] = config.ApplicationPath();
                }
                if (config.ProxyStubPath().empty() == false) {
                    options[_T("-m")] = config.ProxyStubPath();
                }
                if (instance.Threads() > 1) {
                    options[_T("-t")] = Core::NumberType<uint8_t>(instance.Threads()).Text();
                }

                return (Core::Service<ContainerRemoteProcess>::Create<ContainerRemoteProcess>(&parent, &pid, &options, string()));
            }
            ~ContainerRemoteProcess()
            {
                if( _container != nullptr ) {
                    _container->Release();
                }

                TRACE_L1("Destructor for ContainerRemoteProcess process for %d", Id());
            }

        public:
            uint32_t Id() const override
            {
                return 0;
            }
            inline bool IsActive() const
            {
                return true;
            }
            inline uint32_t ExitCode() const
            {
                return 0;
            }
            inline void Kill(const bool hardKill)
            {
            }

        private:
            ProcessContainers::IContainerAdministrator::IContainer* _container;
        };
#endif

*/



        class EXTERNAL RemoteConnectionMap {
        private:
            RemoteConnectionMap(const RemoteConnectionMap&) = delete;
            RemoteConnectionMap& operator=(const RemoteConnectionMap&) = delete;

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

                _adminLock.Lock();

<<<<<<< HEAD
                Communicator::RemoteProcess* result = nullptr;

                if( false ) {
                    result = MasterRemoteProcess::Create(*this, pid, instance, config);
                } else {
                    result = ContainerRemoteProcess::Create(*this, pid, instance, config);
                }
=======
                Communicator::RemoteProcess* result = Core::Service<RemoteProcess>::Create<RemoteProcess>();
>>>>>>> pwielders/lxc_remotehost

                ASSERT(result != nullptr);

                if (result != nullptr) {

                    Core::Event trigger(false, true);

                    // A reference for putting it in the list...
                    result->AddRef();

                    // We expect an announce interface message now...
                    _connections.insert(std::pair<uint32_t, RemoteConnection*>(result->Id(), result));
                    auto locator = _announcements.emplace(std::piecewise_construct,
                        std::forward_as_tuple(result->Id()),
                        std::forward_as_tuple(std::pair<Core::Event&, void*>(trigger, nullptr)));

                    id = result->Id();

                    _adminLock.Unlock();

                    // Start the process, and....
                    result->Launch(instance, config);

                    // wait for the announce message to be exchanged
                    if (trigger.Lock(waitTime) == Core::ERROR_NONE) {

                        uint32_t interfaceId = instance.Interface();

                        _adminLock.Lock();

                        // Get the interface pointer that was stored during the triggering of the event...
                        // It is reference counted so it has to be dereferenced by the caller.
                        ProxyStub::UnknownProxy* proxyStub = RPC::Administrator::Instance().ProxyInstance(result->Channel(), locator.first->second.second, interfaceId, true, interfaceId, false);

                        if (proxyStub != nullptr) {
                            interfaceReturned = proxyStub->QueryInterface(interfaceId);
                        }

                    } else {
                        _adminLock.Lock();

                        // Seems we could not start the application. Cleanout
                        result->Terminate();
                    }

                    // Kill the Event registration. We are no longer interested in what will be hapening..
                    _announcements.erase(locator.first);
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
                    // Remove any channel associated, we had.
                    Core::IPCChannel* destructed = index->second->Channel().operator->();
                    index->second->Close();

                    std::list<RPC::IRemoteConnection::INotification*>::iterator observer(_observers.begin());

                    while (observer != _observers.end()) {
                        (*observer)->Deactivated(index->second);
                        observer++;
                    }

                    // Release this entry, do not wait till it get's overwritten.
                    index->second->Release();
                    _connections.erase(index);
                    _adminLock.Unlock();

                    _parent.Closed(destructed);
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
                    _connections.begin()->second->Terminate();
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
                        Communicator::RemoteConnection* remoteConnection = Core::Service<RemoteConnection>::Create<RemoteConnection>(channel);

                        channel->Extension().Link(*this, remoteConnection->Id());
                        ASSERT(remoteConnection != nullptr);

                        // Add ref is done during the creation, no need to take another reference unless we also would release it after
                        // insertion :-)
                        _connections.insert(std::pair<uint32_t, Communicator::RemoteConnection*>(remoteConnection->Id(), remoteConnection));

                        Activated(remoteConnection);
                    }

                    Handle(channel, info);
                }

                _adminLock.Unlock();

                return (result);
            }
            void Activated(RPC::IRemoteConnection* connection)
            {
                _adminLock.Lock();

                std::list<RPC::IRemoteConnection::INotification*>::iterator index(_observers.begin());

                while (index != _observers.end()) {
                    (*index)->Activated(connection);
                    index++;
                }

                _adminLock.Unlock();
            }

        private:
            void Request(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const Data::Init& info)
            {
                void* result = info.Implementation();

                ASSERT(result != nullptr);

                std::map<uint32_t, Communicator::RemoteConnection*>::iterator index(_connections.find(info.ExchangeId()));

                ASSERT(index != _connections.end());
                ASSERT(index->second->IsOperational() == false)

                // This is when we requested this interface/object to be created, there must be already an
                // administration, it is just not complete.... yet!!!!
                index->second->Open(channel);
                channel->Extension().Link(*this, index->second->Id());

                Activated(index->second);

                auto processConnection = _announcements.find(index->second->Id());

                if (processConnection != _announcements.end()) {
                    processConnection->second.second = result;
                    processConnection->second.first.SetEvent();
                } else {
                    // No one picks it up, release it..
                    // TODO: Release an object that will never be used...
                }
            }

            void* Handle(Core::ProxyType<Core::IPCChannelType<Core::SocketPort, ChannelLink>>& channel, const Data::Init& info)
            {
                void* result = info.Implementation();

                if (info.IsOffer() == true) {

                    ASSERT(result != nullptr);

                    Core::IUnknown* baseIUnknown = Administrator::Instance().ProxyInstance<Core::IUnknown>(channel, result, info.InterfaceId(), info.IsRequested());

                    if (baseIUnknown != nullptr) {
                        _parent.Offer(baseIUnknown, info.InterfaceId());
                        baseIUnknown->Release();
                    }
                    result = nullptr;

                } else if (info.IsRevoke() == true) {

                    ASSERT(result != nullptr);

                    Core::IUnknown* baseIUnknown = Administrator::Instance().ProxyFind<Core::IUnknown>(channel, result, info.InterfaceId());
                    if (baseIUnknown != nullptr) {
                        _parent.Revoke(baseIUnknown, info.InterfaceId());
                        baseIUnknown->Release();
                    }
                    result = nullptr;

                } else if (info.InterfaceId() != static_cast<uint32_t>(~0)) {

                    ASSERT(result == nullptr);

                    // See if we have something we can return right away, if it has been requested..
                    result = _parent.Aquire(info.ClassName(), info.InterfaceId(), info.VersionId());

                    if (result != nullptr) {
                        Core::ProxyType<Core::IPCChannel> baseChannel(channel);
                        Administrator::Instance().RegisterInterface(baseChannel, result, info.InterfaceId());
                    }
                }

                return (result);
            }

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<uint32_t, std::pair<Core::Event&, void*>> _announcements;
            std::map<uint32_t, Communicator::RemoteConnection*> _connections;
            std::list<RPC::IRemoteConnection::INotification*> _observers;
            Communicator& _parent;
        };
        class EXTERNAL ChannelLink {
        private:
            ChannelLink() = delete;
            ChannelLink(const ChannelLink&) = delete;
            ChannelLink& operator=(const ChannelLink&) = delete;

        public:
            ChannelLink(Core::IPCChannelType<Core::SocketPort, ChannelLink>* channel)
                : _channel(channel->Source())
                , _connectionMap(nullptr)
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

        private:
            // Non ref-counted reference to our parent, of which we are a composit :-)
            Core::SocketPort& _channel;
            RemoteConnectionMap* _connectionMap;
            uint32_t _id;
        };
        class EXTERNAL ChannelServer : public Core::IPCChannelServerType<ChannelLink, true> {
        private:
            ChannelServer(const ChannelServer&) = delete;
            ChannelServer& operator=(const ChannelServer&) = delete;

            typedef Core::IPCChannelServerType<ChannelLink, true> BaseClass;
            typedef Core::IPCChannelType<Core::SocketPort, ChannelLink> Client;

            class EXTERNAL InterfaceAnnounceHandler : public Core::IPCServerType<AnnounceMessage> {
            private:
                InterfaceAnnounceHandler() = delete;
                InterfaceAnnounceHandler(const InterfaceAnnounceHandler&) = delete;
                InterfaceAnnounceHandler& operator=(const InterfaceAnnounceHandler&) = delete;

            public:
                InterfaceAnnounceHandler(ChannelServer* parent)
                    : _parent(*parent)
                {

                    ASSERT(parent != nullptr);
                }

                virtual ~InterfaceAnnounceHandler()
                {
                }

            public:
                virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data) override
                {
                    // Anounce the interface as completed
                    string jsonDefaultCategories;
                    Trace::TraceUnit::Instance().GetDefaultCategoriesJson(jsonDefaultCategories);
                    void* result = _parent.Announce(channel, data->Parameters());

                    data->Response().Set(result, _parent.ProxyStubPath(), jsonDefaultCategories);

                    // We are done, report completion
                    Core::ProxyType<Core::IIPC> baseMessage(Core::proxy_cast<Core::IIPC>(data));
                    channel.ReportResponse(baseMessage);
                }

            private:
                ChannelServer& _parent;
            };

        public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            ChannelServer(const Core::NodeId& remoteNode, RemoteConnectionMap& processes, Core::ProxyType<IHandler>& handler, const string& proxyStubPath)
                : BaseClass(remoteNode, CommunicationBufferSize)
                , _proxyStubPath(proxyStubPath)
                , _connections(processes)
                , _interfaceAnnounceHandler(this)
                , _handler(handler)
            {
                _handler->AnnounceHandler(&_interfaceAnnounceHandler);
                BaseClass::Register(_handler->InvokeHandler());
                BaseClass::Register(_handler->AnnounceHandler());
            }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

            ~ChannelServer()
            {
                BaseClass::Unregister(_handler->InvokeHandler());
                BaseClass::Unregister(_handler->AnnounceHandler());
                _handler->AnnounceHandler(nullptr);
            }

        public:
            inline const string& ProxyStubPath() const
            {
                return (_proxyStubPath);
            }

        private:
            inline void* Announce(Core::IPCChannel& channel, const Data::Init& info)
            {
                // We are in business, register the process with this channel.
                ASSERT(dynamic_cast<Client*>(&channel) != nullptr);
                Core::ProxyType<Client> proxyChannel(static_cast<Client&>(channel));
                return (_connections.Announce(proxyChannel, info));
            }

        private:
            const string _proxyStubPath;
            RemoteConnectionMap& _connections;
            InterfaceAnnounceHandler _interfaceAnnounceHandler;
            Core::ProxyType<IHandler> _handler;
        };

    private:
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator& operator=(const Communicator&) = delete;

    public:
        Communicator(const Core::NodeId& node, Core::ProxyType<IHandler> handler, const string& proxyStubPath);
        virtual ~Communicator();

    public:
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
        template <typename ACTUALELEMENT>
        inline void CreateFactory(const uint32_t initialSize)
        {
            _ipcServer.CreateFactory<ACTUALELEMENT>(initialSize);
        }
        template <typename ACTUALELEMENT>
        inline void DestroyFactory()
        {
            _ipcServer.DestroyFactory<ACTUALELEMENT>();
        }
        inline void Register(const Core::ProxyType<Core::IIPCServer>& handler)
        {
            _ipcServer.Register(handler);
        }
        inline void Unregister(const Core::ProxyType<Core::IIPCServer>& handler)
        {
            _ipcServer.Unregister(handler);
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
        void Closed(const Core::IPCChannel* channel)
        {
            std::list<ProxyStub::UnknownProxy*> deadProxies;
            std::list<RPC::ExposedInterface> pendingInterfaces;

            RPC::Administrator::Instance().DeleteChannel(channel, deadProxies, pendingInterfaces);

            std::list<ProxyStub::UnknownProxy*>::const_iterator loop(deadProxies.begin());
            while (loop != deadProxies.end()) {
                Core::IUnknown* base = (*loop)->QueryInterface<Core::IUnknown>();
                Revoke(base, (*loop)->InterfaceId());
                if ((*loop)->Destroy() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                    TRACE_L1("Could not destruct a Proxy on a failing channel!!!");
                }
                loop++;
            }

            std::list<RPC::ExposedInterface>::const_iterator loop2(pendingInterfaces.begin());

            while (loop2 != pendingInterfaces.end()) {
                const Core::IUnknown* source = loop2->first; 
                uint32_t count = loop2->second;
                Cleanup(source, count);
                while (count != 0) {
                    source->Release();
                    count--;
                }
                loop2++;
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
        virtual void Cleanup(const Core::IUnknown* /* source */, const uint32_t /* refCount */)
        {
        }

    private:
        RemoteConnectionMap _connectionMap;
        ChannelServer _ipcServer;
        Core::ProxyType<Core::IIPCServer> _stubHandler;
    };

    class EXTERNAL CommunicatorClient : public Core::IPCChannelClientType<Core::Void, false, true>, public Core::IDispatchType<Core::IIPC> {
    private:
        CommunicatorClient() = delete;
        CommunicatorClient(const CommunicatorClient&) = delete;
        CommunicatorClient& operator=(const CommunicatorClient&) = delete;

        typedef Core::IPCChannelClientType<Core::Void, false, true> BaseClass;

        class AnnounceHandler : public Core::IPCServerType<AnnounceMessage> {
        private:
            AnnounceHandler() = delete;
            AnnounceHandler(const AnnounceHandler&) = delete;
            AnnounceHandler& operator=(const AnnounceHandler&) = delete;

        public:
            AnnounceHandler(CommunicatorClient& parent)
                : _parent(parent)
            {
            }
            virtual ~AnnounceHandler()
            {
            }

        public:
            void Procedure(IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data) override
            {
                // Oke, see if we can reference count the IPCChannel
                Core::ProxyType<Core::IPCChannel> refChannel(dynamic_cast<Core::IReferenceCounted*>(&channel), &channel);

                ASSERT(refChannel.IsValid());

                if (refChannel.IsValid() == true) {
                    const string className(data->Parameters().ClassName());
                    const uint32_t interfaceId(data->Parameters().InterfaceId());
                    const uint32_t versionId(data->Parameters().VersionId());
                    void* implementation = _parent.Aquire(className, interfaceId, versionId);
                    data->Response().Implementation(implementation);
                }

                Core::ProxyType<Core::IIPC> baseData(Core::proxy_cast<Core::IIPC>(data));

                channel.ReportResponse(baseData);
            }

        private:
            CommunicatorClient& _parent;
        };

    public:
        CommunicatorClient(const Core::NodeId& remoteNode, Core::ProxyType<IHandler> handler);
        ~CommunicatorClient();

    public:
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

                _announceMessage->Parameters().Set(className, INTERFACE::ID, versionId);

                // Lock event until Dispatch() sets it.
                if (BaseClass::Invoke(_announceMessage, waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() == nullptr);

                    void* implementation(_announceMessage->Response().Implementation());

                    if (implementation != nullptr) {
                        Core::ProxyType<Core::IPCChannel> baseChannel(*this);

                        ASSERT(baseChannel.IsValid() == true);

                        result = Administrator::Instance().ProxyInstance<INTERFACE>(baseChannel, implementation, INTERFACE::ID, true);
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

                _announceMessage->Parameters().Set(INTERFACE::ID, offer, Data::Init::OFFER);

                BaseClass::Invoke(_announceMessage, waitTime);

                // Lock event until Dispatch() sets it.
                if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() != nullptr);

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

                _announceMessage->Parameters().Set(INTERFACE::ID, offer, Data::Init::REVOKE);

                BaseClass::Invoke(_announceMessage, waitTime);

                // Lock event until Dispatch() sets it.
                if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                    ASSERT(_announceMessage->Parameters().InterfaceId() == INTERFACE::ID);
                    ASSERT(_announceMessage->Parameters().Implementation() != nullptr);
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
            ASSERT(_announceMessage->Parameters().Implementation() == nullptr);

            // Lock event until Dispatch() sets it.
            if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

                void* implementation(_announceMessage->Response().Implementation());

                ASSERT(implementation != nullptr);

                if (implementation != nullptr) {
                    Core::ProxyType<Core::IPCChannel> baseChannel(*this);

                    ASSERT(baseChannel.IsValid() == true);

                    result = Administrator::Instance().ProxyInstance<INTERFACE>(baseChannel, implementation, INTERFACE::ID, true);
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
        Core::ProxyType<IHandler> _handler;
        AnnounceHandler _announcements;
    };
}
}

#endif // __COM_PROCESSLAUNCH_H
