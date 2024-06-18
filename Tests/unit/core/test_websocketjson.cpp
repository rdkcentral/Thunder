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
#include <websocket/websocket.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    class Message : public Thunder::Core::JSON::Container {
    public:
        Message(const Message&) = delete;
	    Message& operator= (const Message&) = delete;

	    Message()
            : Thunder::Core::JSON::Container()
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
	    Thunder::Core::JSON::String EventType;
	    Thunder::Core::JSON::String Event;
    };

    class Factory : public Thunder::Core::ProxyPoolType<Message> {
    public:
	    Factory() = delete;
	    Factory(const Factory&) = delete;
	    Factory& operator= (const Factory&) = delete;

	    Factory(const uint32_t number) : Thunder::Core::ProxyPoolType<Message>(number)
        {
	    }

	    virtual ~Factory()
        {
	    }

    public:
	    Thunder::Core::ProxyType<Thunder::Core::JSON::IElement> Element(const string&)
        {
		    return (Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>(Thunder::Core::ProxyPoolType<Message>::Element()));
	    }
    };

    template<typename INTERFACE>
    class JsonSocketServer : public Thunder::Core::StreamJSONType< Web::WebSocketServerType<Thunder::Core::SocketStream>, Factory&, INTERFACE> {
    private:
	    typedef Thunder::Core::StreamJSONType< Web::WebSocketServerType<Thunder::Core::SocketStream>, Factory&, INTERFACE> BaseClass;

    public:
        JsonSocketServer() = delete;
        JsonSocketServer(const JsonSocketServer&) = delete;
	    JsonSocketServer& operator=(const JsonSocketServer&) = delete;

        JsonSocketServer(const SOCKET& socket, const Thunder::Core::NodeId& remoteNode, Thunder::Core::SocketServerType<JsonSocketServer<INTERFACE>>*)
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

        virtual void Received(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& jsonObject)
        {
            this->Submit(jsonObject);
        }

        virtual void Send(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& jsonObject)
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
    class JsonSocketClient : public Thunder::Core::StreamJSONType<Web::WebSocketClientType<Thunder::Core::SocketStream>, Factory&, INTERFACE> {
    private:
		typedef Thunder::Core::StreamJSONType<Web::WebSocketClientType<Thunder::Core::SocketStream>, Factory&, INTERFACE> BaseClass;

    public:
        JsonSocketClient() = delete;
        JsonSocketClient(const JsonSocketClient&) = delete;
        JsonSocketClient& operator=(const JsonSocketClient&) = delete;

        JsonSocketClient(const Thunder::Core::NodeId& remoteNode)
		    : BaseClass(5, _objectFactory, _T(""), _T(""), _T(""), _T(""), false, true, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
            , _objectFactory(2)
            , _dataPending(false, false)
        {
        }

        virtual ~JsonSocketClient()
        {
        }

    public:
	    virtual void Received(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& jsonObject)
	    {
		    string textElement;
		    jsonObject->ToString(textElement);
            _dataReceived = textElement;
            _dataPending.Unlock();
        }

        virtual void Send(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>& jsonObject)
	    {
        }

        virtual void StateChange()
	    {
        }

        virtual bool IsIdle() const
	    {
		    return (true);
        }

        Thunder::Core::ProxyType<Thunder::Core::JSON::IElement> Element()
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
        mutable Thunder::Core::Event _dataPending;
    };

    TEST(WebSocket, DISABLED_Json)
    {
        std::string connector {"/tmp/wpewebsocketjson0"};
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Thunder::Core::SocketServerType<JsonSocketServer<Thunder::Core::JSON::IElement>> jsonWebSocketServer(Thunder::Core::NodeId(connector.c_str()));
            jsonWebSocketServer.Open(Thunder::Core::infinite);
            testAdmin.Sync("setup server");

            std::unique_lock<std::mutex> lk(JsonSocketServer<Thunder::Core::JSON::IElement>::_mutex);
            while (!JsonSocketServer<Thunder::Core::JSON::IElement>::GetState()) {
                JsonSocketServer<Thunder::Core::JSON::IElement>::_cv.wait(lk);
            }
            testAdmin.Sync("server open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            Thunder::Core::ProxyType<Message> sendObject = Thunder::Core::ProxyType<Message>::Create();
            sendObject->EventType = _T("Test");
            sendObject->Event = _T("TestSend");
            std::string sendString;
            sendObject->ToString(sendString);

            JsonSocketClient<Thunder::Core::JSON::IElement> jsonWebSocketClient(Thunder::Core::NodeId(connector.c_str()));
            jsonWebSocketClient.Open(Thunder::Core::infinite);
            testAdmin.Sync("server open");
            jsonWebSocketClient.Submit(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>(sendObject));
            jsonWebSocketClient.Wait();
            string received;
            jsonWebSocketClient.Retrieve(received);
            EXPECT_STREQ(sendString.c_str(), received.c_str());
            jsonWebSocketClient.Close(Thunder::Core::infinite);
            testAdmin.Sync("client done");
        }
        Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
