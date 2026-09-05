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
#include <functional>
#include <mutex>
#include <thread>
#include <unistd.h>

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
    //   Tests WebSocket text echo over Unix domain sockets:
    //   - Text echo round-trip
    //   - Large message fragmentation (exceeding buffer size)
    //   - Rapid sequential messages
    //   - Short and special-character messages
    //   - Clean close behavior
    //   - Multiple sequential clients on same server
    //   - Activity tracking after send/receive
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

        static void Reset() {
            std::lock_guard<std::mutex> lk(s_mutex);
            s_connected = false;
        }
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
    // Helper: generate unique socket path per test invocation
    // =========================================================================
    static std::atomic<uint32_t> s_socketCounter{0};

    static std::string UniqueSocketPath(const char* tag)
    {
        return std::string("/tmp/wpe_ws_") + tag + "_"
            + std::to_string(::getpid()) + "_"
            + std::to_string(s_socketCounter.fetch_add(1));
    }

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

        std::atomic<uint32_t> serverOpenResult{::Thunder::Core::ERROR_GENERAL};

        std::thread serverThread([&]() {
            ::Thunder::Core::SocketServerType<ProtoTextServer> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            const uint32_t result = server.Open(maxWait);
            serverOpenResult = result;

            {
                std::lock_guard<std::mutex> lk(readyMutex);
                serverReady = (result == ::Thunder::Core::ERROR_NONE);
            }
            readyCV.notify_one();

            if (result != ::Thunder::Core::ERROR_NONE) {
                return;
            }

            while (!clientDone.load()) {
                SleepMs(50);
            }

            SleepMs(200);
            server.Close(maxWait);
    });

        // Ensure the server thread is always joined before returning
        auto stopServer = [&]() {
            clientDone = true;
            if (serverThread.joinable()) {
                serverThread.join();
            }
            ::Thunder::Core::Singleton::Dispose();
        };

        bool ready = false;
        {
            std::unique_lock<std::mutex> lk(readyMutex);
            ready = readyCV.wait_for(lk, std::chrono::seconds(10),
                [&]{ return serverReady.load(); });
        }

        if (!ready) {
            stopServer();
            FAIL() << "Server did not become ready in time";
            return;
        }

        if (serverOpenResult != ::Thunder::Core::ERROR_NONE) {
            stopServer();
            FAIL() << "server.Open() failed: " << serverOpenResult.load();
            return;
        }

        SleepMs(100);

        ProtoTextClient client(::Thunder::Core::NodeId(connector.c_str()));
        if (client.Open(maxWait) != ::Thunder::Core::ERROR_NONE) {
            stopServer();
            FAIL() << "Client Open() failed";
            return;
        }
        if (!client.IsOpen()) {
            stopServer();
            FAIL() << "Client is not open after Open()";
            return;
        }

        {
            std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
            bool connected = ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                []{ return ProtoTextServer::Connected(); });
            if (!connected) {
                client.Close(maxWait);
                stopServer();
                FAIL() << "Server did not register client connection";
                return;
            }
        }

        clientLogic(client);

        EXPECT_EQ(client.Close(maxWait), ::Thunder::Core::ERROR_NONE);
        stopServer();
    }

    // =========================================================================
    // Tests
    // =========================================================================

    TEST(WebSocketProtocol, TextEchoRoundTrip)
    {
        RunTextServer(UniqueSocketPath("echo"), [](ProtoTextClient& client) {
            const string msg = "Hello WebSocket";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, LargeMessage)
    {
        RunTextServer(UniqueSocketPath("large"), [](ProtoTextClient& client) {
            // 2KB message — exceeds single 1024-byte buffer, tests fragmentation
            const string msg(2048, 'A');
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, MultipleRapidMessages)
    {
        RunTextServer(UniqueSocketPath("rapid"), [](ProtoTextClient& client) {
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
        RunTextServer(UniqueSocketPath("short"), [](ProtoTextClient& client) {
            const string msg = "x";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, SpecialCharacters)
    {
        RunTextServer(UniqueSocketPath("special"), [](ProtoTextClient& client) {
            const string msg = R"({"key":"value","num":42,"arr":[1,2,3]})";
            client.Submit(msg);

            ASSERT_TRUE(client.WaitForResponse());
            EXPECT_EQ(client.Retrieve(), msg);
        });
    }

    TEST(WebSocketProtocol, MultipleClientsSequential)
    {
        constexpr uint32_t maxWait = 8000;
        const std::string connector = UniqueSocketPath("multi");

        ProtoTextServer::Reset();
        ::unlink(connector.c_str());

        std::atomic<bool> serverReady{false};
        std::mutex readyMutex;
        std::condition_variable readyCV;
        std::atomic<bool> clientsDone{false};

        std::thread serverThread([&]() {
            ::Thunder::Core::SocketServerType<ProtoTextServer> server(
                ::Thunder::Core::NodeId(connector.c_str()));
            bool opened = (server.Open(maxWait) == ::Thunder::Core::ERROR_NONE);
            EXPECT_TRUE(opened) << "server.Open() failed";

            {
                std::lock_guard<std::mutex> lk(readyMutex);
                serverReady = opened;
            }
            readyCV.notify_one();

            if (!opened) return;

            while (!clientsDone.load()) {
                SleepMs(50);
            }
            SleepMs(200);
            server.Close(maxWait);
        });

        // RAII guard: always signal and join the server thread on exit
        auto stopServer = [&]() {
            clientsDone = true;
            if (serverThread.joinable()) {
                serverThread.join();
            }
            ::Thunder::Core::Singleton::Dispose();
        };

        bool ready = false;
        {
            std::unique_lock<std::mutex> lk(readyMutex);
            ready = readyCV.wait_for(lk, std::chrono::seconds(10),
                [&]{ return serverReady.load(); });
        }
        if (!ready || !serverReady.load()) {
            stopServer();
            FAIL() << "Server did not become ready in time (or Open() failed)";
            return;
        }
        SleepMs(100);

        // First client
        {
            ProtoTextServer::Reset();
            ProtoTextClient client1(::Thunder::Core::NodeId(connector.c_str()));
            if (client1.Open(maxWait) != ::Thunder::Core::ERROR_NONE) {
                stopServer();
                FAIL() << "Client1 Open() failed";
                return;
            }

            {
                std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
                ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                    []{ return ProtoTextServer::Connected(); });
            }

            client1.Submit("client1_msg");
            if (!client1.WaitForResponse()) {
                client1.Close(maxWait);
                stopServer();
                FAIL() << "Client1 did not receive response";
                return;
            }
            EXPECT_EQ(client1.Retrieve(), "client1_msg");

            client1.Close(maxWait);
        }

        SleepMs(200);

        // Second client — server must still accept connections
        {
            ProtoTextServer::Reset();
            ProtoTextClient client2(::Thunder::Core::NodeId(connector.c_str()));
            if (client2.Open(maxWait) != ::Thunder::Core::ERROR_NONE) {
                stopServer();
                FAIL() << "Client2 Open() failed";
                return;
            }

            {
                std::unique_lock<std::mutex> lk(ProtoTextServer::s_mutex);
                ProtoTextServer::s_cv.wait_for(lk, std::chrono::seconds(5),
                    []{ return ProtoTextServer::Connected(); });
            }

            client2.Submit("client2_msg");
            if (!client2.WaitForResponse()) {
                client2.Close(maxWait);
                stopServer();
                FAIL() << "Client2 did not receive response";
                return;
            }
            EXPECT_EQ(client2.Retrieve(), "client2_msg");

            client2.Close(maxWait);
        }

        stopServer();
    }

    TEST(WebSocketProtocol, ActivityTracking)
    {
        RunTextServer(UniqueSocketPath("activity"), [](ProtoTextClient& client) {
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
