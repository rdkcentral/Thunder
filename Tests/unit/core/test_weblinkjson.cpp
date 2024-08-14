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

#include <core/core.h>
#include <websocket/websocket.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    enum class CommandTypeWeblinkJSON {
        EXECUTESHELL,
        WIFISETTINGS,
        FANCONTROL,
        PLAYERCONTROL
    };

    class Parameters : public ::Thunder::Core::JSON::Container {
    public:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

        Parameters()
            : ::Thunder::Core::JSON::Container()
            , Speed(0)
            , Duration(0)
            , Command()
            , Settings()
        {
            Add(_T("speed"), &Speed);
            Add(_T("duration"), &Duration);
            Add(_T("command"), &Command);
            Add(_T("settings"), &Settings);
        }

        ~Parameters()
        {
        }

    public:
        ::Thunder::Core::JSON::OctSInt16 Speed;
        ::Thunder::Core::JSON::DecUInt16 Duration;
        ::Thunder::Core::JSON::EnumType<CommandTypeWeblinkJSON> Command;
        ::Thunder::Core::JSON::ArrayType<::Thunder::Core::JSON::DecUInt16> Settings;
    };

    class Command : public ::Thunder::Core::JSON::Container {
    public:
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        Command()
            : ::Thunder::Core::JSON::Container()
            , Identifier(0)
            , Name()
            , BaseAddress(0)
            , TrickFlag(false)
            , Params()
        {
            Add(_T("id"), &Identifier);
            Add(_T("name"), &Name);
            Add(_T("baseAddress"), &BaseAddress);
            Add(_T("trickFlag"), &TrickFlag);
            Add(_T("parameters"), &Params);
        }

        ~Command()
        {
        }

    public:
        ::Thunder::Core::JSON::DecUInt32 Identifier;
        ::Thunder::Core::JSON::String Name;
        ::Thunder::Core::JSON::HexUInt32 BaseAddress;
        ::Thunder::Core::JSON::Boolean TrickFlag;
        Parameters Params;
    };

    typedef Web::JSONBodyType<Command> CommandBody;

    class JSONWebServer : public Web::WebLinkType<::Thunder::Core::SocketStream, Web::Request, Web::Response, ::Thunder::Core::ProxyPoolType<Web::Request>&> {
    private:
        typedef Web::WebLinkType<::Thunder::Core::SocketStream, Web::Request, Web::Response, ::Thunder::Core::ProxyPoolType<Web::Request>&> BaseClass;

        constexpr static uint32_t maxWaitTimeMs = 4000;

    public:
        JSONWebServer() = delete;
        JSONWebServer(const JSONWebServer& copy) = delete;
        JSONWebServer& operator=(const JSONWebServer&) = delete;

        JSONWebServer(const SOCKET& connector, const ::Thunder::Core::NodeId& remoteId, ::Thunder::Core::SocketServerType<JSONWebServer>*)
            : BaseClass(5, _requestFactory, false, connector, remoteId, 2048, 2048)
            , _requestFactory(5)
            , _commandBodyFactory(5)
        {
        }

        virtual ~JSONWebServer()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(::Thunder::Core::ProxyType<::Thunder::Web::Request>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_EQ(request->MajorVersion, 1);
            EXPECT_EQ(request->MinorVersion, 1);
            EXPECT_TRUE(request->HasBody());
            EXPECT_EQ(request->ContentLength.Value(), 60u);

            ::Thunder::Core::ProxyType<Web::Response> response(::Thunder::Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<CommandBody>(request->Body<CommandBody>());

            EXPECT_TRUE(Submit(response));
        }

        virtual void Send(const ::Thunder::Core::ProxyType<Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_TRUE(response->HasBody());
        }

        virtual void StateChange()
        {
        }

    private:
        ::Thunder::Core::ProxyPoolType<Web::Request> _requestFactory;
        ::Thunder::Core::ProxyPoolType<CommandBody> _commandBodyFactory;
    };

    class JSONWebClient : public Web::WebLinkType<::Thunder::Core::SocketStream, Web::Response, Web::Request, ::Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<::Thunder::Core::SocketStream, Web::Response, Web::Request, ::Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

        static constexpr uint32_t maxWaitTimeMs = 4000;

    public:
        JSONWebClient() = delete;
        JSONWebClient(const JSONWebClient& copy) = delete;
        JSONWebClient& operator=(const JSONWebClient&) = delete;

        JSONWebClient(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
            , _responseFactory(5)
            , _commandBodyFactory(5)
        {
        }

        virtual ~JSONWebClient()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(::Thunder::Core::ProxyType<::Thunder::Web::Response>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_STREQ(response->Message.c_str(), "OK");
            EXPECT_EQ(response->MajorVersion, 1);
            EXPECT_EQ(response->MinorVersion, 1);
            EXPECT_TRUE(response->HasBody());
            EXPECT_EQ(response->ContentLength.Value(), 60u);

            EXPECT_TRUE(response->Body<CommandBody>()->ToString(_dataReceived));

            EXPECT_EQ(_dataPending.Unlock(), ::Thunder::Core::ERROR_NONE);
        }

        virtual void Send(const ::Thunder::Core::ProxyType<Web::Request>& request)
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
        ::Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;
        ::Thunder::Core::ProxyPoolType<CommandBody> _commandBodyFactory;
    };

    TEST(WebLink, Json)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string connector {"0.0.0.0"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONWebServer> webServer(::Thunder::Core::NodeId(connector.c_str(), 12341));

            ASSERT_EQ(webServer.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(webServer.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONWebClient jsonWebConnector(::Thunder::Core::NodeId(connector.c_str(), 12341));

            ::Thunder::Core::ProxyType<Web::Request> jsonRequest(::Thunder::Core::ProxyType<Web::Request>::Create());
            ::Thunder::Core::ProxyType<CommandBody> jsonRequestBody(::Thunder::Core::ProxyType<CommandBody>::Create());
        
            // ProxyType
            ASSERT_TRUE(jsonRequest.IsValid());
            // ProxyType
            ASSERT_TRUE(jsonRequestBody.IsValid());

            jsonRequest->Body<CommandBody>(jsonRequestBody);

            ASSERT_EQ(jsonWebConnector.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_TRUE(jsonWebConnector.IsOpen());
        
            jsonRequest->Verb = Web::Request::HTTP_GET;
            jsonRequestBody->Identifier = 123;
            jsonRequestBody->Name = _T("TestCase");
            jsonRequestBody->Params.Speed = 4321;

            // True object
            ASSERT_TRUE(jsonRequest->IsValid());
 
            string sent;
            EXPECT_TRUE(jsonRequestBody->ToString(sent));

            EXPECT_TRUE(jsonWebConnector.Submit(jsonRequest));

            EXPECT_EQ(jsonWebConnector.Wait(), ::Thunder::Core::ERROR_NONE);

            string received;

            jsonWebConnector.Retrieve(received);

            EXPECT_STREQ(received.c_str(), sent.c_str());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests

ENUM_CONVERSION_BEGIN(Tests::Core::CommandTypeWeblinkJSON)
    { Tests::Core::CommandTypeWeblinkJSON::EXECUTESHELL, _TXT("ExecuteShell") },
    { Tests::Core::CommandTypeWeblinkJSON::WIFISETTINGS, _TXT("WiFiSettings") },
    { Tests::Core::CommandTypeWeblinkJSON::FANCONTROL, _TXT("FanControl") },
    { Tests::Core::CommandTypeWeblinkJSON::PLAYERCONTROL, _TXT("PlayerControl") },
ENUM_CONVERSION_END(Tests::Core::CommandTypeWeblinkJSON)

} // Thunder
