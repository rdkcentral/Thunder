#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

using namespace WPEFramework;

namespace TextWebLinkTest {
    const TCHAR* g_connector = "0.0.0.0";

    static Core::ProxyPoolType<Web::TextBody> g_textBodyFactory(5);
    static Core::ProxyPoolType<Web::Response> g_responseFactory(5);

    class EXTERNAL WebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, WPEFramework::Core::ProxyPoolType<Web::Request> > {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, WPEFramework::Core::ProxyPoolType<Web::Request> > BaseClass;

        WebServer();
        WebServer(const WebServer& copy);
        WebServer& operator=(const WebServer&);

    public:
        WebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<WebServer>*)
            : BaseClass(5, false, connector, remoteId, 2048, 2048)
        {
        }
        virtual ~WebServer()
        {
            Close(WPEFramework::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<WPEFramework::Web::Request>& element)
        {
            // Time to attach a String Body
            element->Body(g_textBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<WPEFramework::Web::Request>& request)
        {
            ASSERT(request->Verb == Web::Request::HTTP_GET);
            ASSERT(request->MajorVersion == 1);
            ASSERT(request->MinorVersion == 1);
            ASSERT(request->HasBody());
            ASSERT(request->ContentLength.Value() == 19);

            Core::ProxyType<Web::Response> response(Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<Web::TextBody>(request->Body<Web::TextBody>());
            Submit(response);
        }
        virtual void Send(const Core::ProxyType<WPEFramework::Web::Response>& response)
        {
            ASSERT(response->ErrorCode == 200);
            ASSERT(response->HasBody());
        }
        virtual void StateChange()
        {
        }
    };

    class EXTERNAL WebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> BaseClass;

        WebClient();
        WebClient(const WebClient& copy);
        WebClient& operator=(const WebClient&);

    public:
        WebClient(const WPEFramework::Core::NodeId& remoteNode)
            : BaseClass(5, g_responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
        {
        }
        virtual ~WebClient()
        {
            Close(WPEFramework::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<WPEFramework::Web::Response>& element)
        {
            // Time to attach a String Body
            element->Body(g_textBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<WPEFramework::Web::Response>& response)
        {
            ASSERT(response->ErrorCode == 200);
            ASSERT(response->Message == string("OK"));
            ASSERT(response->MajorVersion == 1);
            ASSERT(response->MinorVersion == 1);
            ASSERT(response->HasBody());
            ASSERT(response->ContentLength.Value() == 19);

            _dataReceived = *(response->Body<Web::TextBody>());
            _dataPending.Unlock();
        }
        virtual void Send(const Core::ProxyType<WPEFramework::Web::Request>& request)
        {
            ASSERT(request->Verb == Web::Request::HTTP_GET);
            ASSERT(request->HasBody());
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
        mutable WPEFramework::Core::Event _dataPending;
        string _dataReceived;
    };
}

TEST(WebLink, Text)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::SocketServerType<TextWebLinkTest::WebServer> _webServer(Core::NodeId(TextWebLinkTest::g_connector, 12343));
        _webServer.Open(Core::infinite);
        testAdmin.Sync("setup server");
        testAdmin.Sync("client done");
    };

    IPTestAdministrator testAdmin(otherSide);
    testAdmin.Sync("setup server");
    {
        TextWebLinkTest::WebClient webConnector(Core::NodeId(TextWebLinkTest::g_connector, 12343));
        Core::ProxyType<Web::Request> webRequest(Core::ProxyType<Web::Request>::Create());
        Core::ProxyType<Web::TextBody> webRequestBody(Core::ProxyType<Web::TextBody>::Create());
        webRequest->Body<Web::TextBody>(webRequestBody);
        webConnector.Open(Core::infinite);
        while(!webConnector.IsOpen());
        webRequest->Verb = Web::Request::HTTP_GET;
        string sent = "Just a body to send";
        *webRequestBody = sent;
        webConnector.Submit(webRequest);

        webConnector.Wait();
        string received;
        webConnector.Retrieve(received);
        ASSERT_STREQ(received.c_str(), sent.c_str());
        testAdmin.Sync("client done");
    }
    Core::Singleton::Dispose();
}
