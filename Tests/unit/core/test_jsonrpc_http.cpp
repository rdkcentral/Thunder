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

    // =========================================================================
    // TEST FILE: test_jsonrpc_http.cpp
    //
    // Purpose:
    //   Validates JSON-RPC 2.0 message round-trip over an HTTP POST transport
    //   layer. Tests exercise the WebLinkType<SocketStream, Request, Response>
    //   stack directly (no PluginHost, no COM channel), ensuring that
    //   JSONRPC::Message objects can be embedded in HTTP request/response
    //   bodies via JSONBodyType, transmitted over TCP, deserialized on the
    //   server, dispatched to a JSONRPC::Handler, and the response returned
    //   back to the client inside an HTTP 200 response.
    //
    // Architecture:
    //   - Each test uses IPTestAdministrator which fork()s into a child
    //     (server) and parent (client) process, synchronized via shared-
    //     memory futex handshakes.
    //   - Server: SocketServerType< JSONRPCHTTPServer > listens on a TCP
    //     port. On receiving an HTTP POST with a JSONRPCBody, it invokes
    //     the registered handler method and sends an HTTP response with
    //     the JSON-RPC result in the body.
    //   - Client: JSONRPCHTTPClient connects via TCP, constructs HTTP POST
    //     requests with JSONRPCBody payloads, and collects responses via
    //     a thread-safe queue (mutex + condition_variable).
    //
    // Registered server methods:
    //   "add"           - Adds two integers {"a":<int>,"b":<int>} -> sum
    //   "echo"          - Echoes back the parameters string unchanged
    //   "error"         - Always returns ERROR_UNKNOWN_METHOD
    //   "largeResponse" - Returns a 4096-byte string to test large HTTP bodies
    //
    // HTTP specifics tested:
    //   - JSON-RPC payloads are carried as HTTP POST request/response bodies
    //   - Server validates HTTP verb (must be POST) and body presence
    //   - HTTP status code is always 200 for valid JSON-RPC (errors are
    //     in the JSON-RPC Error object, not the HTTP status)
    //   - Multiple requests can be sent over the same TCP connection
    // =========================================================================

    // =========================================================================
    // JSONRPCBody
    //
    // Type alias for Web::JSONBodyType<JSONRPC::Message>. JSONBodyType
    // wraps a JSON-serializable object as an HTTP body: it handles
    // Content-Type headers and serialization/deserialization of the JSON
    // payload within HTTP request/response framing.
    // =========================================================================
    typedef Web::JSONBodyType<::Thunder::Core::JSONRPC::Message> JSONRPCBody;

    // =========================================================================
    // JSONRPCHTTPServer
    //
    // A per-connection HTTP server handler instantiated by SocketServerType
    // for each accepted TCP connection. It:
    //   1. Attaches a JSONRPCBody to each incoming request via LinkBody()
    //      so the HTTP layer knows how to deserialize the POST body
    //   2. In Received(), validates the HTTP method (POST) and body presence
    //   3. Extracts the JSONRPC::Message from the body, dispatches to the
    //      JSONRPC::Handler, and builds a response JSONRPC::Message
    //   4. Wraps the response in an HTTP 200 with a JSONRPCBody and submits
    //
    // The server always returns HTTP 200 for JSON-RPC requests; application-
    // level errors are communicated via the JSON-RPC Error object in the
    // response body, following the JSON-RPC 2.0 specification.
    // =========================================================================
    class JSONRPCHTTPServer
        : public Web::WebLinkType<
              ::Thunder::Core::SocketStream,
              Web::Request,
              Web::Response,
              ::Thunder::Core::ProxyPoolType<Web::Request>&> {
    private:
        typedef Web::WebLinkType<
            ::Thunder::Core::SocketStream,
            Web::Request,
            Web::Response,
            ::Thunder::Core::ProxyPoolType<Web::Request>&>
            BaseClass;

        static constexpr uint32_t maxWaitTimeMs = 4000;

    public:
        JSONRPCHTTPServer() = delete;
        JSONRPCHTTPServer(const JSONRPCHTTPServer&) = delete;
        JSONRPCHTTPServer& operator=(const JSONRPCHTTPServer&) = delete;

        JSONRPCHTTPServer(
            const SOCKET& connector,
            const ::Thunder::Core::NodeId& remoteId,
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer>*)
            : BaseClass(5, _requestFactory, false, connector, remoteId, 2048, 2048)
            , _requestFactory(5)
            , _jsonrpcBodyFactory(5)
            , _handler({ 1 })
        {
            // Register JSON-RPC methods
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

            _handler.Register(_T("echo"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
                    result = parameters;
                    return ::Thunder::Core::ERROR_NONE;
                }
            ));

            _handler.Register(_T("error"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
                    return ::Thunder::Core::ERROR_UNKNOWN_METHOD;
                }
            ));

            _handler.Register(_T("largeResponse"), ::Thunder::Core::JSONRPC::InvokeFunction(
                [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
                    ::Thunder::Core::JSON::String data;
                    data = string(4096, 'Y');
                    data.ToString(result);
                    return ::Thunder::Core::ERROR_NONE;
                }
            ));
        }

        virtual ~JSONRPCHTTPServer()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        virtual void LinkBody(::Thunder::Core::ProxyType<Web::Request>& element)
        {
            // Attach a JSON-RPC body to incoming requests
            element->Body<JSONRPCBody>(_jsonrpcBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<Web::Request>& request)
        {
            // Build an HTTP response object.
            ::Thunder::Core::ProxyType<Web::Response> response(
                ::Thunder::Core::ProxyType<Web::Response>::Create());

            // Validate that the request is a POST with a body.
            // JSON-RPC over HTTP mandates POST; GET or other verbs are rejected.
            if (request->Verb != Web::Request::HTTP_POST) {
                response->ErrorCode = Web::STATUS_METHOD_NOT_ALLOWED;
                response->Message = _T("JSON-RPC requires POST");
            } else if (!request->HasBody()) {
                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                response->Message = _T("Missing body");
            } else {
                ::Thunder::Core::ProxyType<JSONRPCBody> body = request->Body<JSONRPCBody>();

                if (!body.IsValid()) {
                    response->ErrorCode = Web::STATUS_BAD_REQUEST;
                    response->Message = _T("Invalid JSON-RPC body");
                } else {
                    // Process the JSON-RPC request
                    ::Thunder::Core::ProxyType<JSONRPCBody> responseBody(
                        ::Thunder::Core::ProxyType<JSONRPCBody>::Create());

                    responseBody->JSONRPC = _T("2.0");

                    if (body->Id.IsSet()) {
                        responseBody->Id = body->Id.Value();
                    }

                    string method = ::Thunder::Core::JSONRPC::Message::Method(body->Designator.Value());
                    string output;
                    ::Thunder::Core::JSONRPC::Context context(0, body->Id.Value(), _T(""));

                    uint32_t result = _handler.Invoke(context, method, body->Parameters.Value(), output);

                    if (result == ::Thunder::Core::ERROR_NONE) {
                        if (output.empty()) {
                            responseBody->Result.Null(true);
                        } else {
                            responseBody->Result = output;
                        }
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("JSONRPC executed successfully");
                    } else {
                        responseBody->Error.SetError(result);
                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = _T("JSONRPC error");
                    }

                    response->Body<JSONRPCBody>(responseBody);
                }
            }

            response->AccessControlOrigin = _T("*");
            EXPECT_TRUE(Submit(response));
        }

        virtual void Send(const ::Thunder::Core::ProxyType<Web::Response>& response VARIABLE_IS_NOT_USED)
        {
        }

        virtual void StateChange()
        {
        }

    private:
        ::Thunder::Core::ProxyPoolType<Web::Request> _requestFactory;
        ::Thunder::Core::ProxyPoolType<JSONRPCBody> _jsonrpcBodyFactory;
        ::Thunder::Core::JSONRPC::Handler _handler;
    };

    // =========================================================================
    // JSONRPCHTTPClient
    //
    // HTTP client that sends JSON-RPC requests as HTTP POST and collects
    // responses. Key design points:
    //   - LinkBody() attaches a JSONRPCBody to each incoming HTTP response
    //     so the framework knows how to deserialize the JSON response body
    //   - Received() extracts the JSONRPC body, serializes it to a string,
    //     and pushes it into a thread-safe queue for the test to consume
    //   - SendJSONRPC() is a helper that constructs a well-formed HTTP POST
    //     request with the JSON-RPC message as the body
    //   - WaitForResponse() blocks until a response is available in the queue
    //     (with a configurable timeout to prevent test hangs)
    // =========================================================================
    class JSONRPCHTTPClient
        : public Web::WebLinkType<
              ::Thunder::Core::SocketStream,
              Web::Response,
              Web::Request,
              ::Thunder::Core::ProxyPoolType<Web::Response>&> {
    private:
        typedef Web::WebLinkType<
            ::Thunder::Core::SocketStream,
            Web::Response,
            Web::Request,
            ::Thunder::Core::ProxyPoolType<Web::Response>&>
            BaseClass;

        static constexpr uint32_t maxWaitTimeMs = 4000;

    public:
        JSONRPCHTTPClient() = delete;
        JSONRPCHTTPClient(const JSONRPCHTTPClient&) = delete;
        JSONRPCHTTPClient& operator=(const JSONRPCHTTPClient&) = delete;

        JSONRPCHTTPClient(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
            , _responseFactory(5)
            , _jsonrpcBodyFactory(5)
            , _httpStatusCode(0)
        {
        }

        virtual ~JSONRPCHTTPClient()
        {
            EXPECT_EQ(Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        }

    public:
        virtual void LinkBody(::Thunder::Core::ProxyType<Web::Response>& element)
        {
            element->Body<JSONRPCBody>(_jsonrpcBodyFactory.Element());
        }

        virtual void Received(::Thunder::Core::ProxyType<Web::Response>& response)
        {
            _httpStatusCode = response->ErrorCode;

            if (response->HasBody()) {
                ::Thunder::Core::ProxyType<JSONRPCBody> body = response->Body<JSONRPCBody>();
                if (body.IsValid()) {
                    string text;
                    body->ToString(text);
                    std::lock_guard<std::mutex> lock(_responseMutex);
                    _responseQueue.push(text);
                    _responseCV.notify_one();
                }
            }
        }

        virtual void Send(const ::Thunder::Core::ProxyType<Web::Request>& request VARIABLE_IS_NOT_USED)
        {
        }

        virtual void StateChange()
        {
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

        uint16_t StatusCode() const
        {
            return _httpStatusCode;
        }

        // Build and submit a JSON-RPC POST request.
        // Constructs a well-formed HTTP POST to /jsonrpc with the given
        // JSONRPC::Message serialized as the body via JSONBodyType.
        bool SendJSONRPC(const ::Thunder::Core::JSONRPC::Message& message)
        {
            ::Thunder::Core::ProxyType<Web::Request> request(
                ::Thunder::Core::ProxyType<Web::Request>::Create());
            ::Thunder::Core::ProxyType<JSONRPCBody> body(
                ::Thunder::Core::ProxyType<JSONRPCBody>::Create());

            // Copy JSONRPC message fields into the body object
            body->JSONRPC = message.JSONRPC.Value();
            if (message.Id.IsSet()) {
                body->Id = message.Id.Value();
            }
            if (message.Designator.IsSet()) {
                body->Designator = message.Designator.Value();
            }
            if (message.Parameters.IsSet()) {
                body->Parameters = message.Parameters.Value();
            }

            request->Verb = Web::Request::HTTP_POST;
            request->Path = _T("/jsonrpc");
            request->Body<JSONRPCBody>(body);

            return Submit(request);
        }

    private:
        string _dataReceived;
        uint16_t _httpStatusCode;
        ::Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;
        ::Thunder::Core::ProxyPoolType<JSONRPCBody> _jsonrpcBodyFactory;
        std::queue<string> _responseQueue;
        std::mutex _responseMutex;
        std::condition_variable _responseCV;
    };

    // =========================================================================
    // HTTP JSON-RPC Tests
    //
    // Each test follows the IPTestAdministrator fork() pattern:
    //   - Child process: starts an HTTP server on a unique TCP port
    //   - Parent process: connects an HTTP client, sends POST requests
    //     with JSON-RPC bodies, validates responses
    //   - Handshake signals coordinate startup/shutdown ordering
    //
    // Each test uses a unique port (12350-12354) to avoid bind conflicts.
    // =========================================================================

    // Verifies basic JSON-RPC "add" method dispatch over HTTP POST.
    // Sends {a:15, b:25} and expects result "40".
    // Also validates HTTP status code is 200 (OK).
    TEST(HTTPJSONRPC, BasicMethodInvocation)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        const std::string connector{ "0.0.0.0" };
        const uint16_t port = 12350;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer> server(
                ::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONRPCHTTPClient client(::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_TRUE(client.IsOpen());

            // --- Test: Add method via HTTP POST ---
            {
                ::Thunder::Core::JSONRPC::Message request;
                request.JSONRPC = _T("2.0");
                request.Id = 1;
                request.Designator = _T("add");
                request.Parameters = _T("{\"a\":15,\"b\":25}");

                EXPECT_TRUE(client.SendJSONRPC(request));

                ASSERT_TRUE(client.WaitForResponse());

                EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 1u);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), "40");
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, 8);

        ::Thunder::Core::Singleton::Dispose();
    }

    // Verifies the "echo" method over HTTP POST. Sends a nested JSON
    // parameter object and expects the server to return it verbatim as
    // the result string. Tests that complex JSON structures survive the
    // HTTP POST -> JSONRPCBody -> Handler -> JSONRPCBody round-trip.
    TEST(HTTPJSONRPC, EchoMethod)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        const std::string connector{ "0.0.0.0" };
        const uint16_t port = 12351;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer> server(
                ::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONRPCHTTPClient client(::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_TRUE(client.IsOpen());

            // --- Test: Echo method via HTTP POST ---
            {
                ::Thunder::Core::JSONRPC::Message request;
                request.JSONRPC = _T("2.0");
                request.Id = 2;
                request.Designator = _T("echo");
                request.Parameters = _T("{\"key\":\"value\",\"nested\":{\"inner\":true}}");

                EXPECT_TRUE(client.SendJSONRPC(request));

                ASSERT_TRUE(client.WaitForResponse());

                EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 2u);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), "{\"key\":\"value\",\"nested\":{\"inner\":true}}");
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, 8);

        ::Thunder::Core::Singleton::Dispose();
    }

    // Verifies error handling over HTTP. Calls the "error" method which
    // returns ERROR_UNKNOWN_METHOD. The server should still respond with
    // HTTP 200 (the error is in the JSON-RPC Error object, not the HTTP
    // status), and the Error.Code should be -32601.
    TEST(HTTPJSONRPC, ErrorResponse)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        const std::string connector{ "0.0.0.0" };
        const uint16_t port = 12352;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer> server(
                ::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONRPCHTTPClient client(::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_TRUE(client.IsOpen());

            // --- Test: Error method ---
            {
                ::Thunder::Core::JSONRPC::Message request;
                request.JSONRPC = _T("2.0");
                request.Id = 3;
                request.Designator = _T("error");
                request.Parameters = _T("{}");

                EXPECT_TRUE(client.SendJSONRPC(request));

                ASSERT_TRUE(client.WaitForResponse());

                EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 3u);
                EXPECT_TRUE(response.Error.IsSet());
                EXPECT_EQ(response.Error.Code.Value(), -32601);
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, 8);

        ::Thunder::Core::Singleton::Dispose();
    }

    // Verifies that a large JSON-RPC response (4096 bytes of 'Y' payload)
    // is correctly transmitted over HTTP. Tests HTTP chunked transfer or
    // Content-Length handling with payloads exceeding the typical buffer
    // size (2048 bytes configured on client/server).
    TEST(HTTPJSONRPC, LargePayload)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        const std::string connector{ "0.0.0.0" };
        const uint16_t port = 12353;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer> server(
                ::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONRPCHTTPClient client(::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_TRUE(client.IsOpen());

            // --- Test: Large response body ---
            {
                ::Thunder::Core::JSONRPC::Message request;
                request.JSONRPC = _T("2.0");
                request.Id = 1;
                request.Designator = _T("largeResponse");
                request.Parameters = _T("{}");

                EXPECT_TRUE(client.SendJSONRPC(request));

                ASSERT_TRUE(client.WaitForResponse());

                EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), 1u);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_FALSE(response.Error.IsSet());

                ::Thunder::Core::JSON::String resultStr;
                resultStr.FromString(response.Result.Value());
                EXPECT_EQ(resultStr.Value().length(), 4096u);
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, 8);

        ::Thunder::Core::Singleton::Dispose();
    }

    // Sends 5 sequential "add" requests over the same HTTP connection.
    // Each request uses i + i*10 as operands (e.g. i=1 -> {a:1,b:10} = 11).
    // Validates that each response has the correct Id and result, ensuring
    // HTTP keep-alive and request/response correlation work correctly
    // across multiple round-trips on a persistent TCP connection.
    TEST(HTTPJSONRPC, MultipleSequentialRequests)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        const std::string connector{ "0.0.0.0" };
        const uint16_t port = 12354;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONRPCHTTPServer> server(
                ::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            JSONRPCHTTPClient client(::Thunder::Core::NodeId(connector.c_str(), port));

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_TRUE(client.IsOpen());

            // Send multiple requests over the same connection
            for (uint32_t i = 1; i <= 5; i++) {
                ::Thunder::Core::JSONRPC::Message request;
                request.JSONRPC = _T("2.0");
                request.Id = i;
                request.Designator = _T("add");

                string params = _T("{\"a\":") + std::to_string(i) + _T(",\"b\":") + std::to_string(i * 10) + _T("}");
                request.Parameters = params;

                EXPECT_TRUE(client.SendJSONRPC(request));

                ASSERT_TRUE(client.WaitForResponse());

                EXPECT_EQ(client.StatusCode(), Web::STATUS_OK);

                ::Thunder::Core::JSONRPC::Message response;
                client.RetrieveMessage(response);

                EXPECT_EQ(response.Id.Value(), i);
                EXPECT_TRUE(response.Result.IsSet());
                EXPECT_STREQ(response.Result.Value().c_str(), std::to_string(i + i * 10).c_str());
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, 8);

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder