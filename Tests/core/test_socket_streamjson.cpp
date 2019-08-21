#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <websocket/websocket.h>

using namespace WPEFramework;
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

namespace StreamJsonTest {
    const TCHAR* g_connector = "/tmp/wpestreamjson0";
    bool g_done = false;

    class JSONObjectFactory : public Core::ProxyPoolType<DataContainer::Command> {
    private:
        JSONObjectFactory() = delete;
        JSONObjectFactory(const JSONObjectFactory&) = delete;
        JSONObjectFactory& operator= (const JSONObjectFactory&) = delete;

    public:
        JSONObjectFactory(const uint32_t number) : Core::ProxyPoolType<DataContainer::Command>(number) {
        }
        virtual ~JSONObjectFactory() {
        }

    public:
        Core::ProxyType<Core::JSON::IElement> Element(const string&) {
            return (Core::proxy_cast<Core::JSON::IElement>(Core::ProxyPoolType<DataContainer::Command>::Element()));
        }
    };

    class JSONConnector : public Core::StreamJSONType<Core::SocketStream, JSONObjectFactory&> {
    private:
        typedef Core::StreamJSONType<Core::SocketStream, JSONObjectFactory&> BaseClass;

        JSONConnector();
        JSONConnector(const JSONConnector& copy);
        JSONConnector& operator=(const JSONConnector&);

    public:
        JSONConnector(const WPEFramework::Core::NodeId& remoteNode)
            : BaseClass(5, _objectFactory, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }
        JSONConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONConnector>*)
            : BaseClass(5, _objectFactory, false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }
        virtual ~JSONConnector()
        {
        }

    public:
        virtual void Received(Core::ProxyType<Core::JSON::IElement>& newElement)
        {
            string textElement;
            newElement->ToString(textElement);

            if (_serverSocket)
                Submit(newElement);
            else {
                _dataReceived = textElement;
                _dataPending.SetEvent();
            }
        }
        virtual void Send(Core::ProxyType<Core::JSON::IElement>& newElement)
        {
        }
        virtual void StateChange()
        {
            if (IsOpen()) {
                if (_serverSocket)
                    g_done = true;
            }
        }
        virtual bool IsIdle() const
        {
            return (true);
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
    private:
        bool _serverSocket;
        string _dataReceived;
        mutable WPEFramework::Core::Event _dataPending;
        JSONObjectFactory _objectFactory;
    };
}

namespace WPEFramework {
ENUM_CONVERSION_BEGIN(DataContainer::CommandType)

    { DataContainer::ExecuteShell, _TXT("ExecuteShell") },
    { DataContainer::WiFiSettings, _TXT("WiFiSettings") },
    { DataContainer::FanControl, _TXT("FanControl") },
    { DataContainer::PlayerControl, _TXT("PlayerControl") },

ENUM_CONVERSION_END(DataContainer::CommandType)
}

TEST(Core_Socket, StreamJSON)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        Core::SocketServerType<StreamJsonTest::JSONConnector> jsonSocketServer(Core::NodeId(StreamJsonTest::g_connector));
        jsonSocketServer.Open(Core::infinite);
        testAdmin.Sync("setup server");
        while(!StreamJsonTest::g_done);
        testAdmin.Sync("server open");
        testAdmin.Sync("client done");
    };

    IPTestAdministrator testAdmin(otherSide);
    testAdmin.Sync("setup server");
    {
        Core::ProxyType<DataContainer::Command> sendObject = Core::ProxyType<DataContainer::Command>::Create();
        sendObject->Identifier = 1;
        sendObject->Name = _T("TestCase");
        sendObject->Params.Duration = 100;
        std::string sendString;
        sendObject->ToString(sendString);

        StreamJsonTest::JSONConnector jsonSocketClient(Core::NodeId(StreamJsonTest::g_connector));
        jsonSocketClient.Open(Core::infinite);
        testAdmin.Sync("server open");
        jsonSocketClient.Submit(Core::proxy_cast<Core::JSON::IElement>(sendObject));
        jsonSocketClient.Wait(2000);
        string received;
        jsonSocketClient.Retrieve(received);
        ASSERT_STREQ(sendString.c_str(), received.c_str());
        jsonSocketClient.Close(Core::infinite);
        testAdmin.Sync("client done");
    }
    Core::Singleton::Dispose();
}
