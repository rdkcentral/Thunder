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

#include "Communicator.h"

#include <limits>
#include <memory>

#ifdef PROCESSCONTAINERS_ENABLED
#include "ProcessInfo.h"
#endif

namespace WPEFramework {
namespace RPC {

    class ProcessShutdown;

    static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);

    /* static */ Core::CriticalSection Process::_ldLibLock ;

    class ProcessShutdown : public Core::Thread {
    public:
        static constexpr uint32_t DestructionStackSize = 64 * 1024;

        class Destructor {
        private:

        public:
            Destructor(const Destructor&) = delete;
            Destructor& operator= (const Destructor&) = delete;

            Destructor()
                : _cycle(0)
                , _time(0) {
            }
            virtual ~Destructor() = default;

        public:
            uint8_t Cycle() const {
                return (_cycle);
            }
            const uint64_t& Time() const {
                return (_time);
            }
            void Destruct() {
                // Unconditionally KILL !!!
                while (Terminate() != 0) {
                    ++_cycle;
                    ::SleepMs(1);
                }
            }
            bool Destruct(uint64_t& timeSlot) {
                bool destructed = false;

                if (_time <= timeSlot) {
                    uint32_t delay = Terminate();

                    if (delay == 0) {
                        destructed = true;
                    }
                    else {
                        timeSlot = Core::Time::Now().Ticks();
                        _time = timeSlot + (delay * 1000 * Core::Time::TicksPerMillisecond);
                        ++_cycle;
                    }
                }

                return (destructed);
            }

        private:
            // Should return 0 if no more iterations are needed.
            virtual uint32_t Terminate() = 0;

        private:
            uint8_t _cycle;
            uint64_t _time; // in Seconds
        };

        using DestructorMap = std::unordered_map<uint32_t, Destructor* >;

    public:
        ProcessShutdown(const ProcessShutdown& copy) = delete;
        ProcessShutdown& operator=(const ProcessShutdown& RHS) = delete;

        ProcessShutdown()
            : Core::Thread(ProcessShutdown::DestructionStackSize, "COMRPCTerminator")
            , _adminLock()
            , _destructors() {
        }
        ~ProcessShutdown() = default;

    public:
        void ForceDestruct(const uint32_t id) {

            Destructor* handler = nullptr;

            _adminLock.Lock();

            DestructorMap::iterator index(_destructors.find(id));

            if (index != _destructors.end()) {
                handler = index->second;
                _destructors.erase(index);
            }

            _adminLock.Unlock();

            if (handler != nullptr) {

                // Forcefully kill this ID.
                handler->Destruct();

                delete handler;
            }

        }
        template <class IMPLEMENTATION, typename... Args>
        void Destruct(const uint32_t id, Args... args)
        {
            _adminLock.Lock();

            if (_destructors.find(id) == _destructors.end()) {

                Destructor* handler = new IMPLEMENTATION(std::forward<Args>(args)...);

                _destructors.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(handler));

                Run();
            }

            _adminLock.Unlock();
        }

    private:
        uint32_t Worker() override {
            uint32_t delay(Core::infinite);
            uint64_t now(Core::Time::Now().Ticks());
            uint64_t nextSlot(~0);

            _adminLock.Lock();

            DestructorMap::iterator index = _destructors.begin();

            while (index != _destructors.end()) {
                if (index->second->Destruct(now) == false) {
                    uint64_t newTime = index->second->Time();
                    if (nextSlot > newTime) {
                        nextSlot = newTime;
                    }
                    index++;
                }
                else {
                    delete index->second;
                    index = _destructors.erase(index);
                }
            }

            _adminLock.Unlock();

            if (nextSlot != static_cast<uint64_t>(~0)) {
                if (now > nextSlot) {
                    delay = 0;
                }
                else {
                    delay = static_cast<uint32_t>((nextSlot - now) / Core::Time::TicksPerMillisecond);
                }
            }

            if (delay > 0) {
                Core::Thread::Block();
            }

            return (delay);
        }

    private:
        Core::CriticalSection _adminLock;
        DestructorMap _destructors;
    };

    static ProcessShutdown& g_destructor = Core::SingletonType<ProcessShutdown>::Instance();

    class LocalClosingInfo : public ProcessShutdown::Destructor {
    public:
        LocalClosingInfo() = delete;
        LocalClosingInfo(const LocalClosingInfo& copy) = delete;
        LocalClosingInfo& operator=(const LocalClosingInfo& RHS) = delete;

        explicit LocalClosingInfo(const uint32_t pid)
            : ProcessShutdown::Destructor()
            , _process(false, pid)
        {
        }
        ~LocalClosingInfo() override = default;

    private:
        uint32_t Terminate() override
        {
            uint32_t nextinterval = 0;
            if (_process.IsActive() != false) {
                switch (Cycle()) {
                case 0:
                    _process.Kill(false);
                    nextinterval = Communicator::SoftKillCheckWaitTime();
                    break;
                default:
                    _process.Kill(true);
                    nextinterval = Communicator::HardKillCheckWaitTime();
                    break;
                }
            }
            return nextinterval;
        }

    private:
        Core::Process _process;
    };

#ifdef PROCESSCONTAINERS_ENABLED

    class ContainerClosingInfo : public ProcessShutdown::Destructor {
    public:
        ContainerClosingInfo() = delete;
        ContainerClosingInfo(const ContainerClosingInfo& copy) = delete;
        ContainerClosingInfo& operator=(const ContainerClosingInfo& RHS) = delete;

        explicit ContainerClosingInfo(ProcessContainers::IContainer* container)
            : ProcessShutdown::Destructor()
            , _process(static_cast<uint32_t>(container->Pid()))
            , _container(container)
        {
            container->AddRef();
        }
        ~ContainerClosingInfo() override
        {
            _container->Release();
        }

    private:
        uint32_t Terminate() override
        {
            uint32_t nextinterval = 0;
            if (((_process.Id() != 0) && (_process.IsActive() == true)) || ((_process.Id() == 0) && (_container->IsRunning() == true))) {
                switch (Cycle()) {
                case 0: if (_process.Id() != 0) {
                            _process.Kill(false);
                        } else {
                            if(_container->Stop(0)) {
                                nextinterval = 0;
                                break;
                            }
                        }
                        nextinterval = Communicator::SoftKillCheckWaitTime();
                        break;
                case 1: if (_process.Id() != 0) {
                            _process.Kill(true);
                            nextinterval = Communicator::HardKillCheckWaitTime();
                        } else {
                            ASSERT(false);
                            nextinterval = 0;
                        }
                        break;
                case 2: _container->Stop(0);
                        nextinterval = 5;
                        break;
                default:
                    // This should not happen. This is a very stubbern process. Can not be killed.
                    ASSERT(false);
                    break;
                }
            }
            return nextinterval;
        }

    private:
        Core::Process _process;
        ProcessContainers::IContainer* _container;
    };

#endif

    /* static */ std::atomic<uint32_t> Communicator::RemoteConnection::_sequenceId(1);

    static void LoadProxyStubs(const string& pathName)
    {
        static std::list<Core::Library> processProxyStubs;

        Core::Directory index(pathName.c_str(), _T("*.so"));

        while (index.Next() == true) {
            // Check if this ProxySTub file is already loaded in this process space..
            std::list<Core::Library>::const_iterator loop(processProxyStubs.begin());
            while ((loop != processProxyStubs.end()) && (loop->Name() != index.Current())) {
                loop++;
            }

            if (loop == processProxyStubs.end()) {
                Core::Library library(index.Current().c_str());

                if (library.IsLoaded() == true) {
                    processProxyStubs.push_back(library);
                }
            }
        }
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::Id() const
    {
        return (_id);
    }

    /* virtual */ void* Communicator::RemoteConnection::Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        void* result(nullptr);

        if (_channel.IsValid() == true) {
            Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

            TRACE_L1("Aquiring object through RPC: %s, 0x%04X [%d]", className.c_str(), interfaceId, RemoteId());

            message->Parameters().Set(_id, className, interfaceId, version);

            uint32_t feedback = _channel->Invoke(message, waitTime);

            if (feedback == Core::ERROR_NONE) {
                instance_id implementation = message->Response().Implementation();

                if (implementation) {
                    // From what is returned, we need to create a proxy
                    RPC::Administrator::Instance().ProxyInstance(Core::ProxyType<Core::IPCChannel>(_channel), implementation, true, interfaceId, result);
                }
            }
        }

        return (result);
    }

    /* virtual */ void Communicator::RemoteConnection::Terminate()
    {
        Close();
    }

    /* virtual */ uint32_t Communicator::RemoteConnection::RemoteId() const
    {
        return (_remoteId);
    }

    /* virtual */ void Communicator::LocalProcess::Terminate()
    {
        // Do not yet call the close on the connection, the otherside might close down decently and release all opened interfaces..
        // Just submit our selves for destruction !!!!

        // Time to shoot the application, it will trigger a close by definition of the channel, if it is still standing..
        ASSERT(_id != 0);
        g_destructor.Destruct<LocalClosingInfo>(Id(), _id);
    }

    uint32_t Communicator::LocalProcess::RemoteId() const
    {
        return (_id);
    }

    void Communicator::LocalProcess::PostMortem() /* override */
    {
        if (_id != 0) {
            Core::ProcessInfo process(_id);
            process.Dump();
        }
    }


#ifdef PROCESSCONTAINERS_ENABLED

    void Communicator::ContainerProcess::Terminate() /* override */
    {
        ASSERT(_container != nullptr);
        g_destructor.Destruct<ContainerClosingInfo>(Id(), _container);
    }

    void Communicator::ContainerProcess::PostMortem() /* override */
    {
        Core::process_t pid;
        if ( (_container != nullptr) && ((pid = static_cast<Core::process_t>(_container->Pid())) != 0) ) {
            Core::ProcessInfo process(pid);
            process.Dump();
        }
    }

#endif
    // Definitions of static members
    uint8_t Communicator::_softKillCheckWaitTime = 10;
    uint8_t Communicator::_hardKillCheckWaitTime = 4;;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

    Communicator::Communicator(const Core::NodeId& node, const string& proxyStubPath)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath)
    {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    Communicator::Communicator(
        const Core::NodeId& node,
        const string& proxyStubPath,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : _connectionMap(*this)
        , _ipcServer(node, _connectionMap, proxyStubPath, handler)
    {
        if (proxyStubPath.empty() == false) {
            RPC::LoadProxyStubs(proxyStubPath);
        }
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
        _ipcServer.CreateFactory<InvokeMessage>(3);
    }

    /* virtual */ Communicator::~Communicator()
    {
        // Make sure any closed channel is cleared before we start validating the end result :-)
        _ipcServer.Cleanup();

        // Close all communication paths...
        _ipcServer.Close(Core::infinite);

        // Warn but we need to clos up existing connections..
        _connectionMap.Destroy();
    }

    void Communicator::Destroy(const uint32_t id) {
        // This is a forceull call, blocking, to kill that specific connection
        g_destructor.ForceDestruct(id);
    }

    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
        , _connectionId(~0)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        Register(RPC::InvokeMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<InvokeHandlerImplementation>::Create()));
        Register(RPC::AnnounceMessage::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<AnnounceHandlerImplementation>::Create(this)));
    }

    CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode,
        const Core::ProxyType<Core::IIPCServer>& handler)
        : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
        , _handler(this)
        , _connectionId(~0)
    {
        CreateFactory<RPC::AnnounceMessage>(1);
        CreateFactory<RPC::InvokeMessage>(2);

        BaseClass::Register(RPC::InvokeMessage::Id(), handler);
        BaseClass::Register(RPC::AnnounceMessage::Id(), handler);
    }
POP_WARNING()

    CommunicatorClient::~CommunicatorClient()
    {
        BaseClass::Close(Core::infinite);

        BaseClass::Unregister(RPC::InvokeMessage::Id());
        BaseClass::Unregister(RPC::AnnounceMessage::Id());

        DestroyFactory<RPC::InvokeMessage>();
        DestroyFactory<RPC::AnnounceMessage>();
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        //do not set announce parameters, we do not know what side will offer the interface
        _announceMessage->Parameters().Set(Core::ProcessInfo().Id());

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), className, interfaceId, version);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation, const uint32_t exchangeId)
    {
        ASSERT(BaseClass::IsOpen() == false);
        _announceEvent.ResetEvent();

        instance_id impl = instance_cast<void*>(implementation);

        _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), interfaceId, impl, exchangeId);

        uint32_t result = BaseClass::Open(waitTime);

        if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
            result = Core::ERROR_OPENING_FAILED;
        }

        return (result);
    }

    uint32_t CommunicatorClient::Close(const uint32_t waitTime)
    {
        return (BaseClass::Close(waitTime));
    }

    /* virtual */ void CommunicatorClient::StateChange()
    {
        BaseClass::StateChange();

        if (BaseClass::Source().IsOpen()) {
            TRACE_L1("Invoking the Announce message to the server. %d", __LINE__);
            uint32_t result = Invoke<RPC::AnnounceMessage>(_announceMessage, this);

            if (result != Core::ERROR_NONE) {
                TRACE_L1("Error during invoke of AnnounceMessage: %d", result);
            } else {
                RPC::Data::Init& setupFrame(_announceMessage->Parameters());

                if (setupFrame.IsRequested() == true) {
                    Core::ProxyType<Core::IPCChannel> refChannel(*this);

                    ASSERT(refChannel.IsValid());

                    // Register the interface we are passing to the otherside:
                    RPC::Administrator::Instance().RegisterInterface(refChannel, reinterpret_cast<void*>(setupFrame.Implementation()), setupFrame.InterfaceId());
                }
            }
        } else {
            TRACE_L1("Connection to the server is down");
        }
    }

    /* virtual */ void CommunicatorClient::Dispatch(Core::IIPC& element)
    {
        // Message delivered and responded on....
        RPC::AnnounceMessage* announceMessage = static_cast<RPC::AnnounceMessage*>(&element);

        ASSERT(dynamic_cast<RPC::AnnounceMessage*>(&element) != nullptr);

        if (announceMessage->Response().IsSet() == true) {
            string jsonMessagingCategories(announceMessage->Response().MessagingCategories());
            if (!jsonMessagingCategories.empty()) {
#if defined(__CORE_MESSAGING__)
                Core::Messaging::MessageUnit::Instance().Defaults(jsonMessagingCategories);
#else
                Trace::TraceUnit::Instance().Defaults(jsonMessagingCategories);
#endif
            }

#if defined(WARNING_REPORTING_ENABLED)
            string jsonDefaultWarningCategories(announceMessage->Response().WarningReportingCategories());
            if(jsonDefaultWarningCategories.empty() == false){
                WarningReporting::WarningReportingUnit::Instance().Defaults(jsonDefaultWarningCategories);
            }
#endif
            _connectionId = announceMessage->Response().SequenceNumber();

            string proxyStubPath(announceMessage->Response().ProxyStubPath());
            if (proxyStubPath.empty() == false) {
                // Also load the ProxyStubs before we do anything else
                RPC::LoadProxyStubs(proxyStubPath);
            }
        }

        // Set event so WaitForCompletion() can continue.
        _announceEvent.SetEvent();
    }

    //We may eliminate the following statement when switched to C++17 compiler
    constexpr uint32_t RPC::ProcessShutdown::DestructionStackSize;

}
}
