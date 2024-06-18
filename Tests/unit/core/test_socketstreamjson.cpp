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

#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    enum class CommandTypeSocketStreamJSON {
        EXECUTESHELL,
        WIFISETTINGS,
        FANCONTROL,
        PLAYERCONTROL
    };

    class Parameters : public Thunder::Core::JSON::Container {
    public:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

        Parameters()
            : Thunder::Core::JSON::Container()
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
        Thunder::Core::JSON::OctSInt16 Speed;
        Thunder::Core::JSON::DecUInt16 Duration;
        Thunder::Core::JSON::EnumType<CommandTypeSocketStreamJSON> Command;
        Thunder::Core::JSON::ArrayType<Thunder::Core::JSON::DecUInt16> Settings;
    };

    class Command : public Thunder::Core::JSON::Container {
    public:
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        Command()
            : Thunder::Core::JSON::Container()
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
        Thunder::Core::JSON::DecUInt32 Identifier;
        Thunder::Core::JSON::String Name;
        Thunder::Core::JSON::HexUInt32 BaseAddress;
        Thunder::Core::JSON::Boolean TrickFlag;
        Parameters Params;
    };

    class JSONObjectFactory : public Thunder::Core::ProxyPoolType<Command> {
    public:
        JSONObjectFactory() = delete;
        JSONObjectFactory(const JSONObjectFactory&) = delete;
        JSONObjectFactory& operator= (const JSONObjectFactory&) = delete;

        JSONObjectFactory(const uint32_t number) : Thunder::Core::ProxyPoolType<Command>(number)
        {
        }

        virtual ~JSONObjectFactory()
        {
        }

    public:
        Thunder::Core::ProxyType<Thunder::Core::JSON::IElement> Element(const string&)
        {
            return (Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>(Thunder::Core::ProxyPoolType<Command>::Element()));
        }
    };

    template<typename INTERFACE>
    class JSONConnector : public Thunder::Core::StreamJSONType<Thunder::Core::SocketStream, JSONObjectFactory&, INTERFACE> {
    private:
        typedef Thunder::Core::StreamJSONType<Thunder::Core::SocketStream, JSONObjectFactory&, INTERFACE> BaseClass;

    public:
        JSONConnector() = delete;
        JSONConnector(const JSONConnector& copy) = delete;
        JSONConnector& operator=(const JSONConnector&) = delete;

        JSONConnector(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _objectFactory, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }

        JSONConnector(const SOCKET& connector, const Thunder::Core::NodeId& remoteId, Thunder::Core::SocketServerType<JSONConnector<INTERFACE>>*)
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
        virtual void Received(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& newElement)
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

        virtual void Send(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& newElement)
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
        mutable Thunder::Core::Event _dataPending;
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
            Thunder::Core::SocketServerType<JSONConnector<Thunder::Core::JSON::IElement>> jsonSocketServer(Thunder::Core::NodeId(connector.c_str()));
            jsonSocketServer.Open(Thunder::Core::infinite);
            testAdmin.Sync("setup server");
            std::unique_lock<std::mutex> lk(JSONConnector<Thunder::Core::JSON::IElement>::_mutex);
            while (!JSONConnector<Thunder::Core::JSON::IElement>::GetState()) {
                JSONConnector<Thunder::Core::JSON::IElement>::_cv.wait(lk);
            }

            testAdmin.Sync("client open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            Thunder::Core::ProxyType<Command> sendObject = Thunder::Core::ProxyType<Command>::Create();
            sendObject->Identifier = 1;
            sendObject->Name = _T("TestCase");
            sendObject->Params.Duration = 100;
            std::string sendString;
            sendObject->ToString(sendString);

            JSONConnector<Thunder::Core::JSON::IElement> jsonSocketClient(Thunder::Core::NodeId(connector.c_str()));
            jsonSocketClient.Open(Thunder::Core::infinite);
            testAdmin.Sync("client open");
            jsonSocketClient.Submit(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>(sendObject));
            jsonSocketClient.Wait();
            string received;
            jsonSocketClient.Retrieve(received);
            EXPECT_STREQ(sendString.c_str(), received.c_str());
            jsonSocketClient.Close(Thunder::Core::infinite);
            testAdmin.Sync("client done");
       }
       Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests

ENUM_CONVERSION_BEGIN(Tests::Core::CommandTypeSocketStreamJSON)
    { Tests::Core::CommandTypeSocketStreamJSON::EXECUTESHELL, _TXT("ExecuteShell") },
    { Tests::Core::CommandTypeSocketStreamJSON::WIFISETTINGS, _TXT("WiFiSettings") },
    { Tests::Core::CommandTypeSocketStreamJSON::FANCONTROL, _TXT("FanControl") },
    { Tests::Core::CommandTypeSocketStreamJSON::PLAYERCONTROL, _TXT("PlayerControl") },
ENUM_CONVERSION_END(Tests::Core::CommandTypeSocketStreamJSON)

} // Thunder
