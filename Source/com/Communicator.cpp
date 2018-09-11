#include "Communicator.h"

namespace WPEFramework {
namespace RPC {
    static Core::ProxyPoolType<RPC::ObjectMessage> ObjectMessageFactory(2);

	static void LoadProxyStubs(const string & pathName)
	{
		static std::list<Core::Library> processProxyStubs;

		Core::Directory index(pathName.c_str(), _T("*.so"));

		while (index.Next() == true)
		{
			// Check if this ProxySTub file is already loaded in this process space..
			std::list<Core::Library>::const_iterator loop(processProxyStubs.begin());
			while ((loop != processProxyStubs.end()) && (loop->Name() != index.Current())) {
				loop++;
			}

			if (loop == processProxyStubs.end()) {
				Core::Library library(index.Current().c_str());

				if (library.IsLoaded() == true)
				{
					processProxyStubs.push_back(library);
				}
			}
		}
	}

    void* Communicator::RemoteProcess::Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t versionId)
    {
        void* result(nullptr);

        if ((_process.IsActive() == true) && (_channel.IsValid() == true)) {
            Core::ProxyType<RPC::ObjectMessage> message(ObjectMessageFactory.Element());

            message->Parameters().Set(className, versionId, interfaceId);

            uint32_t feedback = _channel->Invoke(message, waitTime);

            if (feedback == Core::ERROR_NONE)
            {
                result = Administrator::Instance().CreateProxy(interfaceId, _channel, message->Response().Value(),
                                                               false, true);
            }
        }

        return (result);
    }

    Communicator::Communicator(const Core::NodeId & node, const Core::ProxyType<Core::IIPCServer> & handler, const string& proxyStubPath)
        : _processMap(*this)
        , _ipcServer(node, _processMap, handler, proxyStubPath)
    {
		if (proxyStubPath.empty() == false) {
			RPC::LoadProxyStubs(proxyStubPath);
		}
        // These are the elements we are expecting to receive over the IPC channels.
        _ipcServer.CreateFactory<AnnounceMessage>(1);
		_ipcServer.CreateFactory<ObjectMessage>(1);
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
		_ipcServer.DestroyFactory<ObjectMessage>();
		_ipcServer.DestroyFactory<AnnounceMessage>();

        TRACE_L1("Clearing Communicator. Active Processes %d", _processMap.Size());
    }

    void Communicator::RemoteProcess::Terminate()
    {

        ASSERT(_parent != nullptr);

        if (_parent != nullptr) {
            _parent->Destroy(_process.Id());
        }
    }

    CommunicatorClient::CommunicatorClient(const Core::NodeId& node)
        : Core::IPCChannelClientType<Core::Void, false, true>(node, CommunicationBufferSize)
        , _announceMessage(Core::ProxyType<RPC::AnnounceMessage>::Create())
        , _announceEvent(false, true)
    {
    }

    CommunicatorClient::~CommunicatorClient()
    {
        BaseClass::Close(Core::infinite);
    }

    void* CommunicatorClient::Create(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t versionId)
    {
        void* result(nullptr);

        ASSERT(className.empty() == false);

        if (BaseClass::IsOpen() == true) {
            Core::ProxyType<RPC::ObjectMessage> message(ObjectMessageFactory.Element());

            message->Parameters().Set(className, versionId, interfaceId);

            Core::ProxyType<Core::IIPC> baseMessage(Core::proxy_cast<Core::IIPC>(message));

            BaseClass::Invoke(baseMessage, waitTime);

            void* implementation(message->Response().Value());

            if (implementation != nullptr) {
                Core::ProxyType<Core::IPCChannel> baseChannel(*this);

                ASSERT (baseChannel.IsValid() == true);

                result = Administrator::Instance().CreateProxy(interfaceId, baseChannel, message->Response().Value(), false, true);

                // The other side can not do anything with this
                if (result == nullptr) {
                    // TODO: Cleanup the otherside if this fails...
                }
            }
        }

        return (result);
    }

	uint32_t CommunicatorClient::Open(const uint32_t waitTime)
	{
        	ASSERT(BaseClass::IsOpen() == false);

		_announceMessage->Parameters().Set(~0, nullptr);

        	return (BaseClass::Open(waitTime));
	}

	uint32_t CommunicatorClient::Open(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
	{
        ASSERT(BaseClass::IsOpen() == false);

		_announceMessage->Parameters().Set(className, interfaceId, version);

        return (BaseClass::Open(waitTime));
    }

    uint32_t CommunicatorClient::Open(const uint32_t waitTime, const uint32_t interfaceId, void * implementation)
    {
        ASSERT(BaseClass::IsOpen() == false);

        _announceMessage->Parameters().Set(interfaceId, implementation);

        return (BaseClass::Open(waitTime));
    }

    uint32_t CommunicatorClient::Close(const uint32_t waitTime)
    {
        return (BaseClass::Close(waitTime));
    }

    /* virtual */ void CommunicatorClient::StateChange()
    {
        BaseClass::StateChange();

        if (BaseClass::Source().IsOpen())
        {
            TRACE_L1("Invoking the Announce message to the server. %d", __LINE__);

            uint32_t result = Invoke<RPC::AnnounceMessage>(_announceMessage, this);

            if (result != Core::ERROR_NONE)
            {
                TRACE_L1("Error during invoke of AnnounceMessage: %d", result);
            }
        }
    }

    /* virtual */ void CommunicatorClient::Dispatch(Core::IIPC & element)
    {
        // Message delivered and responded on....
        RPC::AnnounceMessage * announceMessage = static_cast<RPC::AnnounceMessage *>(&element);

        ASSERT(dynamic_cast<RPC::AnnounceMessage *>(&element) != nullptr);

        // Is result of an announce message, contains default trace categories in JSON format.
        string jsonDefaultCategories(announceMessage->Response().TraceCategories());
        Trace::TraceUnit::Instance().SetDefaultCategoriesJson(jsonDefaultCategories);

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
