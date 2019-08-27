#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

using namespace WPEFramework;

namespace JsonWebLinkTest {
    namespace DataContainer {
        typedef enum {
            ExecuteShell,
            WiFiSettings,
            FanControl,
            PlayerControl

        } CommandType;

        class Parameters : public Core::JSON::Container {
        private:
            Parameters(const Parameters&);
            Parameters& operator=(const Parameters&);

        public:
            Parameters()
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
        private:
            Command(const Command&);
            Command& operator=(const Command&);

        public:
            Command()
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
    }

    typedef Web::JSONBodyType<DataContainer::Command> CommandBody;

    const TCHAR* g_connector = "0.0.0.0";
    static Core::ProxyPoolType<Web::Response> g_responseFactory(5);
    static Core::ProxyPoolType<Web::Request> g_requestFactory(5);
    static Core::ProxyPoolType<CommandBody> g_commandBodyFactory(5);

    class JSONWebServer : public Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, WPEFramework::Core::ProxyPoolType<Web::Request>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Request, Web::Response, WPEFramework::Core::ProxyPoolType<Web::Request>&> BaseClass;

        JSONWebServer();
        JSONWebServer(const JSONWebServer& copy);
        JSONWebServer& operator=(const JSONWebServer&);

    public:
        JSONWebServer(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONWebServer>*)
            : BaseClass(5, g_requestFactory, false, connector, remoteId, 2048, 2048)
        {
        }
        virtual ~JSONWebServer()
        {
            Close(WPEFramework::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<WPEFramework::Web::Request>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(g_commandBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Web::Request>& request)
        {
            ASSERT(request->Verb == Web::Request::HTTP_GET);
            ASSERT(request->MajorVersion == 1);
            ASSERT(request->MinorVersion == 1);
            ASSERT(request->HasBody());
            ASSERT(request->ContentLength.Value() == 60);

            Core::ProxyType<Web::Response> response(Core::ProxyType<Web::Response>::Create());
            response->ErrorCode = 200;
            response->Body<CommandBody>(request->Body<CommandBody>());
            Submit(response);
        }
        virtual void Send(const Core::ProxyType<Web::Response>& response)
        {
            ASSERT(response->ErrorCode == 200);
            ASSERT(response->HasBody());
        }
        virtual void StateChange()
        {
        }
    };

    class JSONWebClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, WPEFramework::Core::ProxyPoolType<Web::Response>&> BaseClass;

        JSONWebClient();
        JSONWebClient(const JSONWebClient& copy);
        JSONWebClient& operator=(const JSONWebClient&);

    public:
        JSONWebClient(const WPEFramework::Core::NodeId& remoteNode)
            : BaseClass(5, g_responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 208)
            , _dataPending(false, false)
        {
        }
        virtual ~JSONWebClient()
        {
            Close(WPEFramework::Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<WPEFramework::Web::Response>& element)
        {
            // Time to attach a Command Body
            element->Body<CommandBody>(g_commandBodyFactory.Element());
        }
        virtual void Received(Core::ProxyType<Web::Response>& response)
        {
            ASSERT(response->ErrorCode == 200);
            ASSERT(response->Message == string("OK"));
            ASSERT(response->MajorVersion == 1);
            ASSERT(response->MinorVersion == 1);
            ASSERT(response->HasBody());
            ASSERT(response->ContentLength.Value() == 60);

            response->Body<CommandBody>()->ToString(_dataReceived);
            _dataPending.Unlock();
        }
        virtual void Send(const Core::ProxyType<Web::Request>& request)
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

namespace WPEFramework {
ENUM_CONVERSION_BEGIN(JsonWebLinkTest::DataContainer::CommandType)

    { JsonWebLinkTest::DataContainer::ExecuteShell, _TXT("ExecuteShell") },
    { JsonWebLinkTest::DataContainer::WiFiSettings, _TXT("WiFiSettings") },
    { JsonWebLinkTest::DataContainer::FanControl, _TXT("FanControl") },
    { JsonWebLinkTest::DataContainer::PlayerControl, _TXT("PlayerControl") },

ENUM_CONVERSION_END(JsonWebLinkTest::DataContainer::CommandType)
}

TEST(WebLink, Json)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::SocketServerType<JsonWebLinkTest::JSONWebServer> _webServer(Core::NodeId(JsonWebLinkTest::g_connector, 12341));
        _webServer.Open(Core::infinite);
        testAdmin.Sync("setup server");
        testAdmin.Sync("client done");
    };

    IPTestAdministrator testAdmin(otherSide);
    testAdmin.Sync("setup server");
    {
        JsonWebLinkTest::JSONWebClient jsonWebConnector(Core::NodeId(JsonWebLinkTest::g_connector, 12341));
        Core::ProxyType<Web::Request> jsonRequest(Core::ProxyType<Web::Request>::Create());
        Core::ProxyType<JsonWebLinkTest::CommandBody> jsonRequestBody(Core::ProxyType<JsonWebLinkTest::CommandBody>::Create());
        jsonRequest->Body<JsonWebLinkTest::CommandBody>(jsonRequestBody);
        jsonWebConnector.Open(Core::infinite);
        while(!jsonWebConnector.IsOpen());
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
        ASSERT_STREQ(received.c_str(), sent.c_str());
        testAdmin.Sync("client done");
    }
    Core::Singleton::Dispose();
}
