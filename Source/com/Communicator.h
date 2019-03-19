#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include "Administrator.h"
#include "ITracing.h"
#include "IUnknown.h"
#include "Module.h"

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

    struct EXTERNAL IRemoteProcess : virtual public Core::IUnknown {
        enum { ID = ID_REMOTEPROCESS };

        virtual ~IRemoteProcess() {}

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_REMOTEPROCESS_NOTIFICATION };

            virtual ~INotification() {}
            virtual void Activated(IRemoteProcess* process) = 0;
            virtual void Deactivated(IRemoteProcess* process) = 0;
        };

        enum enumState {
            CONSTRUCTED,
            ACTIVE,
            DEACTIVATED
        };

        virtual uint32_t Id() const = 0;
        virtual enumState State() const = 0;
        virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
        virtual void Terminate() = 0;

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Aquire(const uint32_t waitTime, const string& className, const uint32_t version)
        {
            void* baseInterface(Aquire(waitTime, className, REQUESTEDINTERFACE::ID, version));

            if (baseInterface != nullptr) {

                Core::IUnknown* iuptr = reinterpret_cast<Core::IUnknown*>(baseInterface);

                REQUESTEDINTERFACE* result = dynamic_cast<REQUESTEDINTERFACE*>(iuptr);

                if (result == nullptr) {

                    result = reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface);
                }

                return result;
            }

            return (nullptr);
        }
    };

    class EXTERNAL Communicator {
    private:
        class RemoteProcessMap;

    public:
        class EXTERNAL RemoteProcess : public IRemoteProcess {
        private:
            friend class RemoteProcessMap;

            RemoteProcess() = delete;
            RemoteProcess(const RemoteProcess&) = delete;
            RemoteProcess& operator=(const RemoteProcess&) = delete;

        protected:
            RemoteProcess(RemoteProcessMap* parent, const Core::ProxyType<Core::IPCChannel>& channel)
                : _parent(parent)
                , _state(IRemoteProcess::ACTIVE)
                , _channel(channel)
            {
            }
            RemoteProcess(RemoteProcessMap* parent)
                : _parent(parent)
                , _state(IRemoteProcess::CONSTRUCTED)
                , _channel()
            {
            }

        public:
            ~RemoteProcess()
            {
            }

        public:
            inline bool HasChannel() const
            {
                return (_channel.IsValid());
            }
            inline const Core::IPCChannel* Channel() const
            {
                return (_channel.operator->());
            }
            virtual enumState State() const
            {
                return (_state);
            }
            virtual void* QueryInterface(const uint32_t id)
            {
                if (id == IRemoteProcess::ID) {
                    AddRef();
                    return (static_cast<IRemoteProcess*>(this));
                } else {
                    assert(false);
                }
                return (nullptr);
            }
            virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version);

            uint32_t WaitState(const uint32_t state, const uint32_t time) const
            {
                return (_state.WaitState(state, time));
            }
            void Announce(Core::ProxyType<Core::IPCChannel>& channel)
            {
                // Seems we received an interface from the otherside. Prepare the actual stub around it.
                TRACE_L1("Remote Process %d, has announced itself.", Id());

                _channel = channel;
            }

            inline void Activate()
            {
                State(IRemoteProcess::ACTIVE);
            }

            virtual void Terminate();

        protected:
            inline void State(const IRemoteProcess::enumState newState)
            {

                ASSERT(newState != IRemoteProcess::CONSTRUCTED);

                _state.Lock();

                if (_state != newState) {

                    _state = newState;

                    if (_state == IRemoteProcess::DEACTIVATED) {

                        _parent->Deactivated(this);

                        if (_channel.IsValid() == true) {
                            _channel.Release();
                        }
                    } else if (_state == IRemoteProcess::ACTIVE) {

                        _parent->Activated(this);
                    }
                }

                _state.Unlock();
            }

        private:
            RemoteProcessMap* _parent;
            Core::StateTrigger<IRemoteProcess::enumState> _state;
            Core::ProxyType<Core::IPCChannel> _channel;
        };

    private:
        class EXTERNAL MasterRemoteProcess : public RemoteProcess {
        private:
            friend class Core::Service<MasterRemoteProcess>;

            MasterRemoteProcess() = delete;
            MasterRemoteProcess(const MasterRemoteProcess&) = delete;
            MasterRemoteProcess& operator=(const MasterRemoteProcess&) = delete;

        protected:
            MasterRemoteProcess(RemoteProcessMap* parent, uint32_t* pid, const Core::Process::Options* options)
                : RemoteProcess(parent)
                , _process(false)
            {
                // Start the external process launch..
                _process.Launch(*options, pid);
            }

        public:
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

                return (Core::Service<MasterRemoteProcess>::Create<MasterRemoteProcess>(&parent, &pid, &options));
            }
            ~MasterRemoteProcess()
            {
                TRACE_L1("Destructor for MasterRemoteProcess process for %d", Id());
            }

        public:
            virtual uint32_t Id() const
            {
                return (_process.Id());
            }
            inline bool IsActive() const
            {
                return (_process.IsActive());
            }
            inline uint32_t ExitCode() const
            {
                return (_process.ExitCode());
            }
            inline void Kill(const bool hardKill)
            {
                return (_process.Kill(hardKill));
            }

        private:
            Core::Process _process;
        };
        class EXTERNAL SlaveRemoteProcess : public RemoteProcess {
        private:
            friend class Core::Service<SlaveRemoteProcess>;

            SlaveRemoteProcess() = delete;
            SlaveRemoteProcess(const SlaveRemoteProcess&) = delete;
            SlaveRemoteProcess& operator=(const SlaveRemoteProcess&) = delete;

            SlaveRemoteProcess(RemoteProcessMap* parent, const uint32_t pid, const Core::ProxyType<Core::IPCChannel>& channel)
                : RemoteProcess(parent, channel)
                , _pid(pid)
            {
            }

        public:
            inline static RemoteProcess* Create(RemoteProcessMap& parent, const uint32_t pid, Core::ProxyType<Core::IPCChannel>& channel)
            {
                RemoteProcess* result(Core::Service<SlaveRemoteProcess>::Create<SlaveRemoteProcess>(&parent, pid, channel));

                ASSERT(result != nullptr);

                // As the channel is there, Announde came in over it :-), we are active by default.
                parent.Activated(result);

                return (result);
            }
            ~SlaveRemoteProcess()
            {
                TRACE_L1("Destructor for Remote process for %d", Id());
            }

        public:
            virtual uint32_t Id() const
            {
                return (_pid);
            }
            virtual void Terminate()
            {
            }

        private:
            uint32_t _pid;
        };

    private:
        class EXTERNAL RemoteProcessMap {
        private:
            class ClosingInfo {
            private:
                ClosingInfo() = delete;
                ClosingInfo& operator=(const ClosingInfo& RHS) = delete;

                enum enumState {
                    INITIAL,
                    SOFTKILL,
                    HARDKILL
                };

            public:
                ClosingInfo(MasterRemoteProcess* process)
                    : _process(process)
                    , _state(INITIAL)
                {
                    if (_process != nullptr) {
                        _process->AddRef();
                    }
                }
                ClosingInfo(const ClosingInfo& copy)
                    : _process(copy._process)
                    , _state(copy._state)
                {
                    if (_process != nullptr) {
                        _process->AddRef();
                    }
                }
                ~ClosingInfo()
                {
                    if (_process != nullptr) {
                        // If we are still active, No time to wait, force kill
                        if (_process->IsActive() == true) {
                            _process->Kill(true);
                        }
                        _process->Release();
                    }
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    uint64_t result = 0;

                    if (_process->IsActive() == false) {
                        // It is no longer active.... just report, and clear our selves !!!
                        TRACE_L1("Destructed. Closed down nicely [%d].\n", _process->Id());
                        _process->Release();
                        _process = nullptr;
                    } else if (_state == INITIAL) {
                        _state = SOFTKILL;
                        _process->Kill(false);
                        result = Core::Time(scheduledTime).Add(6000).Ticks(); // Next check in 6S
                    } else if (_state == SOFTKILL) {
                        _state = HARDKILL;
                        _process->Kill(true);
                        result = Core::Time(scheduledTime).Add(8000).Ticks(); // Next check in 8S
                    } else {
                        // This should not happen. This is a very stubbern process. Can be killed.
                        ASSERT(false);
                    }

                    return (result);
                }

            private:
                MasterRemoteProcess* _process;
                enumState _state;
            };

        private:
            RemoteProcessMap(const RemoteProcessMap&) = delete;
            RemoteProcessMap& operator=(const RemoteProcessMap&) = delete;

            static constexpr uint32_t DestructionStackSize = 64 * 1024;

        public:
            RemoteProcessMap(Communicator& parent)
                : _adminLock()
                , _processes()
                , _destructor(DestructionStackSize, "ProcessDestructor")
                , _parent(parent)
            {
            }
            virtual ~RemoteProcessMap()
            {
                // All observers should have unregistered before this map get's destroyed !!!
                ASSERT(_observers.size() == 0);

                while (_observers.size() != 0) {
                    _observers.front()->Release();
                    _observers.pop_front();
                }
            }

        public:
            inline void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t version)
            {
                return (_parent.Aquire(className, interfaceId, version));
            }
            inline void Register(RPC::IRemoteProcess::INotification* sink)
            {
                ASSERT(sink != nullptr);

                if (sink != nullptr) {

                    _adminLock.Lock();

                    ASSERT(std::find(_observers.begin(), _observers.end(), sink) == _observers.end());

                    sink->AddRef();
                    _observers.push_back(sink);

                    std::map<uint32_t, RemoteProcess*>::iterator index(_processes.begin());

                    // Report all Active Processes..
                    while (index != _processes.end()) {
                        if (index->second->State() == RemoteProcess::ACTIVE) {
                            sink->Activated(&(*(index->second)));
                        }
                        index++;
                    }

                    _adminLock.Unlock();
                }
            }
            inline void Unregister(RPC::IRemoteProcess::INotification* sink)
            {
                ASSERT(sink != nullptr);

                if (sink != nullptr) {

                    _adminLock.Lock();

                    std::list<RPC::IRemoteProcess::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), sink));

                    ASSERT(index != _observers.end());

                    if (index != _observers.end()) {
                        (*index)->Release();
                        _observers.erase(index);
                    }

                    _adminLock.Unlock();
                }
            }
            inline uint32_t Size() const
            {
                return (static_cast<uint32_t>(_processes.size()));
            }
            inline Communicator::RemoteProcess* Create(uint32_t& pid, const Object& instance, const Config& config, const uint32_t waitTime)
            {
                _adminLock.Lock();

                Communicator::RemoteProcess* result = MasterRemoteProcess::Create(*this, pid, instance, config);

                ASSERT(result != nullptr);

                if (result != nullptr) {

                    // A reference for putting it in the list...
                    result->AddRef();

                    // We expect an announce interface message now...
                    _processes.insert(std::pair<uint32_t, RemoteProcess*>(result->Id(), result));

                    _adminLock.Unlock();

                    // Now lets wait for the announce message to be exchanged
                    result->WaitState(RPC::IRemoteProcess::ACTIVE | RPC::IRemoteProcess::DEACTIVATED, waitTime);

                    if (result->State() != RPC::IRemoteProcess::ACTIVE) {
                        result->Terminate();
                        result = nullptr;
                    }
                } else {
                    _adminLock.Unlock();
                }

                return (result);
            }

            inline void Destroy(const uint32_t pid)
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess*>::iterator index(_processes.find(pid));

                if (index != _processes.end()){
					
					if (index->second->HasChannel() == true) {

						std::list<ProxyStub::UnknownProxy*> deadProxies;
						RPC::Administrator::Instance().DeleteChannel(index->second->Channel(), deadProxies);
						std::list<ProxyStub::UnknownProxy*>::const_iterator loop(deadProxies.begin());
						while (loop != deadProxies.end()) {
							Core::IUnknown* base = (*loop)->QueryInterface<Core::IUnknown>();
							_parent.Revoke(pid, base, (*loop)->InterfaceId());
							if ((*loop)->Destroy() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
								TRACE_L1("Could not destruct a Proxy on a failing channel!!!");
							}
							loop++;
						}
					}

                    index->second->State(IRemoteProcess::DEACTIVATED);

                    MasterRemoteProcess* base = dynamic_cast<MasterRemoteProcess*>(index->second);

                    if (base == nullptr) {
                        TRACE_L1("Connection lost to process: %d", pid);
                    } else if (base->IsActive() == false) {
                        TRACE_L1("CLEAN EXIT of process: %d", pid);
                    } else {
                        Core::Time scheduleTime(Core::Time::Now().Add(3000)); // Check again in 3S...

                        TRACE_L1("Submitting process for closure: %d", pid);
                        _destructor.Schedule(scheduleTime, ClosingInfo(base));
                    }

                    index->second->Release();

                    // Release this entry, do not wait till it get's overwritten.
                    _processes.erase(index);
                }

                _adminLock.Unlock();
            }

            Communicator::RemoteProcess* Process(const uint32_t id)
            {
                Communicator::RemoteProcess* result = nullptr;

                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess*>::iterator index(_processes.find(id));

                if (index != _processes.end()) {
                    result = index->second;
                    result->AddRef();
                }

                _adminLock.Unlock();

                return (result);
            }

            inline void Processes(std::list<uint32_t>& pidList) const
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess*>::const_iterator index(_processes.begin());

                while (index != _processes.end()) {
                    pidList.push_back(index->first);
                    index++;
                }

                _adminLock.Unlock();
            }
            uint32_t Announce(Core::ProxyType<Core::IPCChannel>& channel, const Data::Init& info, void*& implementation)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (channel.IsValid() == true) {

                    _adminLock.Lock();

                    std::map<uint32_t, Communicator::RemoteProcess*>::iterator index(_processes.find(info.ExchangeId()));

                    if (index == _processes.end()) {
                        // This is an announce message from a process that wasn't created by us. So typically this is
                        // An RPC client reaching out to an RPC server. The RPCServer does not spawn processes it just
                        // listens for clients requesting service.
                        Communicator::RemoteProcess* remoteProcess = SlaveRemoteProcess::Create(*this, info.ExchangeId(), channel);

                        ASSERT(remoteProcess != nullptr);

                        // Add ref is done during the creation, no need to take another reference unless we also would release it after
                        // insertion :-)
                        auto newElement = _processes.insert(std::pair<uint32_t, Communicator::RemoteProcess*>(info.ExchangeId(), remoteProcess));

                        index = newElement.first;
                    } else {
                        index->second->Announce(channel);
                    }

                    ASSERT(index != _processes.end());

                    if (implementation == nullptr) {

                        if (info.InterfaceId() != static_cast<uint32_t>(~0)) {
                            // See if we have something we can return right away, if it has been requested..
                            implementation = Aquire(info.ClassName(), info.InterfaceId(), info.VersionId());
                        }
                    } else if ((info.IsOffer() == true) || (info.IsRequested())) {
                        Core::IUnknown* result = Administrator::Instance().ProxyInstance<Core::IUnknown>(channel, implementation, info.InterfaceId(), info.IsRequested());
                        if (result != nullptr) {
                            _parent.Offer(index->first, result, info.InterfaceId());
                            result->Release();
                        }
                    } else {
                        ASSERT(info.IsRevoke() == true);

                        Core::IUnknown* result = Administrator::Instance().ProxyFind<Core::IUnknown>(channel, implementation, info.InterfaceId());
                        if (result != nullptr) {
                            _parent.Revoke(index->first, result, info.InterfaceId());
                            result->Release();
                        }
                    }

                    index->second->Activate();

                    result = Core::ERROR_NONE;

                    _adminLock.Unlock();
                }

                return (result);
            }

            void Activated(RPC::IRemoteProcess* process)
            {
                _adminLock.Lock();

                std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

                while (index != _observers.end()) {
                    (*index)->Activated(process);
                    index++;
                }

                _adminLock.Unlock();
            }
            void Deactivated(RPC::IRemoteProcess* process)
            {
                _adminLock.Lock();

                std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

                while (index != _observers.end()) {
                    (*index)->Deactivated(process);
                    index++;
                }

                _adminLock.Unlock();
            }

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<uint32_t, Communicator::RemoteProcess*> _processes;
            std::list<RPC::IRemoteProcess::INotification*> _observers;
            Core::TimerType<ClosingInfo> _destructor;
            Communicator& _parent;
        };
        class EXTERNAL ProcessChannelLink {
        private:
            ProcessChannelLink() = delete;
            ProcessChannelLink(const ProcessChannelLink&) = delete;
            ProcessChannelLink& operator=(const ProcessChannelLink&) = delete;

        public:
            ProcessChannelLink(Core::IPCChannelType<Core::SocketPort, ProcessChannelLink>* channel)
                : _channel(*channel)
                , _processMap(nullptr)
                , _pid(0)
            {
                // We are a composit of the Channel, no need (and do not for cyclic references) not maintain a reference...
                ASSERT(channel != nullptr);
            }
            ~ProcessChannelLink()
            {
            }

        public:
            inline bool IsValid() const
            {
                return (_processMap != nullptr);
            }
            void Link(RemoteProcessMap& processMap, const uint32_t pid)
            {
                _processMap = &processMap;
                _pid = pid;
            }
            uint32_t Pid() const
            {
                return (_pid);
            }

            void StateChange()
            {
                if ((_channel.Source().IsOpen() == false) && (_pid != 0)) {

                    ASSERT(_processMap != nullptr);

                    // Seems there is not much we can do, time to kill the process, we lost a connection with it.
                    TRACE_L1("Lost connection on process [%d]. Closing the process.\n", _pid);
                    _processMap->Destroy(_pid);
                    // _pid = 0;
                }
            }

        private:
            // Non ref-counted reference to our parent, of which we are a composit :-)
            Core::IPCChannelType<Core::SocketPort, ProcessChannelLink>& _channel;
            RemoteProcessMap* _processMap;
            uint32_t _pid;
        };
        class EXTERNAL ProcessChannelServer : public Core::IPCChannelServerType<ProcessChannelLink, true> {
        private:
            ProcessChannelServer(const ProcessChannelServer&) = delete;
            ProcessChannelServer& operator=(const ProcessChannelServer&) = delete;

            typedef Core::IPCChannelServerType<ProcessChannelLink, true> BaseClass;
            typedef Core::IPCChannelType<Core::SocketPort, ProcessChannelLink> Client;

            class EXTERNAL InterfaceAnnounceHandler : public Core::IPCServerType<AnnounceMessage> {
            private:
                InterfaceAnnounceHandler() = delete;
                InterfaceAnnounceHandler(const InterfaceAnnounceHandler&) = delete;
                InterfaceAnnounceHandler& operator=(const InterfaceAnnounceHandler&) = delete;

            public:
                InterfaceAnnounceHandler(ProcessChannelServer* parent)
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
                    void* result = data->Parameters().Implementation();

                    // Anounce the interface as completed
                    string jsonDefaultCategories;
                    Trace::TraceUnit::Instance().GetDefaultCategoriesJson(jsonDefaultCategories);
                    _parent.Announce(channel, data->Parameters(), result);
                    data->Response().Set(result, _parent.ProxyStubPath(), jsonDefaultCategories);

                    // We are done, report completion
                    Core::ProxyType<Core::IIPC> baseMessage(Core::proxy_cast<Core::IIPC>(data));
                    channel.ReportResponse(baseMessage);
                }

            private:
                ProcessChannelServer& _parent;
            };

        public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            ProcessChannelServer(const Core::NodeId& remoteNode, RemoteProcessMap& processes, Core::ProxyType<IHandler>& handler, const string& proxyStubPath)
                : BaseClass(remoteNode, CommunicationBufferSize)
                , _proxyStubPath(proxyStubPath)
                , _processes(processes)
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

            ~ProcessChannelServer()
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
            inline uint32_t Announce(Core::IPCChannel& channel, const Data::Init& info, void*& implementation)
            {
                Core::ProxyType<Core::IPCChannel> baseChannel(channel);

                ASSERT(baseChannel.IsValid() == true);

                uint32_t result = _processes.Announce(baseChannel, info, implementation);

                if (result == Core::ERROR_NONE) {

                    // We are in business, register the process with this channel.
                    ASSERT(dynamic_cast<Client*>(&channel) != nullptr);

                    Client& client(static_cast<Client&>(channel));

                    client.Extension().Link(_processes, info.ExchangeId());
                }

                return (result);
            }

        private:
            const string _proxyStubPath;
            RemoteProcessMap& _processes;
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
        inline void Register(RPC::IRemoteProcess::INotification* sink)
        {
            _processMap.Register(sink);
        }
        inline void Unregister(RPC::IRemoteProcess::INotification* sink)
        {
            _processMap.Unregister(sink);
        }
        inline void Processes(std::list<uint32_t>& pidList) const
        {
            _processMap.Processes(pidList);
        }
        inline Communicator::RemoteProcess* Process(const uint32_t id)
        {
            return (_processMap.Process(id));
        }
        inline Communicator::RemoteProcess* Create(uint32_t& pid, const Object& instance, const Config& config, const uint32_t waitTime)
        {
            return (_processMap.Create(pid, instance, config, waitTime));
        }

        // Use this method with care. It goes beyond the refence counting taking place in the object.
        // This method kills the process without any considerations!!!!
        inline void Destroy(const uint32_t& pid)
        {
            _processMap.Destroy(pid);
        }

    private:
        virtual void* Aquire(const string& /* className */, const uint32_t /* interfaceId */, const uint32_t /* version */)
        {
            return (nullptr);
        }
        virtual void Offer(const uint32_t /* processId */, Core::IUnknown* /* remote */, const uint32_t /* interfaceId */)
        {
        }
        // note: do NOT do a QueryInterface on the IUnknown pointer (or any other method for that matter), the object it points to might already be destroyed
        virtual void Revoke(const uint32_t /* processId */, const Core::IUnknown* /* remote */, const uint32_t /* interfaceId */)
        {
        }

    private:
        RemoteProcessMap _processMap;
        ProcessChannelServer _ipcServer;
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
        uint32_t Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation);

        template <typename INTERFACE>
        INTERFACE* Aquire(const uint32_t waitTime, const string& className, const uint32_t versionId)
        {
            INTERFACE* result(nullptr);

            ASSERT(className.empty() == false);

            if (BaseClass::IsOpen() == true) {

                _announceMessage->Parameters().Set(className, INTERFACE::ID, versionId);

                BaseClass::Invoke(_announceMessage, waitTime);

                // Lock event until Dispatch() sets it.
                if (_announceEvent.Lock(waitTime) == Core::ERROR_NONE) {

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
            return (Core::ServiceAdministrator::Instance().Instantiate(emptyLibrary, className.c_str(), versionId, interfaceId));
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
