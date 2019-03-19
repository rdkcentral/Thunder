#include "Communicator.h"

namespace WPEFramework {
namespace RPC {
    static Core::ProxyPoolType<RPC::AnnounceMessage> AnnounceMessageFactory(2);

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

    void* Communicator::RemoteProcess::Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t versionId)
    {
        void* result(nullptr);

        if (_channel.IsValid() == true) {
            Core::ProxyType<RPC::AnnounceMessage> message(AnnounceMessageFactory.Element());

            message->Parameters().Set(className, interfaceId, versionId);

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

    Communicator::Communicator(const Core::NodeId& node, Core::ProxyType<IHandler> handler, const string& proxyStubPath)
        : _processMap(*this)
        , _ipcServer(node, _processMap, handler, proxyStubPath)
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

        // All process must be terminated if we end up here :-)
        ASSERT(_processMap.Size() == 0);

        // Close all communication paths...
        _ipcServer.Close(Core::infinite);

        _ipcServer.DestroyFactory<InvokeMessage>();
        _ipcServer.DestroyFactory<AnnounceMessage>();

        TRACE_L1("Clearing Communicator. Active Processes %d", _processMap.Size());
    }

    void Communicator::RemoteProcess::Terminate()
    {
        ASSERT(_parent != nullptr);

        if (_parent != nullptr) {
            _parent->Destroy(Id());
        }
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

        // For now clients do not support announce messages from the server...
        // Register(_handler->AnnounceHandler());
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

            TRACE_L1("Todo Announce message handled. %d", __LINE__);

            if (result != Core::ERROR_NONE) {
                TRACE_L1("Error during invoke of AnnounceMessage: %d", result);
            }
        } else {
            TRACE_L1("Connection to the server is down (ticket has been raised: WPE-255)");
        }
    }

    /* virtual */ void CommunicatorClient::Dispatch(Core::IIPC& element)
    {
        // Message delivered and responded on....
        RPC::AnnounceMessage* announceMessage = static_cast<RPC::AnnounceMessage*>(&element);

        ASSERT(dynamic_cast<RPC::AnnounceMessage*>(&element) != nullptr);

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

        // Set event so WaitForCompletion() can continue.
        _announceEvent.SetEvent();
    }
}
}
