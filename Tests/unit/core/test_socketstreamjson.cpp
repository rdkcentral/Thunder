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
#include <condition_variable>
#include <mutex>

namespace Thunder {
namespace Tests {

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

    class JSONObjectFactory : public Core::ProxyPoolType<Command> {
    public:
        JSONObjectFactory() = delete;
        JSONObjectFactory(const JSONObjectFactory&) = delete;
        JSONObjectFactory& operator= (const JSONObjectFactory&) = delete;

        JSONObjectFactory(const uint32_t number) : Core::ProxyPoolType<Command>(number)
        {
        }

        virtual ~JSONObjectFactory()
        {
        }

    public:
        Core::ProxyType<Core::JSON::IElement> Element(const string&)
        {
            return (Core::ProxyType<Core::JSON::IElement>(Core::ProxyPoolType<Command>::Element()));
        }
    };

    template<typename INTERFACE>
    class JSONConnector : public Core::StreamJSONType<Core::SocketStream, JSONObjectFactory&, INTERFACE> {
    private:
        typedef Core::StreamJSONType<Core::SocketStream, JSONObjectFactory&, INTERFACE> BaseClass;

    public:
        JSONConnector() = delete;
        JSONConnector(const JSONConnector& copy) = delete;
        JSONConnector& operator=(const JSONConnector&) = delete;

        JSONConnector(const Core::NodeId& remoteNode)
            : BaseClass(5, _objectFactory, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }

        JSONConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<JSONConnector<INTERFACE>>*)
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
                this->Submit(newElement);
            else {
                _dataReceived = textElement;
                _dataPending.Unlock();
            }
        }

        virtual void Send(Core::ProxyType<Core::JSON::IElement>& newElement)
        {
        }

        virtual void StateChange()
        {
            if (this->IsOpen()) {
                if (_serverSocket) {
                    std::unique_lock<std::mutex> lk(_mutex);
                    _done = true;
                    _cv.notify_one();
                 }
            }
        }

        virtual bool IsIdle() const
        {
            return (true);
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

        static bool GetState()
        {
            return _done;
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable Core::Event _dataPending;
        JSONObjectFactory _objectFactory;
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    template<typename INTERFACE>
    std::mutex JSONConnector<INTERFACE>::_mutex;
    template<typename INTERFACE>
    std::condition_variable JSONConnector<INTERFACE>::_cv;
    template<typename INTERFACE>
    bool JSONConnector<INTERFACE>::_done = false;

    TEST(Core_Socket, StreamJSON)
    {
        std::string connector = "/tmp/wpestreamjson0";
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<JSONConnector<Core::JSON::IElement>> jsonSocketServer(Core::NodeId(connector.c_str()));
            jsonSocketServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            std::unique_lock<std::mutex> lk(JSONConnector<Core::JSON::IElement>::_mutex);
            while (!JSONConnector<Core::JSON::IElement>::GetState()) {
                JSONConnector<Core::JSON::IElement>::_cv.wait(lk);
            }

            testAdmin.Sync("client open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            Core::ProxyType<Command> sendObject = Core::ProxyType<Command>::Create();
            sendObject->Identifier = 1;
            sendObject->Name = _T("TestCase");
            sendObject->Params.Duration = 100;
            std::string sendString;
            sendObject->ToString(sendString);

            JSONConnector<Core::JSON::IElement> jsonSocketClient(Core::NodeId(connector.c_str()));
            jsonSocketClient.Open(Core::infinite);
            testAdmin.Sync("client open");
            jsonSocketClient.Submit(Core::ProxyType<Core::JSON::IElement>(sendObject));
            jsonSocketClient.Wait();
            string received;
            jsonSocketClient.Retrieve(received);
            EXPECT_STREQ(sendString.c_str(), received.c_str());
            jsonSocketClient.Close(Core::infinite);
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
