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

namespace WPEFramework {
namespace Tests {

    class Message : public Core::JSON::Container {
    public:
        Message(const Message&) = delete;
	    Message& operator= (const Message&) = delete;

	    Message()
            : Core::JSON::Container()
            , EventType()
            , Event()
        {
            Add(_T("eventType"), &EventType);
            Add(_T("event"), &Event);
        }

        ~Message()
        {
        }

    public:
	    Core::JSON::String EventType;
	    Core::JSON::String Event;
    };

    class Factory : public Core::ProxyPoolType<Message> {
    public:
	    Factory() = delete;
	    Factory(const Factory&) = delete;
	    Factory& operator= (const Factory&) = delete;

	    Factory(const uint32_t number) : Core::ProxyPoolType<Message>(number)
        {
	    }

	    virtual ~Factory()
        {
	    }

    public:
	    Core::ProxyType<Core::JSON::IElement> Element(const string&)
        {
		    return (Core::ProxyType<Core::JSON::IElement>(Core::ProxyPoolType<Message>::Element()));
	    }
    };

    template<typename INTERFACE>
    class JsonSocketServer : public Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> {
    private:
	    typedef Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> BaseClass;

    public:
        JsonSocketServer() = delete;
        JsonSocketServer(const JsonSocketServer&) = delete;
	    JsonSocketServer& operator=(const JsonSocketServer&) = delete;

        JsonSocketServer(const SOCKET& socket, const Core::NodeId& remoteNode, Core::SocketServerType<JsonSocketServer<INTERFACE>>*)
            : BaseClass(2, _objectFactory, false, false, false, socket, remoteNode, 512, 512)
		    , _objectFactory(1)
        {
	    }

        virtual ~JsonSocketServer()
        {
        }

    public:
	    virtual bool IsIdle() const
        {
            return (true);
        }

	    virtual void StateChange()
        {
		    if (this->IsOpen()) {
                std::unique_lock<std::mutex> lk(_mutex);
                _done = true;
                _cv.notify_one();
            }
        }

        bool IsAttached() const
        {
            return (this->IsOpen());
        }

        virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject)
        {
            this->Submit(jsonObject);
        }

        virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject)
        {
	    }

        static bool GetState()
        {
            return _done;
        }

    private:
	    Factory _objectFactory;
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    template<typename INTERFACE>
    std::mutex JsonSocketServer<INTERFACE>::_mutex;
    template<typename INTERFACE>
    std::condition_variable JsonSocketServer<INTERFACE>::_cv;
    template<typename INTERFACE>
    bool JsonSocketServer<INTERFACE>::_done = false;

    template<typename INTERFACE>
    class JsonSocketClient : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Factory&, INTERFACE> {
    private:
		typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, Factory&, INTERFACE> BaseClass;

    public:
        JsonSocketClient() = delete;
        JsonSocketClient(const JsonSocketClient&) = delete;
        JsonSocketClient& operator=(const JsonSocketClient&) = delete;

        JsonSocketClient(const Core::NodeId& remoteNode)
		    : BaseClass(5, _objectFactory, _T(""), _T(""), _T(""), _T(""), false, true, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
            , _objectFactory(2)
            , _dataPending(false, false)
        {
        }

        virtual ~JsonSocketClient()
        {
        }

    public:
	    virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject)
	    {
		    string textElement;
		    jsonObject->ToString(textElement);
            _dataReceived = textElement;
            _dataPending.Unlock();
        }

        virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject)
	    {
        }

        virtual void StateChange()
	    {
        }

        virtual bool IsIdle() const
	    {
		    return (true);
        }

        Core::ProxyType<Core::JSON::IElement> Element()
        {
            return _objectFactory.Element("");
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
        Factory _objectFactory;
        string _dataReceived;
        mutable Core::Event _dataPending;
    };

    TEST(WebSocket, DISABLED_Json)
    {
        std::string connector {"/tmp/wpewebsocketjson0"};
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<JsonSocketServer<Core::JSON::IElement>> jsonWebSocketServer(Core::NodeId(connector.c_str()));
            jsonWebSocketServer.Open(Core::infinite);
            testAdmin.Sync("setup server");

            std::unique_lock<std::mutex> lk(JsonSocketServer<Core::JSON::IElement>::_mutex);
            while (!JsonSocketServer<Core::JSON::IElement>::GetState()) {
                JsonSocketServer<Core::JSON::IElement>::_cv.wait(lk);
            }
            testAdmin.Sync("server open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            Core::ProxyType<Message> sendObject = Core::ProxyType<Message>::Create();
            sendObject->EventType = _T("Test");
            sendObject->Event = _T("TestSend");
            std::string sendString;
            sendObject->ToString(sendString);

            JsonSocketClient<Core::JSON::IElement> jsonWebSocketClient(Core::NodeId(connector.c_str()));
            jsonWebSocketClient.Open(Core::infinite);
            testAdmin.Sync("server open");
            jsonWebSocketClient.Submit(Core::ProxyType<Core::JSON::IElement>(sendObject));
            jsonWebSocketClient.Wait();
            string received;
            jsonWebSocketClient.Retrieve(received);
            EXPECT_STREQ(sendString.c_str(), received.c_str());
            jsonWebSocketClient.Close(Core::infinite);
            testAdmin.Sync("client done");
        }
        Core::Singleton::Dispose();
    }

} // Tests
} // WPEFramework
