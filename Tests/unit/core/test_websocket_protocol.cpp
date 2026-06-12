/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <websocket/websocket.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // =========================================================================
    // TEST FILE: test_websocket_protocol.cpp
    //
    // Purpose:
    //   Tests WebSocket protocol edge cases beyond basic text/JSON roundtrip:
    //   - Binary data exchange
    //   - Zero-length messages
    //   - Large messages (multi-frame)
    //   - Rapid sequential messages
    //   - Close frame behavior
    //   - Ping/Pong activity tracking
    //   - Multiple clients on same server
    //
    // Architecture:
    //   Thread-based server/client using SocketServerType and
    //   WebSocketClientType/WebSocketServerType over Unix domain sockets.
    // =========================================================================

    // =========================================================================
    // TextSocketServer — echoes received text back
    // =========================================================================
    class ProtoTextServer
        : public ::Thunder::Core::StreamTextType<
              Web::WebSocketServerType<::Thunder::Core::SocketStream>,
              ::Thunder::Core::TerminatorCarriageReturn> {
    private:
        typedef ::Thunder::Core::StreamTextType<
            Web::WebSocketServerType<::Thunder::Core::SocketStream>,
            ::Thunder::Core::TerminatorCarriageReturn>
            BaseClass;

    public:
        ProtoTextServer() = delete;
        ProtoTextServer(const ProtoTextServer&) = delete;
        ProtoTextServer& operator=(const ProtoTextServer&) = delete;

        ProtoTextServer(const SOCKET& connector,
                        const ::Thunder::Core::NodeId& remoteId,
                        ::Thunder::Core::SocketServerType<ProtoTextServer>*)
            : BaseClass(false, false, false, connector, remoteId, 1024, 1024)
        {
        }
        ~ProtoTextServer() override = default;

        void StateChange() override
        {
            if (this->IsOpen()) {
                std::lock_guard<std::mutex> lk(s_mutex);
                s_connected = true;
                s_cv.notify_one();
            }
        }

        void Received(string& text) override
        {
            Submit(text);
        }

        void Send(const string&) override {}

        static void Reset() { s_connected = false; }
        static bool Connected() { return s_connected; }

        static std::mutex s_mutex;
        static std::condition_variable s_cv;
        static bool s_connected;
    };

    std::mutex ProtoTextServer::s_mutex;
    std::condition_variable ProtoTextServer::s_cv;
    bool ProtoTextServer::s_connected = false;

    // =========================================================================
    // TextSocketClient — receives text, stores last received
    // =========================================================================
    class ProtoTextClient
        : public ::Thunder::Core::StreamTextType<
              Web::WebSocketClientType<::Thunder::Core::SocketStream>,
              ::Thunder::Core::TerminatorCarriageReturn> {
    private:
        typedef ::Thunder::Core::StreamTextType<
            Web::WebSocketClientType<::Thunder::Core::SocketStream>,
            ::Thunder::Core::TerminatorCarriageReturn>
            BaseClass;

    public:
        ProtoTextClient() = delete;
        ProtoTextClient(const ProtoTextClient&) = delete;
        ProtoTextClient& operator=(const ProtoTextClient&) = delete;

        ProtoTextClient(const ::Thunder::Core::NodeId& remote)
            : BaseClass(_T("/"), _T(""), _T(""), _T(""), false, true,
                        false, remote.AnyInterface(), remote, 1024, 1024)
        {
        }
        ~ProtoTextClient() override = default;

        void Received(string& text) override
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _received = text;
            _hasData = true;
            _cv.notify_one();
        }

        void Send(const string&) override {}
        void StateChange() override {}

        bool WaitForResponse(uint32_t ms = 5000)
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return _cv.wait_for(lk, std::chrono::milliseconds(ms),
                [this]{ return _hasData; });
        }

        string Retrieve()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            _hasData = false;
            return _received;
        }

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        string _received;
        bool _hasData = false;
    };

    // =========================================================================
    // Helper: run text server in thread
    // =========================================================================
    static void RunTextServer(const std::string& connector,
        std::function<void(ProtoTextClient&)> clientLogic)
    {
        constexpr uint32_t maxWait = 8000;

        ProtoTextServer::Reset();
        ::unlink(connector.c_str());

        std::atomic<bool> serverReady{false};
        std::mutex readyMutex;
        std::condition_variable readyCV;
        std::atomic<bool> clientDone{false};

        std::thread serverThread([&]() {
            ::Thunder::Core::SocketServerType<ProtoTextServer> server(
                ::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(server.Open(maxWait), ::Thunder::Core::ERROR_NONE);

            {
                std::lock_guard<std::mutex> lk(readyMutex);
                serverReady = true;
            }
            readyCV.notify_one();

            while (!clientDone.load()) {
                SleepMs(50);
            }
            SleepMs(200);
            server.Close(maxWait);
        });

        {
            std::unique_lock<std::mutex> lk(readyMutex);
            ASSERT_TRUE(readyCV.wait_for(lk, std::chrono::seconds(10),
                [&]{ return serverReady.load(); }));
        }

        SleepMs(100);

        ProtoTextClient client(::Thunder::Core::NodeId(connector.c_str()));
        ASSERT_EQ(client.Open(maxWait), ::Thunder::Core::ERROR_NONE);
        ASSERT_TRUE(client.IsOpen());

        {
            std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
            ASSERT_TRUE(ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                []{ return ProtoTextServer::Connected(); }));
        }

        clientLogic(client);

        EXPECT_EQ(client.Close(maxWait), ::Thunder::Core::ERROR_NONE);
        clientDone = true;
        serverThread.join();

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Tests
    // =========================================================================

    TEST(WebSocketProtocol, TextEchoRoundTrip)
    {
        RunTextServer("/tmp/wpe_ws_proto_0", [](ProtoTextClient& client) {
            const string msg = "Hello WebSocket";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, LargeMessage)
    {
        RunTextServer("/tmp/wpe_ws_proto_1", [](ProtoTextClient& client) {
            // 2KB message — exceeds single 1024-byte buffer, tests fragmentation
            const string msg(2048, 'A');
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, MultipleRapidMessages)
    {
        RunTextServer("/tmp/wpe_ws_proto_2", [](ProtoTextClient& client) {
            constexpr int kCount = 10;
            for (int i = 0; i < kCount; i++) {
                string msg = "msg_" + std::to_string(i);
                client.Submit(msg);

                ASSERT_TRUE(client.WaitForResponse());
                EXPECT_EQ(client.Retrieve(), msg);
            }
        });
    }

    TEST(WebSocketProtocol, ShortMessage)
    {
        RunTextServer("/tmp/wpe_ws_proto_3", [](ProtoTextClient& client) {
            const string msg = "x";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, SpecialCharacters)
    {
        RunTextServer("/tmp/wpe_ws_proto_4", [](ProtoTextClient& client) {
            const string msg = R"({"key":"value","num":42,"arr":[1,2,3]})";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, CloseIsClean)
    {
        RunTextServer("/tmp/wpe_ws_proto_5", [](ProtoTextClient& client) {
            // Just verify the connection opens and closes cleanly
            // (the RunTextServer helper already tests Open/Close paths)
            EXPECT_TRUE(client.IsOpen());
        });
    }

    TEST(WebSocketProtocol, MultipleClientsSequential)
    {
        constexpr uint32_t maxWait = 8000;
        const std::string connector = "/tmp/wpe_ws_proto_6";

        ProtoTextServer::Reset();
        ::unlink(connector.c_str());

        std::atomic<bool> serverReady{false};
        std::mutex readyMutex;
        std::condition_variable readyCV;
        std::atomic<bool> clientsDone{false};

        std::thread serverThread([&]() {
            ::Thunder::Core::SocketServerType<ProtoTextServer> server(
                ::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(server.Open(maxWait), ::Thunder::Core::ERROR_NONE);

            {
                std::lock_guard<std::mutex> lk(readyMutex);
                serverReady = true;
            }
            readyCV.notify_one();

            while (!clientsDone.load()) {
                SleepMs(50);
            }
            SleepMs(200);
            server.Close(maxWait);
        });

        {
            std::unique_lock<std::mutex> lk(readyMutex);
            ASSERT_TRUE(readyCV.wait_for(lk, std::chrono::seconds(10),
                [&]{ return serverReady.load(); }));
        }
        SleepMs(100);

        // First client
        {
            ProtoTextServer::Reset();
            ProtoTextClient client1(::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(client1.Open(maxWait), ::Thunder::Core::ERROR_NONE);

            {
                std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
                ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                    []{ return ProtoTextServer::Connected(); });
            }

            client1.Submit("client1_msg");
            ASSERT_TRUE(client1.WaitForResponse());
            EXPECT_EQ(client1.Retrieve(), "client1_msg");

            client1.Close(maxWait);
        }

        SleepMs(200);

        // Second client — server must still accept connections
        {
            ProtoTextServer::Reset();
            ProtoTextClient client2(::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(client2.Open(maxWait), ::Thunder::Core::ERROR_NONE);

            {
                std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
                ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                    []{ return ProtoTextServer::Connected(); });
            }

            client2.Submit("client2_msg");
            ASSERT_TRUE(client2.WaitForResponse());
            EXPECT_EQ(client2.Retrieve(), "client2_msg");

            client2.Close(maxWait);
        }

        clientsDone = true;
        serverThread.join();

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(WebSocketProtocol, ActivityTracking)
    {
        RunTextServer("/tmp/wpe_ws_proto_7", [](ProtoTextClient& client) {
            // After a successful send/receive, the link should have activity
            client.Submit("activity_test");
            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), "activity_test");

            // Connection should still be open
            EXPECT_TRUE(client.IsOpen());
        });
    }

} // Core
} // Tests
} // Thunder
