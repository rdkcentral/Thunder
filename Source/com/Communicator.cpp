#include "Communicator.h"

#include <limits>
#include <memory>

namespace WPEFramework {
namespace RPC {

class ProcessShutdown;

static constexpr uint32_t DestructionStackSize = 64 * 1024;
static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);
static Core::TimerType<ProcessShutdown>& _destructor = Core::SingletonType < Core::TimerType<ProcessShutdown> >::Instance(DestructionStackSize, "ProcessDestructor");

class ClosingInfo {
public:
    ClosingInfo& operator=(const ClosingInfo& RHS) = delete;
    ClosingInfo(const ClosingInfo& copy) = delete;

    virtual ~ClosingInfo() = default;

protected:
    ClosingInfo() = default;

public:
    virtual uint32_t AttemptClose(const uint8_t iteration) = 0; //shoud return 0 if no more iterations needed
};

class ProcessShutdown  {
public:
    template<class IMPLEMENTATION, typename... Args>
    static void Start(Args... args) {

        std::unique_ptr<IMPLEMENTATION> handler( new IMPLEMENTATION(args...) );

        uint32_t nextinterval = handler->AttemptClose(0);

        if( nextinterval != 0 ) {
            _destructor.Schedule(Core::Time::Now().Add(10000), ProcessShutdown(std::move(handler)));
        }
    }

public:
    ProcessShutdown& operator=(const ProcessShutdown& RHS) = delete;
    ProcessShutdown(const ProcessShutdown& copy) = delete;

    ProcessShutdown(ProcessShutdown&& rhs)
        : _handler(std::move(rhs._handler))
        , _cycle(rhs._cycle) {
    }

    explicit ProcessShutdown(std::unique_ptr<ClosingInfo>&& handler) 
        : _handler(std::move(handler))
        , _cycle(1)
    {
    }

    ~ProcessShutdown() = default;

public:
    uint64_t Timed(const uint64_t scheduledTime)
    {
        uint64_t result = 0;

        uint32_t nextinterval = _handler->AttemptClose(_cycle++);

        uint32_t test = std::numeric_limits<uint8_t>::max();

        if ( nextinterval != 0 ) {
            result = Core::Time(scheduledTime).Add(nextinterval).Ticks(); 
        } 
        else  {
            ASSERT( _cycle != std::numeric_limits<uint8_t>::max() ); //too many attempts trying to kill the process
        }

        return (result);
    }

private:
    std::unique_ptr<ClosingInfo> _handler;
    uint8_t _cycle;
};

class LocalClosingInfo : public ClosingInfo {
public:
    LocalClosingInfo& operator=(const LocalClosingInfo& RHS) = delete;
    LocalClosingInfo(const LocalClosingInfo& copy) = delete;

    virtual ~LocalClosingInfo() = default;

private:
    friend class ProcessShutdown;

    explicit LocalClosingInfo(const uint32_t pid)
        : ClosingInfo()
        , _process(pid)
    {
    }

protected:
    uint32_t AttemptClose(const uint8_t iteration) override {
        uint32_t nextinterval = 0;
        if( _process.IsActive() != false ) {
            switch( iteration ) {
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

class ContainerClosingInfo : public ClosingInfo {
public:
    ContainerClosingInfo& operator=(const ContainerClosingInfo& RHS) = delete;
    ContainerClosingInfo(const ContainerClosingInfo& copy) = delete;

    virtual ~ContainerClosingInfo() {
        _container->Release();
    }

private:
    friend class ProcessShutdown;

    explicit ContainerClosingInfo(ProcessContainers::IContainerAdministrator::IContainer* container)
        : ClosingInfo()
        , _pid(static_cast<uint32_t>(container->Pid()))
        , _process(_pid)
        , _container(container)
    {
        container->AddRef();
    }

protected:
    uint32_t AttemptClose(const uint8_t iteration) override {
        uint32_t nextinterval = 0;
        if( _container->IsRunning() == true ) {
            switch( iteration ) {
                case 0:
                    {
                        if( _pid != 0 ) {
                            _process.Kill(false);
                        } else {
                            _container->Stop(0);
                        }
                    }
                    nextinterval = 10000;
                break;
                case 1:
                    {
                        if( _pid != 0 ) {
                            _process.Kill(true);
                            nextinterval = 4000;
                        } else {
                            ASSERT(false);
                            nextinterval = 0;
                        }
                    }
                break;
                case 2:
                    _container->Stop(0);
                    nextinterval = 5000;
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
    uint32_t _pid;
    Core::Process _process;
    ProcessContainers::IContainerAdministrator::IContainer* _container;
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

/* virtual */ void* Communicator::RemoteConnection::QueryInterface(const uint32_t id)
{
    if (id == IRemoteConnection::ID) {
        AddRef();
        return (static_cast<IRemoteConnection*>(this));
    } else if (id == Core::IUnknown::ID) {
        AddRef();
        return (static_cast<Core::IUnknown*>(this));		
	}
    return (nullptr);
}

/* virtual */ void* Communicator::RemoteConnection::Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
{
    void* result(nullptr);

    if (_channel.IsValid() == true) {
        Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

        message->Parameters().Set(_id, className, interfaceId, version);

        uint32_t feedback = _channel->Invoke(message, waitTime);

        if (feedback == Core::ERROR_NONE) {
            void* implementation = message->Response().Implementation();

            if (implementation != nullptr) {
                // From what is returned, we need to create a proxy
                ProxyStub::UnknownProxy* instance = RPC::Administrator::Instance().ProxyInstance(_channel, implementation, interfaceId, true, interfaceId, false);
                result = (instance != nullptr ? instance->QueryInterface(interfaceId) : nullptr);
            }
        }
    }

    return (result);
}

/* virtual */ void Communicator::RemoteConnection::Terminate()
{
    Close();
}

/* virtual */ uint32_t Communicator::RemoteConnection::RemoteId() const  {
    return (_remoteId);
}

/* virtual */ void Communicator::LocalRemoteProcess::Terminate()
{
    // Do not yet call the close on the connection, the otherside might close down decently and release all opened interfaces..
    // Just submit our selves for destruction !!!!

    // Time to shoot the application, it will trigger a close by definition of the channel, if it is still standing..
    if (_id != 0) {
        ProcessShutdown::Start<LocalClosingInfo>(_id);
        _id = 0;
    }
}

uint32_t Communicator::LocalRemoteProcess::RemoteId() const  {
    return (_id);
}

#ifdef PROCESSCONTAINERS_ENABLED 

void Communicator::ContainerRemoteProcess::Terminate()
{
    ASSERT(_container != nullptr);
    if( _container != nullptr ) {
        ProcessShutdown::Start<ContainerClosingInfo>(_container);
    }
}

#endif

Communicator::RemoteHost::RemoteHost(const Core::NodeId& remoteNode)
    : RemoteProcess()
{
}

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
    const Core::ProxyType<ServerType<InvokeMessage> >& invoke,
    const Core::ProxyType<ServerType<AnnounceMessage> >& announcement) 
    : _connectionMap(*this)
    , _ipcServer(node, _connectionMap, proxyStubPath, invoke, announcement)
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

    _ipcServer.DestroyFactory< InvokeMessage>();
    _ipcServer.DestroyFactory< AnnounceMessage>();
}

CommunicatorClient::CommunicatorClient(
    const Core::NodeId& remoteNode)
    : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
    , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
    , _announceEvent(false, true)
    , _invokeHandler(Core::ProxyType<InvokeHandlerImplementation>::Create())
    , _announceHandler(Core::ProxyType<AnnounceHandlerImplementation>::Create(this))
{
    CreateFactory<RPC::AnnounceMessage>(1);
    CreateFactory<RPC::InvokeMessage>(2);

    Register(_invokeHandler);
    Register(_announceHandler);
}

CommunicatorClient::CommunicatorClient(
        const Core::NodeId& remoteNode,
	    const Core::ProxyType< ServerType<InvokeMessage> >& invoke,
	    const Core::ProxyType< ServerType<AnnounceMessage> >& announce)
    : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
    , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
    , _announceEvent(false, true)
    , _invokeHandler()
    , _announceHandler()
{
    CreateFactory<RPC::AnnounceMessage>(1);
    CreateFactory<RPC::InvokeMessage>(2);
    Core::ProxyType<Core::IPCServerType<InvokeMessage>> invokeHandler(Core::ProxyType<InvokeHandlerImplementation>::Create());
    Core::ProxyType<Core::IPCServerType<AnnounceMessage>> announceHandler(Core::ProxyType<AnnounceHandlerImplementation>::Create(this));

    if (invoke.IsValid() == true) {
        _invokeHandler = invoke;
        invoke->Handler(invokeHandler);
    } else {
        _invokeHandler = invokeHandler;
    }

    if (announce.IsValid() == true) {
        _announceHandler = announce;
        announce->Handler(announceHandler);
    } else {
        _announceHandler = announceHandler;
    }

    BaseClass::Register(_invokeHandler);
    BaseClass::Register(_announceHandler);
}

CommunicatorClient::~CommunicatorClient()
{
    BaseClass::Close(Core::infinite);

    DestroyFactory<RPC::InvokeMessage>();
    DestroyFactory<RPC::AnnounceMessage>();
}

uint32_t CommunicatorClient::Open(const uint32_t waitTime)
{
    ASSERT(BaseClass::IsOpen() == false);
    _announceEvent.ResetEvent();

    //do not set announce parameters, we do not know what side will offer the interface

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

    _announceMessage->Parameters().Set(Core::ProcessInfo().Id(), interfaceId, implementation, Data::Init::REQUEST, exchangeId);

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
                RPC::Administrator::Instance().RegisterInterface(refChannel, setupFrame.Implementation(), setupFrame.InterfaceId());
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
        // Is result of an announce message, contains default trace categories in JSON format.
        string jsonDefaultCategories(announceMessage->Response().TraceCategories());

        if (jsonDefaultCategories.empty() == false) {
            Trace::TraceUnit::Instance().SetDefaultCategoriesJson(jsonDefaultCategories);
        }

        string proxyStubPath(announceMessage->Response().ProxyStubPath());
        if (proxyStubPath.empty() == false) {
            // Also load the ProxyStubs before we do anything else
            RPC::LoadProxyStubs(proxyStubPath);
        }
    }

    // Set event so WaitForCompletion() can continue.
    _announceEvent.SetEvent();
}
}
}
