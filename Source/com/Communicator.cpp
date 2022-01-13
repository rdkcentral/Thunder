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

    class ProcessShutdown {
    public:
        static constexpr uint32_t DestructionStackSize = 64 * 1024;

        class IClosingInfo {
        public:
            virtual ~IClosingInfo() = default;

            // Should return 0 if no more iterations are needed.
            virtual uint32_t AttemptClose(const uint8_t iteration) = 0;
        };

    public:
        ProcessShutdown() = delete;
        ProcessShutdown& operator=(const ProcessShutdown& RHS) = delete;
        ProcessShutdown(const ProcessShutdown& copy) = delete;

        ProcessShutdown(ProcessShutdown&& rhs)
            : _handler(std::move(rhs._handler))
            , _cycle(rhs._cycle)
        {
        }

        explicit ProcessShutdown(std::unique_ptr<IClosingInfo>&& handler)
            : _handler(std::move(handler))
            , _cycle(1)
        {
        }
        ~ProcessShutdown() = default;

    public:
        template <class IMPLEMENTATION, typename... Args>
        static void Start(Args... args)
        {

            std::unique_ptr<IMPLEMENTATION> handler(new IMPLEMENTATION(args...));

            uint32_t nextinterval = handler->AttemptClose(0);

            if (nextinterval != 0) {
                _destructor.Schedule(Core::Time::Now().Add(nextinterval), ProcessShutdown(std::move(handler)));
            }
        }
        uint64_t Timed(const uint64_t scheduledTime)
        {
            uint64_t result = 0;

            uint32_t nextinterval = _handler->AttemptClose(_cycle++);

            if (nextinterval != 0) {
                result = Core::Time(scheduledTime).Add(nextinterval).Ticks();
            } else {
                ASSERT(_cycle != std::numeric_limits<uint8_t>::max()); //too many attempts trying to kill the process
            }

            return (result);
        }

    private:
        std::unique_ptr<IClosingInfo> _handler;
        uint8_t _cycle;
        static Core::TimerType<ProcessShutdown>& _destructor;
    };

    /* static */ Core::TimerType<ProcessShutdown>& ProcessShutdown::_destructor(Core::SingletonType<Core::TimerType<ProcessShutdown>>::Instance(ProcessShutdown::DestructionStackSize, "ProcessDestructor"));

    class LocalClosingInfo : public ProcessShutdown::IClosingInfo {
    public:
        LocalClosingInfo() = delete;
        LocalClosingInfo(const LocalClosingInfo& copy) = delete;
        LocalClosingInfo& operator=(const LocalClosingInfo& RHS) = delete;

        explicit LocalClosingInfo(const uint32_t pid)
            : _process(false, pid)
        {
        }
        ~LocalClosingInfo() override = default;

    public:
        uint32_t AttemptClose(const uint8_t iteration) override
        {
            uint32_t nextinterval = 0;
            if (_process.IsActive() != false) {
                switch (iteration) {
                case 0:
                    _process.Kill(false);
                    nextinterval = 10000;
                    break;
                case 1:
                    _process.Kill(true);
                    nextinterval = 4000;
                    break;
                default:
                    // This should not happen. This is a very stubbern process. Can be killed.
                    ASSERT(false);
                    break;
                }
            }
            return nextinterval;
        }

    private:
        Core::Process _process;
    };

#ifdef PROCESSCONTAINERS_ENABLED

    class ContainerClosingInfo : public ProcessShutdown::IClosingInfo {
    public:
        ContainerClosingInfo() = delete;
        ContainerClosingInfo(const ContainerClosingInfo& copy) = delete;
        ContainerClosingInfo& operator=(const ContainerClosingInfo& RHS) = delete;

        explicit ContainerClosingInfo(ProcessContainers::IContainer* container)
            : _process(static_cast<uint32_t>(container->Pid()))
            , _container(container)
        {
            container->AddRef();
        }
        ~ContainerClosingInfo() override
        {
            _container->Release();
        }

    public:
        uint32_t AttemptClose(const uint8_t iteration) override
        {
            uint32_t nextinterval = 0;
            if ((_process.Id() != 0 && _process.IsActive() == true) || (_process.Id() == 0 && _container->IsRunning() == true)) {
                switch (iteration) {
                case 0: {
                    if (_process.Id() != 0) {
                        _process.Kill(false);
                    } else {
                        if(_container->Stop(0)) {
                            nextinterval = 0;
                            break;
                        }
                    }
                }
                    nextinterval = 10000;
                    break;
                case 1: {
                    if (_process.Id() != 0) {
                        _process.Kill(true);
                        nextinterval = 4000;
                    } else {
                        ASSERT(false);
                        nextinterval = 0;
                    }
                } break;
                case 2:
                    _container->Stop(0);
                    nextinterval = 5000;
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
        if (_id != 0) {
            if (!_stopInvokedFlag.test_and_set()) {
                ProcessShutdown::Start<LocalClosingInfo>(_id);
            } else {
                TRACE_L1("Process terminate already started");
            }
        }
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
        if (!_stopInvokedFlag.test_and_set()) {
            ProcessShutdown::Start<ContainerClosingInfo>(_container);
        } else {
            TRACE_L1("Containter terminate already started");
        }
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

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif

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
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

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
            if(!jsonMessagingCategories.empty()) {
                Core::Messaging::MessageUnit::Instance().Defaults(jsonMessagingCategories);
            }

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
