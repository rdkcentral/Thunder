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

    class TextConnector : public ::Thunder::Core::StreamTextType<::Thunder::Core::SocketStream, ::Thunder::Core::TerminatorCarriageReturn> {
    private:
        typedef ::Thunder::Core::StreamTextType<::Thunder::Core::SocketStream, ::Thunder::Core::TerminatorCarriageReturn> BaseClass;

    public:
        TextConnector() = delete;
        TextConnector(const TextConnector& copy) = delete;
        TextConnector& operator=(const TextConnector&) = delete;

        TextConnector(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
        {
        }

        TextConnector(const SOCKET& connector, const ::Thunder::Core::NodeId& remoteId, ::Thunder::Core::SocketServerType<TextConnector>*)
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

        uint32_t Wait() const
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
        mutable ::Thunder::Core::Event _dataPending;
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
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string connector {"/tmp/wpestreamtext0"};

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<TextConnector> textSocketServer(::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(textSocketServer.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            std::unique_lock<std::mutex> lk(TextConnector::_mutex);

            while (!TextConnector::GetState()) {
                TextConnector::_cv.wait(lk);
            }

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(textSocketServer.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            TextConnector textSocketClient(::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(textSocketClient.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        
            const string message = "hello";
            textSocketClient.Submit(message);
            EXPECT_EQ(textSocketClient.Wait(), ::Thunder::Core::ERROR_NONE);

            string received;
            textSocketClient.Retrieve(received);

            EXPECT_STREQ(message.c_str(), received.c_str());

            ASSERT_EQ(textSocketClient.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
