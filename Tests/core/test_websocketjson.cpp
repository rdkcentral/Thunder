#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

namespace WPEFramework {
namespace Tests {

namespace JsonWebSocketTest {
    bool g_done = false;
    const TCHAR* g_connector = "/tmp/wpewebsocketjson0";
} // JsonWebSocketTest

	class Message : public Core::JSON::Container {
	private:
		Message(const Message&) = delete;
		Message& operator= (const Message&) = delete;

	public:
		Message() :
			Core::JSON::Container(),
			EventType(),
			Event() {
			Add(_T("eventType"), &EventType);
			Add(_T("event"), &Event);
		}
		~Message() {
		}

	public:
		Core::JSON::String EventType;
		Core::JSON::String Event;
	};

	class Factory : public Core::ProxyPoolType<Message> {
	private:
		Factory() = delete;
		Factory(const Factory&) = delete;
		Factory& operator= (const Factory&) = delete;

	public:
		Factory(const uint32_t number) : Core::ProxyPoolType<Message>(number) {
		}
		virtual ~Factory() {
		}

	public:
		Core::ProxyType<Core::JSON::IElement> Element(const string&) {
			return (Core::proxy_cast<Core::JSON::IElement>(Core::ProxyPoolType<Message>::Element()));
		}
	};

    template<typename INTERFACE>
	class JsonSocketServer : public Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> {
	private:
		typedef Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> BaseClass;

	private:
		JsonSocketServer(const JsonSocketServer&) = delete;
		JsonSocketServer& operator=(const JsonSocketServer&) = delete;

	public:
		JsonSocketServer(const SOCKET& socket, const Core::NodeId& remoteNode, Core::SocketServerType<JsonSocketServer<INTERFACE>>*)
			: BaseClass(2, _objectFactory, false, false, false, socket, remoteNode, 512, 512)
			, _objectFactory(1) {
		}
		virtual ~JsonSocketServer() {
		}

	public:
		virtual bool IsIdle() const {
            return (true);
        }
		virtual void StateChange() {
			if (this->IsOpen())
                JsonWebSocketTest::g_done = true;
		}

		bool IsAttached() const {
			return (this->IsOpen());
		}
		virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject) {
            this->Submit(jsonObject);
		}
		virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject) {
		}
	private:
		Factory _objectFactory;
	};

    template<typename INTERFACE>
	class JsonSocketClient : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Factory&, INTERFACE> {
	private:
		typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Factory&, INTERFACE> BaseClass;

	private:
		JsonSocketClient(const JsonSocketClient&) = delete;
		JsonSocketClient& operator=(const JsonSocketClient&) = delete;

	public:
		JsonSocketClient(const Core::NodeId& remoteNode)
			: BaseClass(5, _objectFactory, _T(""), _T(""), _T(""), _T(""), false, true, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
			, _objectFactory(2)
            , _dataPending(false, false)
		{
		}
		virtual ~JsonSocketClient()
		{
		}

	public:
		virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject)
		{
			string textElement;
			jsonObject->ToString(textElement);
            _dataReceived = textElement;
            _dataPending.Unlock();
            
		}
		virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject)
		{
		}
		virtual void StateChange()
		{
		}
		virtual bool IsIdle() const
		{
			return (true);
		}

        Core::ProxyType<Core::JSON::IElement> Element()
        {
            return _objectFactory.Element("");
        } 
        int Wait() const
        {
            return _dataPending.Lock();
        }
        void Retrieve(string& text)
        {
            text = _dataReceived;
            _dataReceived.clear();
        }
	private:
		Factory _objectFactory;
        string _dataReceived;
        mutable Core::Event _dataPending;
	};

    TEST(WebSocket, Json)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<JsonSocketServer<Core::JSON::IElement>> jsonWebSocketServer(Core::NodeId(JsonWebSocketTest::g_connector));
            jsonWebSocketServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            sleep(2);
            while(!JsonWebSocketTest::g_done);
            testAdmin.Sync("server open");
            testAdmin.Sync("client done");
        };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            Core::ProxyType<Message> sendObject = Core::ProxyType<Message>::Create();
            sendObject->EventType = _T("Test");
            sendObject->Event = _T("TestSend");
            std::string sendString;
            sendObject->ToString(sendString);

            JsonSocketClient<Core::JSON::IElement> jsonWebSocketClient(Core::NodeId(JsonWebSocketTest::g_connector));
            jsonWebSocketClient.Open(Core::infinite);
            testAdmin.Sync("server open");
            jsonWebSocketClient.Submit(Core::proxy_cast<Core::JSON::IElement>(sendObject));
            jsonWebSocketClient.Wait();
            string received;
            jsonWebSocketClient.Retrieve(received);
            EXPECT_STREQ(sendString.c_str(), received.c_str());
            jsonWebSocketClient.Close(Core::infinite);
            testAdmin.Sync("client done");
        }
        Core::Singleton::Dispose();
    }
} // Tests
} // WPEFramework
