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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <websocket/websocket.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    class WebServer : public Web::WebLinkType<::Thunder::Core::SocketStream, Web::Request, Web::Response, ::Thunder::Core::ProxyPoolType<Web::Request> > {
    private:
        typedef Web::WebLinkType<::Thunder::Core::SocketStream, Web::Request, Web::Response, ::Thunder::Core::ProxyPoolType<Web::Request> > BaseClass;

        constexpr static uint32_t maxWaitTimeMs = 4000;

    public:
        WebServer() = delete;
        WebServer(const WebServer& copy) = delete;
        WebServer& operator=(const WebServer&) = delete;

        WebServer(const SOCKET& connector, const ::Thunder::Core::NodeId& remoteId, ::Thunder::Core::SocketServerType<WebServer>*)
            : BaseClass(5, false, connector, remoteId, 2048, 2048)
        {
        }

        virtual ~WebServer()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(::Thunder::Core::ProxyType<::Thunder::Web::Request>& element)
        {
            // Time to attach a String Body
            element->Body(_textBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<::Thunder::Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_EQ(request->MajorVersion, 1);
            EXPECT_EQ(request->MinorVersion, 1);
            EXPECT_TRUE(request->HasBody());
            EXPECT_EQ(request->ContentLength.Value(), 19u);

            ::Thunder::Core::ProxyType<Web::Response> response(::Thunder::Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<Web::TextBody>(request->Body<Web::TextBody>());

            EXPECT_TRUE(Submit(response));
        }

        virtual void Send(const ::Thunder::Core::ProxyType<::Thunder::Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_TRUE(response->HasBody());
        }

        virtual void StateChange()
        {
        }

    private:
        static ::Thunder::Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };

    ::Thunder::Core::ProxyPoolType<Web::TextBody> WebServer::_textBodyFactory(5);

    class WebClient : public Web::WebLinkType<::Thunder::Core::SocketStream, Web::Response, Web::Request, ::Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<::Thunder::Core::SocketStream, Web::Response, Web::Request, ::Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

        static constexpr uint32_t maxWaitTimeMs = 4000;

    public:
        WebClient() = delete;
        WebClient(const WebClient& copy) = delete;
        WebClient& operator=(const WebClient&) = delete;

        WebClient(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(5,_responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
        {
        }

        virtual ~WebClient()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(::Thunder::Core::ProxyType<::Thunder::Web::Response>& element)
        {
            // Time to attach a String Body
            element->Body(_textBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<::Thunder::Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_STREQ(response->Message.c_str(), "OK");
            EXPECT_EQ(response->MajorVersion, 1);
            EXPECT_EQ(response->MinorVersion, 1);
            EXPECT_TRUE(response->HasBody());
            EXPECT_EQ(response->ContentLength.Value(), 19u);

            _dataReceived = *(response->Body<Web::TextBody>());

            EXPECT_EQ(_dataPending.Unlock(), ::Thunder::Core::ERROR_NONE);
        }

        virtual void Send(const ::Thunder::Core::ProxyType<::Thunder::Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_TRUE(request->HasBody());
        }

        virtual void StateChange()
        {
        }

        uint32_t Wait() const
        {
            return _dataPending.Lock();
        }

        void Retrieve(string& text)
        {
            text = _dataReceived;
            _dataReceived.clear();
        }

    private:
        mutable ::Thunder::Core::Event _dataPending;
        string _dataReceived;
        static ::Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;
        static ::Thunder::Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
    };

    ::Thunder::Core::ProxyPoolType<Web::Response> WebClient::_responseFactory(5);
    ::Thunder::Core::ProxyPoolType<Web::TextBody> WebClient::_textBodyFactory(5);

    TEST(WebLink, Text)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string connector {"127.0.0.1"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<WebServer> _webServer(::Thunder::Core::NodeId(connector.c_str(), 12343));

            ASSERT_EQ(_webServer.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(_webServer.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            WebClient webConnector(::Thunder::Core::NodeId(connector.c_str(), 12343));

            ::Thunder::Core::ProxyType<Web::Request> webRequest(::Thunder::Core::ProxyType<Web::Request>::Create());
            ::Thunder::Core::ProxyType<Web::TextBody> webRequestBody(::Thunder::Core::ProxyType<Web::TextBody>::Create());
        
            webRequest->Body<Web::TextBody>(webRequestBody);

            ASSERT_EQ(webConnector.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        
            ASSERT_TRUE(webConnector.IsOpen());

            webRequest->Verb = Web::Request::HTTP_GET;

            // True object
            ASSERT_TRUE(webRequest->IsValid());
 
            string sent = "Just a body to send";
        
            *webRequestBody = sent;

            EXPECT_TRUE(webConnector.Submit(webRequest));

            ASSERT_EQ(webConnector.Wait(), ::Thunder::Core::ERROR_NONE);

            string received;

            webConnector.Retrieve(received);

            EXPECT_STREQ(received.c_str(), sent.c_str());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
