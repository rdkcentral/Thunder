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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

namespace Thunder {
namespace Tests {

    class WebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request> > {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request> > BaseClass;

    public:
        WebServer() = delete;
        WebServer(const WebServer& copy) = delete;
        WebServer& operator=(const WebServer&) = delete;

        WebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<WebServer>*)
            : BaseClass(5, false, connector, remoteId, 2048, 2048)
        {
        }

        virtual ~WebServer()
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Request>& element)
        {
            // Time to attach a String Body
            element->Body(_textBodyFactory.Element());
        }

        virtual void Received(Core::ProxyType<Thunder::Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_EQ(request->MajorVersion, 1);
            EXPECT_EQ(request->MinorVersion, 1);
            EXPECT_TRUE(request->HasBody());
            EXPECT_EQ(request->ContentLength.Value(), 19u);

            Core::ProxyType<Web::Response> response(Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<Web::TextBody>(request->Body<Web::TextBody>());
            Submit(response);
        }

        virtual void Send(const Core::ProxyType<Thunder::Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_TRUE(response->HasBody());
        }

        virtual void StateChange()
        {
        }

    private:
        static Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };

    Core::ProxyPoolType<Web::TextBody> WebServer::_textBodyFactory(5);

    class WebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

    public:
        WebClient() = delete;
        WebClient(const WebClient& copy) = delete;
        WebClient& operator=(const WebClient&) = delete;

        WebClient(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5,_responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
        {
        }

        virtual ~WebClient()
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Response>& element)
        {
            // Time to attach a String Body
            element->Body(_textBodyFactory.Element());
        }

        virtual void Received(Core::ProxyType<Thunder::Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_STREQ(response->Message.c_str(), "OK");
            EXPECT_EQ(response->MajorVersion, 1);
            EXPECT_EQ(response->MinorVersion, 1);
            EXPECT_TRUE(response->HasBody());
            EXPECT_EQ(response->ContentLength.Value(), 19u);

            _dataReceived = *(response->Body<Web::TextBody>());
            _dataPending.Unlock();
        }

        virtual void Send(const Core::ProxyType<Thunder::Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_TRUE(request->HasBody());
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
        mutable Thunder::Core::Event _dataPending;
        string _dataReceived;
        static Core::ProxyPoolType<Web::Response> _responseFactory;
        static Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };

    Core::ProxyPoolType<Web::Response> WebClient::_responseFactory(5);
    Core::ProxyPoolType<Web::TextBody> WebClient::_textBodyFactory(5);

    TEST(WebLink, Text)
    {
        std::string connector {"127.0.0.1"};
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<WebServer> _webServer(Core::NodeId(connector.c_str(), 12343));
            _webServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            WebClient webConnector(Core::NodeId(connector.c_str(), 12343));
            Core::ProxyType<Web::Request> webRequest(Core::ProxyType<Web::Request>::Create());
            Core::ProxyType<Web::TextBody> webRequestBody(Core::ProxyType<Web::TextBody>::Create());
            webRequest->Body<Web::TextBody>(webRequestBody);
            webConnector.Open(Core::infinite);
            while (!webConnector.IsOpen());
            webRequest->Verb = Web::Request::HTTP_GET;
            string sent = "Just a body to send";
            *webRequestBody = sent;
            webConnector.Submit(webRequest);

            webConnector.Wait();
            string received;
            webConnector.Retrieve(received);
            EXPECT_STREQ(received.c_str(), sent.c_str());
            testAdmin.Sync("client done");
        }
        Core::Singleton::Dispose();
    }

} // Tests
} // Thunder
