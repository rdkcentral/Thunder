#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

using namespace WPEFramework;

namespace TextWebSocketTest {
    bool g_done = false;
    const TCHAR* g_connector = "/tmp/wpewebsocket0";

	class SocketServerLink : public Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> { 
	private:
        typedef Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

	private:
		SocketServerLink(const SocketServerLink&) = delete;
		SocketServerLink& operator=(const SocketServerLink&) = delete;

	public:
		SocketServerLink(const SOCKET& socket, const WPEFramework::Core::NodeId& remoteNode, WPEFramework::Core::SocketServerType<SocketServerLink>*)
            : BaseClass(false, true, false, socket, remoteNode, 1024, 1024) {
		}
		virtual ~SocketServerLink() {
		}

	public:
		virtual void StateChange() {
			if (IsOpen())
                g_done = true;
		}

		virtual void Received(string& text) {
            Submit(text);
		}
        virtual void Send(const string& text) {
		}
	};

    class SocketClientLink : public Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> {
	private:
		typedef Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

	private:
		SocketClientLink(const SocketClientLink&) = delete;
		SocketClientLink& operator=(const SocketClientLink&) = delete;

	public:
        SocketClientLink(const Core::NodeId& remoteNode)
            : BaseClass(_T("/"), _T("echo"), "", "", false, true, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _dataPending(false, false)
		{
		}
		virtual ~SocketClientLink()
		{
		}

	public:
		virtual void Received(string& text)
		{
            _dataReceived = text;
            _dataPending.Unlock();
            
		}
        virtual void Send(const string& text)
		{
		}
		virtual void StateChange()
		{
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
        string _dataReceived;
        mutable WPEFramework::Core::Event _dataPending;
	};
}

TEST(WebSocket, Text)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::SocketServerType<TextWebSocketTest::SocketServerLink> textWebSocketServer(Core::NodeId(TextWebSocketTest::g_connector));
		textWebSocketServer.Open(Core::infinite);
        testAdmin.Sync("setup server");
        while(!TextWebSocketTest::g_done);
        testAdmin.Sync("server open");
        testAdmin.Sync("client done");
    };

    IPTestAdministrator testAdmin(otherSide);
    testAdmin.Sync("setup server");
    {
        TextWebSocketTest::SocketClientLink textWebSocketClient(Core::NodeId(TextWebSocketTest::g_connector));
        textWebSocketClient.Open(Core::infinite);
        testAdmin.Sync("server open");
        string sentString = "Test String";
        textWebSocketClient.Submit(sentString);
        uint32_t result = textWebSocketClient.Wait();
        string received;
        textWebSocketClient.Retrieve(received);
        ASSERT_STREQ(sentString.c_str(), received.c_str());
        textWebSocketClient.Close(Core::infinite);
        testAdmin.Sync("client done");
    }
    Core::Singleton::Dispose();
}

