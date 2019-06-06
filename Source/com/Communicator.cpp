#include "Communicator.h"

namespace WPEFramework {
namespace RPC {

class ClosingInfo {
    private:
        ClosingInfo() = delete;
        ClosingInfo& operator=(const ClosingInfo& RHS) = delete;

        enum enumState {
            SOFTKILL,
            HARDKILL
        };

    public:
        ClosingInfo(const uint32_t pid)
            : _process(pid)
            , _state(SOFTKILL)
        {
            _process.Kill(false);
        }
        ClosingInfo(const ClosingInfo& copy)
            : _process(copy._process.Id())
            , _state(copy._state)
        {
        }
        ~ClosingInfo()
        {
        }

    public:
        uint64_t Timed(const uint64_t scheduledTime)
        {
            uint64_t result = 0;

            if (_process.IsActive() != false) {
				if (_state == SOFTKILL) {
					_state = HARDKILL;
					_process.Kill(true);
					result = Core::Time(scheduledTime).Add(4000).Ticks(); // Next check in 4S
				} else {
					// This should not happen. This is a very stubbern process. Can be killed.
					ASSERT(false);
				}
			}

			return (result);
		}

		private : Core::Process _process;
		enumState _state;
	};

static constexpr uint32_t DestructionStackSize = 64 * 1024;
static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);
static Core::TimerType<ClosingInfo> _destructor(DestructionStackSize, "ProcessDestructor");

/* static */ std::atomic<uint32_t> Communicator::RemoteConnection::_sequenceId;

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
    } else {
        assert(false);
    }
    return (nullptr);
}

/* virtual */ void* Communicator::RemoteConnection::Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
{
    void* result(nullptr);

    if (_channel.IsValid() == true) {
        Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

        message->Parameters().Set(className, interfaceId, version);

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
    if (_channel.IsValid() == true) {
        _channel->Source().Close(0);
    }
}

/* virtual */ void Communicator::RemoteProcess::Terminate()
{
	// Time to shoot the application, it will trigger a close by definition of the channel, if it is still standing..
    _destructor.Schedule(Core::Time::Now().Add(10000), ClosingInfo(_id));
}

Communicator::Communicator(const Core::NodeId& node, Core::ProxyType<IHandler> handler, const string& proxyStubPath)
    : _connectionMap(*this)
    , _ipcServer(node, _connectionMap, handler, proxyStubPath)
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

    _ipcServer.DestroyFactory<InvokeMessage>();
    _ipcServer.DestroyFactory<AnnounceMessage>();
}

CommunicatorClient::CommunicatorClient(const Core::NodeId& remoteNode, Core::ProxyType<IHandler> handler)
    : Core::IPCChannelClientType<Core::Void, false, true>(remoteNode, CommunicationBufferSize)
    , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
    , _announceEvent(false, true)
    , _handler(handler)
    , _announcements(*this)
{
    CreateFactory<RPC::AnnounceMessage>(1);
    CreateFactory<RPC::InvokeMessage>(2);
    Register(_handler->InvokeHandler());
    Register(_handler->AnnounceHandler());
    _handler->AnnounceHandler(&_announcements);
}

CommunicatorClient::~CommunicatorClient()
{
    BaseClass::Close(Core::infinite);

    Unregister(_handler->AnnounceHandler());
    Unregister(_handler->InvokeHandler());
    _handler->AnnounceHandler(nullptr);
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

    _announceMessage->Parameters().Set(className, interfaceId, version);

    uint32_t result = BaseClass::Open(waitTime);

    if ((result == Core::ERROR_NONE) && (_announceEvent.Lock(waitTime) != Core::ERROR_NONE)) {
        result = Core::ERROR_OPENING_FAILED;
    }

    return (result);
}

uint32_t CommunicatorClient::Open(const uint32_t waitTime, const uint32_t interfaceId, void* implementation)
{
    ASSERT(BaseClass::IsOpen() == false);
    _announceEvent.ResetEvent();

    _announceMessage->Parameters().Set(interfaceId, implementation, Data::Init::REQUEST);

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
                Core::ProxyType<Core::IPCChannel> refChannel(dynamic_cast<Core::IReferenceCounted*>(this), this);

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
