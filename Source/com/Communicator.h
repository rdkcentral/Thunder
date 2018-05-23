#ifndef __COM_PROCESSLAUNCH_H
#define __COM_PROCESSLAUNCH_H

#include "Module.h"
#include "Administrator.h"
#include "ITracing.h"

#include "../tracing/TraceUnit.h"

namespace WPEFramework {
namespace RPC {

    struct EXTERNAL IRemoteProcess : virtual public Core::IUnknown {
        enum { ID = 0x00000001 };

        virtual ~IRemoteProcess() {}

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000002 };

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
        virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
        virtual void Terminate() = 0;

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Instantiate(const uint32_t waitTime, const string& className, const uint32_t version)
        {
            void* baseInterface(Instantiate(waitTime, className, REQUESTEDINTERFACE::ID, version));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
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
            friend class Core::Service<RemoteProcess>;
            friend class RemoteProcessMap;

            RemoteProcess() = delete;
            RemoteProcess(const RemoteProcess&) = delete;
            RemoteProcess& operator=(const RemoteProcess&) = delete;

			RemoteProcess(RemoteProcessMap* parent, uint32_t pid, const Core::ProxyType<Core::IPCChannel>& channel)
				: _parent(parent)
				, _process(pid)
				, _state(IRemoteProcess::ACTIVE)
				, _channel(channel)
				, _returnedInterface(nullptr)
			{
			}
			RemoteProcess(RemoteProcessMap* parent, uint32_t* pid, const Core::Process::Options* options)
                : _parent(parent)
                , _process(false)
                , _state(IRemoteProcess::CONSTRUCTED)
                , _channel()
                , _returnedInterface(nullptr)
            {
                // Start the external process launch..
                _process.Launch(*options, pid);
            }

        public:
            inline static RemoteProcess* Create(RemoteProcessMap& parent, const uint32_t pid, Core::ProxyType<Core::IPCChannel>& channel)
            {
				RemoteProcess* result (Core::Service<RemoteProcess>::Create<RemoteProcess>(&parent, pid, channel));

				ASSERT(result != nullptr);

				// As the channel is there, Announde came in over it :-), we are active by default.
				parent.Activated(result);

				return (result);
            }
			inline static RemoteProcess* Create(RemoteProcessMap& parent, uint32_t& pid, const Core::Process::Options& options)
			{
				return (Core::Service<RemoteProcess>::Create<RemoteProcess>(&parent, &pid, &options));
			}
			~RemoteProcess()
            {
                TRACE_L1("Destructor for Remote process for %d", _process.Id());
            }

        public:
            virtual uint32_t Id() const
            {
                return (_process.Id());
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
                }
                else if (_returnedInterface != nullptr) {
                    void* result = _returnedInterface;
                    _returnedInterface = nullptr;
                    return (result);
                }
                return (nullptr);
            }
            virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version);

            uint32_t WaitState(const uint32_t state, const uint32_t time) const
            {
                return (_state.WaitState(state, time));
            }
            inline bool IsActive() const
            {
                return (_process.IsActive());
            }
            inline uint32_t ExitCode() const
            {
                return (_process.ExitCode());
            }
            void Terminate();
            void Announce(Core::ProxyType<Core::IPCChannel>& channel, const uint32_t exchangeId, const uint32_t interfaceId, void* implementation);

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
                    }
					else if (_state == IRemoteProcess::ACTIVE) {

						_parent->Activated(this);
					}
				}

                _state.Unlock();
            }
            inline void Kill(const bool hardKill)
            {
                return (_process.Kill(hardKill));
            }

        private:
            RemoteProcessMap* _parent;
            Core::Process _process;
            Core::StateTrigger<IRemoteProcess::enumState> _state;
            Core::ProxyType<Core::IPCChannel> _channel;
            void* _returnedInterface;
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
                ClosingInfo(RemoteProcess* process)
                    : _process(process)
                    , _state(INITIAL)
                {
                    if (_process != nullptr) 
                    {
                        _process->AddRef();
                    }
                }
                ClosingInfo(const ClosingInfo& copy)
                    : _process(copy._process)
                    , _state(copy._state)
                {
                    if (_process != nullptr) 
                    {
                        _process->AddRef();
                    }
                }
                ~ClosingInfo()
                {
                    if (_process != nullptr) 
                    {
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
                    }
                    else if (_state == INITIAL) {
                        _state = SOFTKILL;
                        _process->Kill(false);
                        result = Core::Time(scheduledTime).Add(6000).Ticks(); // Next check in 6S
                    }
                    else if (_state == SOFTKILL) {
                        _state = HARDKILL;
                        _process->Kill(true);
                        result = Core::Time(scheduledTime).Add(8000).Ticks(); // Next check in 8S
                    }
                    else {
                        // This should not happen. This is a very stubbern process. Can be killed.
                        ASSERT(false);
                    }

                    return (result);
                }

            private:
                RemoteProcess* _process;
                enumState _state;
            };

        private:
            RemoteProcessMap(const RemoteProcessMap&) = delete;
            RemoteProcessMap& operator=(const RemoteProcessMap&) = delete;

            static constexpr uint32_t DestructionStackSize = 64 * 1024;

        public:
            RemoteProcessMap()
                : _adminLock()
                , _processes()
                , _destructor(DestructionStackSize, "ProcessDestructor")
            {
            }

            virtual ~RemoteProcessMap()
            {
				// All observers should have unregistered before this map get's destroyed !!!
				ASSERT(_observers.size() == 0);

				while (_observers.size() != 0)
				{
					_observers.front()->Release();
					_observers.pop_front();
				}
            }

        public:
			inline void Register(RPC::IRemoteProcess::INotification* sink)
			{
				ASSERT(sink != nullptr);

				if (sink != nullptr) {

					_adminLock.Lock();

					ASSERT(std::find(_observers.begin(), _observers.end(), sink) == _observers.end());

					sink->AddRef();
					_observers.push_back(sink);

					std::map<uint32_t, RemoteProcess* >::iterator index(_processes.begin());

					// Report all Active Processes..
					while (index != _processes.end())
					{
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

					if (index != _observers.end())
					{
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
            inline Communicator::RemoteProcess* Create(const Core::Process::Options& options, uint32_t& pid)
            {
                _adminLock.Lock();

                Communicator::RemoteProcess* result = RemoteProcess::Create(*this, pid, options);

                ASSERT(result != nullptr);

                if (result != nullptr) {

                    // A reference for putting it in the list...
                    result->AddRef();
 
                    // We expect an announce interface message now...
                    _processes.insert(std::pair<uint32_t, RemoteProcess* >(result->Id(), result));
                }

                _adminLock.Unlock();

                return (result);
            }

            inline void Destroy(const uint32_t pid)
            {
                // First do an activity check on all processes registered.
                _adminLock.Lock();

                std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(pid));

                if (index != _processes.end()) {

                    index->second->State(IRemoteProcess::DEACTIVATED);

                    if (index->second->IsActive() == false) {
                        TRACE_L1("CLEAN EXIT of process: %d", pid);
                    }
                    else {
                        Core::Time scheduleTime(Core::Time::Now().Add(3000)); // Check again in 3S...

                        TRACE_L1("Submitting process for closure: %d", pid);
                        _destructor.Schedule(scheduleTime, ClosingInfo(index->second));
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

                std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(id));

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

                std::map<uint32_t, Communicator::RemoteProcess* >::const_iterator index(_processes.begin());

                while (index != _processes.end()) {
                    pidList.push_back(index->first);
                    index++;
                }

                _adminLock.Unlock();
            }
            uint32_t Announce(Core::ProxyType<Core::IPCChannel>& channel, const uint32_t exchangeId, const uint32_t interfaceId, void* implementation)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (channel.IsValid() == true) {

                    _adminLock.Lock();

                    std::map<uint32_t, Communicator::RemoteProcess* >::iterator index(_processes.find(exchangeId));

                    if (index != _processes.end()) {
                        index->second->Announce(channel, exchangeId, interfaceId, implementation);
                        index->second->State(IRemoteProcess::ACTIVE);

                        result = Core::ERROR_NONE;
                    } else {
                        // This is an announce message from a process that wasn't created by us.
                        Communicator::RemoteProcess* remoteProcess = RemoteProcess::Create(*this, exchangeId, channel);

                        if (remoteProcess != nullptr) 
                        {
                            // Add ref is done during the creation, no need to take another reference unless we also wouldrelease it after 
                            // insertion :-)
                            _processes.insert(std::pair<uint32_t, Communicator::RemoteProcess* > (exchangeId, remoteProcess));

                            result = Core::ERROR_NONE;
                        }
                    }

                    _adminLock.Unlock();
                }

                return (result);
            }

			void Activated(RPC::IRemoteProcess* process)
			{
				_adminLock.Lock();

				std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

				while (index != _observers.end())
				{
					(*index)->Activated(process);
					index++;
				}

				_adminLock.Unlock();
			}
			void Deactivated(RPC::IRemoteProcess* process)
			{
				_adminLock.Lock();

				std::list<RPC::IRemoteProcess::INotification*>::iterator index(_observers.begin());

				while (index != _observers.end())
				{
					(*index)->Deactivated(process);
					index++;
				}

				_adminLock.Unlock();
			}

        private:
            mutable Core::CriticalSection _adminLock;
            std::map<uint32_t, Communicator::RemoteProcess* > _processes;
            std::list<RPC::IRemoteProcess::INotification*> _observers;
            Core::TimerType<ClosingInfo> _destructor;
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
                    _pid = 0;
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
                virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<AnnounceMessage>& data)
                {
                    // Anounce the interface as completed
                    string jsonDefaultCategories;
                    Trace::TraceUnit::Instance().GetDefaultCategoriesJson(jsonDefaultCategories);
                    _parent.Announce(channel, data->Parameters().ExchangeId(), data->Parameters().InterfaceId(), data->Parameters().Implementation());
                    data->Response() = jsonDefaultCategories;

                    // We are done, report completion
                    Core::ProxyType<Core::IIPC> baseMessage(Core::proxy_cast<Core::IIPC>(data));
                    channel.ReportResponse(baseMessage);
                }

            private:
                ProcessChannelServer& _parent;
            };

        public:
			#ifdef __WIN32__ 
			#pragma warning( disable : 4355 )
			#endif
			ProcessChannelServer(const Core::NodeId& remoteNode, RemoteProcessMap& processes, const Core::ProxyType<Core::IIPCServer>& handler)
                : BaseClass(remoteNode, CommunicationBufferSize)
                , _processes(processes)
                , _interfaceAnnounceHandler(Core::ProxyType<InterfaceAnnounceHandler>::Create(this))
                , _interfaceMessageHandler(handler)
            {
                BaseClass::Register(_interfaceAnnounceHandler);
                BaseClass::Register(handler);
            }
			#ifdef __WIN32__ 
			#pragma warning( default : 4355 )
			#endif

			~ProcessChannelServer()
            {
                BaseClass::Unregister(_interfaceAnnounceHandler);
                BaseClass::Unregister(_interfaceMessageHandler);
            }

        private:
            inline uint32_t Announce(Core::IPCChannel& channel, const uint32_t exchangeId, const uint32_t interfaceId, void* implementation)
            {
                Core::ProxyType<Core::IPCChannel> baseChannel(channel);

                ASSERT(baseChannel.IsValid() == true);

                uint32_t result = _processes.Announce(baseChannel, exchangeId, interfaceId, implementation);

                if (result == Core::ERROR_NONE) {

                    // We are in business, register the process with this channel.
                    ASSERT(dynamic_cast<Client*>(&channel) != nullptr);

                    Client& client(static_cast<Client&>(channel));

                    client.Extension().Link(_processes, exchangeId);
                }

                return (result);
            }

        private:
            RemoteProcessMap& _processes;
            Core::ProxyType<Core::IIPCServer> _interfaceAnnounceHandler; // IPCInterfaceAnnounce
            Core::ProxyType<Core::IIPCServer> _interfaceMessageHandler; //IPCInterfaceMessage
        };

    private:
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator& operator=(const Communicator&) = delete;

    public:
        Communicator(const Core::NodeId& node, const Core::ProxyType<Core::IIPCServer>& handler);
        virtual ~Communicator();

    public:
	inline uint32_t Open(const uint32_t waitTime) {
		return (_ipcServer.Open(waitTime));
	}
	inline uint32_t Close(const uint32_t waitTime) {
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
	inline const string& Connector() const {
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
		inline Communicator::RemoteProcess* Create(uint32_t& pid, const Core::Process::Options& options) {
			return (_processMap.Create(options, pid));
		}

        // Use this method with care. It goes beyond the refence counting taking place in the object.
        // This method kills the process without any considerations!!!!
        inline void Destroy(const uint32_t& pid)
        {
            _processMap.Destroy(pid);
        }

		void LoadProxyStubs(const string& pathName);

    private:
        RemoteProcessMap _processMap;
        ProcessChannelServer _ipcServer;
        Core::ProxyType<Core::IIPCServer> _stubHandler;
        std::list<Core::Library> _proxyStubs;
    };


    class EXTERNAL CommunicatorClient : public Core::IPCChannelClientType<Core::Void, false, true>, public Core::IDispatchType<Core::IIPC> {
    private:
		CommunicatorClient() = delete;
		CommunicatorClient(const CommunicatorClient&) = delete;
        CommunicatorClient& operator=(const CommunicatorClient&) = delete;

        typedef Core::IPCChannelClientType<Core::Void, false, true> BaseClass;

    public:
        CommunicatorClient(const Core::NodeId& remoteNode);
        ~CommunicatorClient();

    public:
        template <typename INTERFACE>
        inline INTERFACE* Create(const string& className, const uint32_t version = static_cast<uint32_t>(~0), const uint32_t waitTime = CommunicationTimeOut)
        {
            return (static_cast<INTERFACE*>(Create(waitTime, className, INTERFACE::ID, version)));
        }
        uint32_t Open(const uint32_t waitTime = CommunicationTimeOut);
		uint32_t Open(const uint32_t interfaceId, void* implementation, const uint32_t waitTime = CommunicationTimeOut);
		uint32_t Close(const uint32_t waitTime);
		void WaitForCompletion();

    private:
		void* Create(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version = static_cast<uint32_t>(~0));

		virtual void StateChange();
		virtual void Dispatch(Core::IIPC& element);

	private:
		Core::ProxyType<RPC::AnnounceMessage> _announceMessage;
		Core::Event _announceEvent;
    };
}
}

#endif // __COM_PROCESSLAUNCH_H
