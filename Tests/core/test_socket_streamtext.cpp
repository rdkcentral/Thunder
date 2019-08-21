#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

using namespace WPEFramework;
namespace StreamTextTest {
    const TCHAR* g_connector = "/tmp/wpestreamtext0";
    bool g_done = false;

    class TextConnector : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> BaseClass;

        TextConnector();
        TextConnector(const TextConnector& copy);
        TextConnector& operator=(const TextConnector&);

    public:
        TextConnector(const WPEFramework::Core::NodeId& remoteNode)
            : BaseClass(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
        {
        }
        TextConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<TextConnector>*)
            : BaseClass(false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
        {
        }
        virtual ~TextConnector()
        {
        }

    public:
        virtual void Received(string& text)
        {
            if (_serverSocket)
                Submit(text);
            else {
                _dataReceived = text;
                _dataPending.SetEvent();
            }
        }
        int Wait(unsigned int milliseconds) const
        {
            return _dataPending.Lock(milliseconds);
        }
        void Retrieve(string& text)
        {
            text = _dataReceived;
            _dataReceived.clear();
        }
        virtual void Send(const string& text)
        {
        }
        virtual void StateChange()
        {
            if (IsOpen()) {
                if (_serverSocket)
                    g_done = true;
            }
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable WPEFramework::Core::Event _dataPending;
    };
}

TEST(Core_Socket, StreamText)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::SocketServerType<StreamTextTest::TextConnector> textSocketServer(Core::NodeId(StreamTextTest::g_connector));
        textSocketServer.Open(Core::infinite);
        testAdmin.Sync("setup server");
        while(!StreamTextTest::g_done);
        testAdmin.Sync("server open");
        testAdmin.Sync("client done");
    };

    IPTestAdministrator testAdmin(otherSide);
    testAdmin.Sync("setup server");
    {
        StreamTextTest::TextConnector textSocketClient(Core::NodeId(StreamTextTest::g_connector));
        textSocketClient.Open(Core::infinite);
        testAdmin.Sync("server open");
        string message = "hello";
        textSocketClient.Submit(message);
        textSocketClient.Wait(2000);
        string received;
        textSocketClient.Retrieve(received);
        ASSERT_STREQ(message.c_str(), received.c_str());
        textSocketClient.Close(Core::infinite);
        testAdmin.Sync("client done");
    }
    Core::Singleton::Dispose();
}
