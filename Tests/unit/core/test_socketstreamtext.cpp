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

    class TextConnector : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
    private:
        typedef Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> BaseClass;

    public:
        TextConnector() = delete;
        TextConnector(const TextConnector& copy) = delete;
        TextConnector& operator=(const TextConnector&) = delete;

        TextConnector(const Thunder::Core::NodeId& remoteNode)
            : BaseClass(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
        {
        }

        TextConnector(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<TextConnector>*)
            : BaseClass(false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
        {
        }

        virtual ~TextConnector()
        {
        }

    public:
        virtual void Received(string& text)
        {
            if (_serverSocket)
                Submit(text);
            else {
                _dataReceived = text;
                _dataPending.Unlock();
            }
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

        virtual void Send(const string& text)
        {
        }

        virtual void StateChange()
        {
            if (IsOpen()) {
                if (_serverSocket) {
                    std::unique_lock<std::mutex> lk(_mutex);
                    _done = true;
                    _cv.notify_one();
                }
            }
        }

        static bool GetState()
        {
            return _done;
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable Thunder::Core::Event _dataPending;
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    std::mutex TextConnector::_mutex;
    std::condition_variable TextConnector::_cv;
    bool TextConnector::_done = false;

    TEST(Core_Socket, StreamText)
    {
        std::string connector {"/tmp/wpestreamtext0"};

        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::SocketServerType<TextConnector> textSocketServer(Core::NodeId(connector.c_str()));
            textSocketServer.Open(Core::infinite);
            testAdmin.Sync("setup server");
            std::unique_lock<std::mutex> lk(TextConnector::_mutex);
            while (!TextConnector::GetState()) {
                TextConnector::_cv.wait(lk);
            }
            testAdmin.Sync("server open");
            testAdmin.Sync("client done");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        testAdmin.Sync("setup server");
        {
            TextConnector textSocketClient(Core::NodeId(connector.c_str()));
            textSocketClient.Open(Core::infinite);
            testAdmin.Sync("server open");
            string message = "hello";
            textSocketClient.Submit(message);
            textSocketClient.Wait();
            string received;
            textSocketClient.Retrieve(received);
            EXPECT_STREQ(message.c_str(), received.c_str());
            textSocketClient.Close(Core::infinite);
            testAdmin.Sync("client done");
        }
        Core::Singleton::Dispose();
    }
} // Tests
} // Thunder
