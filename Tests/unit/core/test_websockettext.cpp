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

    class TextSocketServer : public Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Web::WebSocketServerType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

    public:
        TextSocketServer() = delete;
	    TextSocketServer(const TextSocketServer&) = delete;
	    TextSocketServer& operator=(const TextSocketServer&) = delete;

        TextSocketServer(const SOCKET& socket, const WPEFramework::Core::NodeId& remoteNode, WPEFramework::Core::SocketServerType<TextSocketServer>*)
            : BaseClass(false, true, false, socket, remoteNode, 1024, 1024)
        {
        }

        virtual ~TextSocketServer()
        {
        }

    public:
	    virtual void StateChange()
        {
		    if (IsOpen()) {
                std::unique_lock<std::mutex> lk(_mutex);
                _done = true;
                _cv.notify_one();
            }
        }

        virtual void Received(string& text)
        {
            Submit(text);
        }

        virtual void Send(const string& text)
        {
        }

        static bool GetState()
        {
            return _done;
        }

    private:
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    bool TextSocketServer::_done = false;
    std::mutex TextSocketServer::_mutex;
    std::condition_variable TextSocketServer::_cv;

    class TextSocketClient : public Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> {
    private:
		typedef Core::StreamTextType<Web::WebSocketClientType<Core::SocketStream>, Core::TerminatorCarriageReturn> BaseClass;

    public:
        TextSocketClient() = delete;
	    TextSocketClient(const TextSocketClient&) = delete;
        TextSocketClient& operator=(const TextSocketClient&) = delete;

        TextSocketClient(const Core::NodeId& remoteNode)
            : BaseClass(_T("/"), _T("echo"), "", "", false, true, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _dataPending(false, false)
        {
        }

	    virtual ~TextSocketClient()
        {
	    }

    public:
	    virtual void Received(string& text)
	    {
            _dataReceived = text;
            _dataPending.Unlock();
	    }

        virtual void Send(const string& text)
        {
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
        string _dataReceived;
        mutable WPEFramework::Core::Event _dataPending;
    };

    TEST(WebSocket, DISABLED_Text)
    {
        std::string connector {"/tmp/wpewebsockettext0"};
        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<TextSocketServer> textWebSocketServer(Core::NodeId(connector.c_str()));
            textWebSocketServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            std::unique_lock<std::mutex> lk(TextSocketServer::_mutex);
            while (!TextSocketServer::GetState()) {
                TextSocketServer::_cv.wait(lk);
            }

            testAdmin.Sync("server open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            TextSocketClient textWebSocketClient(Core::NodeId(connector.c_str()));
            textWebSocketClient.Open(Core::infinite);
            testAdmin.Sync("server open");
            string sentString = "Test String";
            textWebSocketClient.Submit(sentString);
            textWebSocketClient.Wait();
            string received;
            textWebSocketClient.Retrieve(received);
            EXPECT_STREQ(sentString.c_str(), received.c_str());
            textWebSocketClient.Close(Core::infinite);
            testAdmin.Sync("client done");
        }
        Core::Singleton::Dispose();
    }

} // Tests
} // WPEFramework
