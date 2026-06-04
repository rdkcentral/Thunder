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
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>

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
    // TEST FILE: test_jsonrpc_websocket.cpp
    //
    // Purpose:
    //   Validates JSON-RPC 2.0 message round-trip over a raw WebSocket
    //   transport layer. Tests exercise the StreamJSONType<WebSocket*Type>
    //   stack directly (no PluginHost, no COM channel), ensuring that
    //   JSONRPC::Message objects can be serialized, transmitted through a
    //   WebSocket connection, deserialized on the server, dispatched to a
    //   JSONRPC::Handler, and the response returned back to the client.
    //
    // Architecture:
    //   - Each test runs a server in a background thread and client in the
    //     test thread, synchronized via mutex/condition_variable.
    //   - Server: SocketServerType< JSONRPCWebSocketServer > listens on a
    //     Unix domain socket. On receiving a JSONRPC::Message it invokes
    //     the registered handler method and sends the response back.
    //   - Client: JSONRPCWebSocketClient connects to the server, submits
    //     JSONRPC::Message objects, and collects responses via a thread-
    //     safe queue (mutex + condition_variable).
    //
    // Registered server methods:
    //   "add"           - Adds two integers {"a":<int>,"b":<int>} -> sum
    //   "echo"          - Echoes back the parameters string unchanged
    //   "error"         - Always returns ERROR_UNKNOWN_METHOD
    //   "largeResponse" - Returns a 4096-byte string to test frame
    //                     fragmentation on WebSocket
    // =========================================================================

    // =========================================================================
    // JSONRPCMessageFactory
    //
    // A ProxyPool-based allocator for JSONRPC::Message objects, required by
    // StreamJSONType as its ALLOCATOR template parameter. The Element()
    // method returns a new or recycled JSONRPC::Message wrapped as an
    // IElement proxy, which the deserializer will populate from the wire.
    // =========================================================================

    class JSONRPCMessageFactory : public ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message> {
    public:
        JSONRPCMessageFactory() = delete;
        JSONRPCMessageFactory(const JSONRPCMessageFactory&) = delete;
        JSONRPCMessageFactory& operator=(const JSONRPCMessageFactory&) = delete;

        JSONRPCMessageFactory(const uint32_t number)
            : ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message>(number)
        {
        }

        ~JSONRPCMessageFactory() = default;

    public:
        ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement> Element(const string&)
        {
            return ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(
                ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message>::Element());
        }
    };

    // =========================================================================
    // JSONRPCWebSocketServer
    //
    // A per-connection server handler instantiated by SocketServerType for
    // each accepted WebSocket connection. It:
    //   1. Deserializes incoming bytes into JSONRPC::Message via StreamJSONType
    //   2. Extracts the method name from Message::Designator
    //   3. Dispatches to the JSONRPC::Handler registered in the constructor
    //   4. Populates either Result or Error on a new response Message
    //   5. Serializes the response back over the same WebSocket connection
    //
    // StateChange() signals when a client connects (used by tests to wait
    // for the connection to be established before proceeding).
    // =========================================================================

    template <typename INTERFACE>
    class JSONRPCWebSocketServer
        : public ::Thunder::Core::StreamJSONType<
              Web::WebSocketServerType<::Thunder::Core::SocketStream>,
              JSONRPCMessageFactory&,
              INTERFACE> {
    private:
        typedef ::Thunder::Core::StreamJSONType<
            Web::WebSocketServerType<::Thunder::Core::SocketStream>,
            JSONRPCMessageFactory&,
            INTERFACE>
            BaseClass;

    public:
        JSONRPCWebSocketServer() = delete;
        JSONRPCWebSocketServer(const JSONRPCWebSocketServer&) = delete;
        JSONRPCWebSocketServer& operator=(const JSONRPCWebSocketServer&) = delete;

        JSONRPCWebSocketServer(
            const SOCKET& socket,
            const ::Thunder::Core::NodeId& remoteNode,
            ::Thunder::Core::SocketServerType<JSONRPCWebSocketServer<INTERFACE>>*)
            : BaseClass(2, _objectFactory, false, false, false, socket, remoteNode, 1024, 1024)
            , _objectFactory(5)
            , _handler({ 1 })  // version array for JSONRPC::Handler
        {
            // Register test methods on the handler.
            // "add": parses {"a":<int>,"b":<int>} and returns the sum as a string.
            _handler.Register(_T("add"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    ::Thunder::Core::JSON::DecSInt32 a, b;
                    ::Thunder::Core::JSON::Container params;
                    params.Add(_T("a"), &a);
                    params.Add(_T("b"), &b);
                    params.FromString(parameters);
                    ::Thunder::Core::JSON::DecSInt32 sum;
                    sum = a.Value() + b.Value();
                    sum.ToString(result);
                    return ::Thunder::Core::ERROR_NONE;
                }
            ));

            // "echo": returns the raw parameters string back to the caller.
            _handler.Register(_T("echo"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    result = parameters;
                    return ::Thunder::Core::ERROR_NONE;
                }
            ));

            // "error": always returns ERROR_UNKNOWN_METHOD to test error path.
            _handler.Register(_T("error"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
                    return ::Thunder::Core::ERROR_UNKNOWN_METHOD;
                }
            ));

            // "largeResponse": returns a 4KB string to stress WebSocket frame
            // fragmentation (buffer sizes are 1024 bytes).
            _handler.Register(_T("largeResponse"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                    // Generate a response larger than a single WebSocket frame (>125 bytes)
                    ::Thunder::Core::JSON::String data;
                    data = string(4096, 'X');
                    data.ToString(result);
                    return ::Thunder::Core::ERROR_NONE;
                }
            ));
        }

        virtual ~JSONRPCWebSocketServer()
        {
        }

    public:
        virtual bool IsIdle() const
        {
            return true;
        }

        virtual void StateChange()
        {
            if (this->IsOpen()) {
                std::unique_lock<std::mutex> lk(_mutex);
                _attached = true;
                _cv.notify_one();
            }
        }

        bool IsAttached() const
        {
            return this->IsOpen();
        }

        virtual void Received(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& jsonObject)
        {
            // Cast the generic IElement to a JSONRPC::Message.
            // Only process if the message has a Designator (method name);
            // ignore malformed or empty messages.
            ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message> inbound(jsonObject);

            if (inbound.IsValid() && inbound->Designator.IsSet()) {
                // Build a new response message and copy the request Id
                // for JSON-RPC request/response correlation.
                ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message> response =
                    ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message>::Create();

                response->JSONRPC = _T("2.0");
                if (inbound->Id.IsSet()) {
                    response->Id = inbound->Id.Value();
                }

                // Dispatch to the registered handler method and populate
                // either the Result field (success) or Error field (failure)
                // on the response message.
                string output;
                string method = ::Thunder::Core::JSONRPC::Message::Method(inbound->Designator.Value());
                ::Thunder::Core::JSONRPC::Context context(0, inbound->Id.Value(), _T(""));

                uint32_t result = _handler.Invoke(context, method, inbound->Parameters.Value(), output);

                if (result == ::Thunder::Core::ERROR_NONE) {
                    if (output.empty()) {
                        response->Result.Null(true);
                    } else {
                        response->Result = output;
                    }
                } else {
                    response->Error.SetError(result);
                }

                this->Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(response));
            }
        }

        virtual void Send(VARIABLE_IS_NOT_USED ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& jsonObject)
        {
        }

        static bool GetState()
        {
            return _attached;
        }

        static void Reset()
        {
            _attached = false;
        }

    private:
        JSONRPCMessageFactory _objectFactory;
        ::Thunder::Core::JSONRPC::Handler _handler;

        static bool _attached;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    template <typename INTERFACE>
    std::mutex JSONRPCWebSocketServer<INTERFACE>::_mutex;
    template <typename INTERFACE>
    std::condition_variable JSONRPCWebSocketServer<INTERFACE>::_cv;
    template <typename INTERFACE>
    bool JSONRPCWebSocketServer<INTERFACE>::_attached = false;

    // =========================================================================
    // JSONRPCWebSocketClient
    //
    // WebSocket client that sends JSONRPC::Message objects and collects
    // responses. Responses are queued in a thread-safe std::queue and
    // signaled via condition_variable so that tests can synchronously
    // wait for each response with WaitForResponse().
    //
    // Filtering: The Received() callback only enqueues messages where
    // JSONRPC field is set, ignoring spurious empty deserialization
    // callbacks from StreamJSONType.
    // =========================================================================

    template <typename INTERFACE>
    class JSONRPCWebSocketClient
        : public ::Thunder::Core::StreamJSONType<
              Web::WebSocketClientType<::Thunder::Core::SocketStream>,
              JSONRPCMessageFactory&,
              INTERFACE> {
    private:
        typedef ::Thunder::Core::StreamJSONType<
            Web::WebSocketClientType<::Thunder::Core::SocketStream>,
            JSONRPCMessageFactory&,
            INTERFACE>
            BaseClass;

    public:
        JSONRPCWebSocketClient() = delete;
        JSONRPCWebSocketClient(const JSONRPCWebSocketClient&) = delete;
        JSONRPCWebSocketClient& operator=(const JSONRPCWebSocketClient&) = delete;

        JSONRPCWebSocketClient(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _objectFactory, _T(""), _T(""), _T(""), _T(""), false, true, false,
                        remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _objectFactory(5)
        {
        }

        virtual ~JSONRPCWebSocketClient()
        {
        }

    public:
        virtual void Received(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& jsonObject)
        {
            // Only signal for valid JSONRPC responses (filter spurious empty
            // deserialization events from StreamJSONType)
            ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message> msg(jsonObject);
            if (msg.IsValid() && msg->JSONRPC.IsSet()) {
                string textElement;
                EXPECT_TRUE(jsonObject->ToString(textElement));
                std::lock_guard<std::mutex> lock(_responseMutex);
                _responseQueue.push(textElement);
                _responseCV.notify_one();
            }
        }

        virtual void Send(VARIABLE_IS_NOT_USED ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& jsonObject)
        {
        }

        virtual void StateChange()
        {
        }

        virtual bool IsIdle() const
        {
            return true;
        }

        ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message> CreateMessage()
        {
            return ::Thunder::Core::ProxyType<::Thunder::Core::JSONRPC::Message>::Create();
        }

        bool WaitForResponse(uint32_t timeout_ms = 5000)
        {
            std::unique_lock<std::mutex> lock(_responseMutex);
            return _responseCV.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                [this]{ return !_responseQueue.empty(); });
        }

        void RetrieveMessage(::Thunder::Core::JSONRPC::Message& message)
        {
            std::lock_guard<std::mutex> lock(_responseMutex);
            if (!_responseQueue.empty()) {
                message.FromString(_responseQueue.front());
                _responseQueue.pop();
            }
        }

    private:
        JSONRPCMessageFactory _objectFactory;
        std::queue<string> _responseQueue;
        std::mutex _responseMutex;
        std::condition_variable _responseCV;
    };

    // =========================================================================
    // Helper: run server in background thread, wait for it to be ready,
    // run client logic, then shut down.
    // =========================================================================

    static void RunWithServer(const std::string& connector,
        std::function<void(JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement>&)> clientLogic)
    {
        constexpr uint32_t maxWaitTimeMs = 8000;

        JSONRPCWebSocketServer<::Thunder::Core::JSON::IElement>::Reset();

        std::mutex serverReadyMutex;
        std::condition_variable serverReadyCV;
        std::atomic<bool> serverReady{false};
        std::atomic<bool> clientDone{false};

        // Remove stale socket file
        ::unlink(connector.c_str());

        std::thread serverThread([&]() {
            ::Thunder::Core::SocketServerType<JSONRPCWebSocketServer<::Thunder::Core::JSON::IElement>> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Signal that server is listening
            {
                std::lock_guard<std::mutex> lk(serverReadyMutex);
                serverReady = true;
            }
            serverReadyCV.notify_one();

            // Wait until client is done
            while (!clientDone.load()) {
                SleepMs(50);
            }

            // Brief pause to let final responses flush
            SleepMs(200);

            server.Close(maxWaitTimeMs);
        });

        // Wait for server to be ready
        {
            std::unique_lock<std::mutex> lk(serverReadyMutex);
            ASSERT_TRUE(serverReadyCV.wait_for(lk, std::chrono::seconds(10),
                [&]{ return serverReady.load(); }));
        }

        // Brief pause to let server socket settle
        SleepMs(100);

        // Create client and connect
        JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement> client(
            ::Thunder::Core::NodeId(connector.c_str()));

        ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        ASSERT_TRUE(client.IsOpen());

        // Wait for server to register connection
        {
            std::unique_lock<std::mutex> lk(JSONRPCWebSocketServer<::Thunder::Core::JSON::IElement>::_mutex);
            bool attached = JSONRPCWebSocketServer<::Thunder::Core::JSON::IElement>::_cv.wait_for(
                lk, std::chrono::seconds(5),
                []{ return JSONRPCWebSocketServer<::Thunder::Core::JSON::IElement>::GetState(); });
            ASSERT_TRUE(attached);
        }

        // Run client test logic
        clientLogic(client);

        EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

        clientDone = true;
        serverThread.join();

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // WebSocket JSON-RPC Tests
    // =========================================================================

    TEST(WebSocketJSONRPC, BasicMethodInvocation)
    {
        RunWithServer("/tmp/wpe_jsonrpc_ws_test0",
            [](JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement>& client) {
            // --- Test 1: Basic method invocation ("add") ---
            {
                auto msg = client.CreateMessage();
                msg->JSONRPC = _T("2.0");
                msg->Id = 1;
                msg->Designator = _T("add");
                msg->Parameters = _T("{\"a\":10,\"b\":20}");

                client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

                ASSERT_TRUE(client.WaitForResponse());

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 1u);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_FALSE(response.Error.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), "30");
            }

            // --- Test 2: Echo method ---
            {
                auto msg = client.CreateMessage();
                msg->JSONRPC = _T("2.0");
                msg->Id = 2;
                msg->Designator = _T("echo");
                msg->Parameters = _T("{\"message\":\"hello world\"}");

                client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

                ASSERT_TRUE(client.WaitForResponse());

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 2u);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), "{\"message\":\"hello world\"}");
            }

            // --- Test 3: Method returning error ---
            {
                auto msg = client.CreateMessage();
                msg->JSONRPC = _T("2.0");
                msg->Id = 3;
                msg->Designator = _T("error");
                msg->Parameters = _T("{}");

                client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

                ASSERT_TRUE(client.WaitForResponse());

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 3u);
                EXPECT_TRUE(response.Error.IsSet());
                EXPECT_EQ(response.Error.Code.Value(), -32601);
            }
        });
    }

    TEST(WebSocketJSONRPC, LargePayload)
    {
        RunWithServer("/tmp/wpe_jsonrpc_ws_test1",
            [](JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement>& client) {
            auto msg = client.CreateMessage();
            msg->JSONRPC = _T("2.0");
            msg->Id = 1;
            msg->Designator = _T("largeResponse");
            msg->Parameters = _T("{}");

            client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

            ASSERT_TRUE(client.WaitForResponse());

            ::Thunder::Core::JSONRPC::Message response;
            client.RetrieveMessage(response);

            EXPECT_EQ(response.Id.Value(), 1u);
            EXPECT_TRUE(response.Result.IsSet());
            EXPECT_FALSE(response.Error.IsSet());

            ::Thunder::Core::JSON::String resultStr;
            resultStr.FromString(response.Result.Value());
            EXPECT_EQ(resultStr.Value().length(), 4096u);
        });
    }

    TEST(WebSocketJSONRPC, MultipleSequentialRequests)
    {
        RunWithServer("/tmp/wpe_jsonrpc_ws_test2",
            [](JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement>& client) {
            for (uint32_t i = 1; i <= 10; i++) {
                auto msg = client.CreateMessage();
                msg->JSONRPC = _T("2.0");
                msg->Id = i;
                msg->Designator = _T("add");

                string params = _T("{\"a\":") + std::to_string(i) + _T(",\"b\":") + std::to_string(i) + _T("}");
                msg->Parameters = params;

                client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

                ASSERT_TRUE(client.WaitForResponse());

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), i);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), std::to_string(i * 2).c_str());
            }
        });
    }

    TEST(WebSocketJSONRPC, UnknownMethodReturnsError)
    {
        RunWithServer("/tmp/wpe_jsonrpc_ws_test3",
            [](JSONRPCWebSocketClient<::Thunder::Core::JSON::IElement>& client) {
            auto msg = client.CreateMessage();
            msg->JSONRPC = _T("2.0");
            msg->Id = 1;
            msg->Designator = _T("nonExistentMethod");
            msg->Parameters = _T("{}");

            client.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(msg));

            ASSERT_TRUE(client.WaitForResponse());

            ::Thunder::Core::JSONRPC::Message response;
            client.RetrieveMessage(response);

            EXPECT_EQ(response.Id.Value(), 1u);
            EXPECT_TRUE(response.Error.IsSet());
            EXPECT_FALSE(response.Result.IsSet());
        });
    }

} // Core
} // Tests
} // Thunder