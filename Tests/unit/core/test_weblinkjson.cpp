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

namespace {
    enum class CommandType {
        EXECUTESHELL,
        WIFISETTINGS,
        FANCONTROL,
        PLAYERCONTROL
    };

    class Parameters : public Core::JSON::Container {
    public:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

        Parameters()
            : Core::JSON::Container()
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
        Core::JSON::OctSInt16 Speed;
        Core::JSON::DecUInt16 Duration;
        Core::JSON::EnumType<CommandType> Command;
        Core::JSON::ArrayType<Core::JSON::DecUInt16> Settings;
    };

    class Command : public Core::JSON::Container {
    public:
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        Command()
            : Core::JSON::Container()
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
        Core::JSON::DecUInt32 Identifier;
        Core::JSON::String Name;
        Core::JSON::HexUInt32 BaseAddress;
        Core::JSON::Boolean TrickFlag;
        Parameters Params;
    };

    typedef Web::JSONBodyType<Command> CommandBody;

    }

    class JSONWebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, Thunder::Core::ProxyPoolType<Web::Request>&> BaseClass;

    public:
        JSONWebServer() = delete;
        JSONWebServer(const JSONWebServer& copy) = delete;
        JSONWebServer& operator=(const JSONWebServer&) = delete;

        JSONWebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONWebServer>*)
            : BaseClass(5, _requestFactory, false, connector, remoteId, 2048, 2048)
            , _requestFactory(5)
            , _commandBodyFactory(5)
        {
        }

        virtual ~JSONWebServer()
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Request>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }

        virtual void Received(Core::ProxyType<Web::Request>& request)
        {
            EXPECT_EQ(request->Verb, Web::Request::HTTP_GET);
            EXPECT_EQ(request->MajorVersion, 1);
            EXPECT_EQ(request->MinorVersion, 1);
            EXPECT_TRUE(request->HasBody());
            EXPECT_EQ(request->ContentLength.Value(), 60u);

            Core::ProxyType<Web::Response> response(Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<CommandBody>(request->Body<CommandBody>());
            Submit(response);
        }

        virtual void Send(const Core::ProxyType<Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_TRUE(response->HasBody());
        }

        virtual void StateChange()
        {
        }

    private:
        Core::ProxyPoolType<Web::Request> _requestFactory;
        Core::ProxyPoolType<CommandBody> _commandBodyFactory;
    };

    class JSONWebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

    public:
        JSONWebClient() = delete;
        JSONWebClient(const JSONWebClient& copy) = delete;
        JSONWebClient& operator=(const JSONWebClient&) = delete;

        JSONWebClient(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
            , _responseFactory(5)
            , _commandBodyFactory(5)
        {
        }

        virtual ~JSONWebClient()
        {
            Close(Thunder::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Thunder::Web::Response>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(_commandBodyFactory.Element());
        }

        virtual void Received(Core::ProxyType<Web::Response>& response)
        {
            EXPECT_EQ(response->ErrorCode, 200);
            EXPECT_STREQ(response->Message.c_str(), "OK");
            EXPECT_EQ(response->MajorVersion, 1);
            EXPECT_EQ(response->MinorVersion, 1);
            EXPECT_TRUE(response->HasBody());
            EXPECT_EQ(response->ContentLength.Value(), 60u);

            response->Body<CommandBody>()->ToString(_dataReceived);
            _dataPending.Unlock();
        }

        virtual void Send(const Core::ProxyType<Web::Request>& request)
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
        Core::ProxyPoolType<Web::Response> _responseFactory;
        Core::ProxyPoolType<CommandBody> _commandBodyFactory;
    };

    TEST(WebLink, Json)
    {
        std::string connector {"0.0.0.0"};
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<JSONWebServer> webServer(Core::NodeId(connector.c_str(), 12341));
            webServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            JSONWebClient jsonWebConnector(Core::NodeId(connector.c_str(), 12341));
            Core::ProxyType<Web::Request> jsonRequest(Core::ProxyType<Web::Request>::Create());
            Core::ProxyType<CommandBody> jsonRequestBody(Core::ProxyType<CommandBody>::Create());
            jsonRequest->Body<CommandBody>(jsonRequestBody);
            jsonWebConnector.Open(Core::infinite);
            while (!jsonWebConnector.IsOpen());
            jsonRequest->Verb = Web::Request::HTTP_GET;
            jsonRequestBody->Identifier = 123;
            jsonRequestBody->Name = _T("TestCase");
            jsonRequestBody->Params.Speed = 4321;
            string sent;
            jsonRequestBody->ToString(sent);
            jsonWebConnector.Submit(jsonRequest);

            jsonWebConnector.Wait();
            string received;
            jsonWebConnector.Retrieve(received);
            EXPECT_STREQ(received.c_str(), sent.c_str());
            testAdmin.Sync("client done");
       }
       Core::Singleton::Dispose();
    }
} // Tests

ENUM_CONVERSION_BEGIN(Tests::CommandType)
    { Tests::CommandType::EXECUTESHELL, _TXT("ExecuteShell") },
    { Tests::CommandType::WIFISETTINGS, _TXT("WiFiSettings") },
    { Tests::CommandType::FANCONTROL, _TXT("FanControl") },
    { Tests::CommandType::PLAYERCONTROL, _TXT("PlayerControl") },
ENUM_CONVERSION_END(Tests::CommandType)

} // Thunder
