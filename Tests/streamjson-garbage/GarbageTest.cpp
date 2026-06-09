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

//
// StreamJSON garbage-data hang reproducer
// ========================================
// Demonstrates issue #1963: an infinite loop inside StreamJSONType::ReceiveData()
// triggered by a single-byte slice landing on the last character of a garbage
// (non-JSON) payload.
//
// How to build (from your WPEFramework build directory):
//   cmake -DSTREAMJSON_GARBAGE_TEST=ON ...
//   make StreamJSONGarbageTest
//
// Expected output WITHOUT the fix:
//   [Server] Connection accepted
//   [Server] State -> OPEN
//   [Main]   Sending garbage: "dfsda" (5 bytes)
//   [Server] Received() called  x4  (bytes 'd','f','s','d' processed, each fires Received)
//   [Main]   Waiting up to 3000 ms for server connection-close event ...
//   [Main]   FAIL: Timed out — server is stuck in ReceiveData() infinite loop
//            (IsNullValue returned UNKNOWN without consuming the last byte 'a')
//
// Expected output WITH the fix (IsNullValue guard changed to loaded >= maxLength):
//   [Server] Received() called  x5
//   [Server] State -> CLOSED
//   [Main]   PASS: Server processed close normally within the timeout
//

#include "Module.h"

#include <cstdio>
#include <cstring>
#include <atomic>

#ifdef __POSIX__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // __POSIX__

namespace WPEFramework {
namespace Test {

    // -----------------------------------------------------------------------
    // Factory — hands out JSONRPC::Message objects as the deserialization target.
    // JSONRPC::Message extends JSON::Container, so Container::Deserialize is the
    // code path that calls IsNullValue when the first byte is not '{'.
    // -----------------------------------------------------------------------
    class JSONFactory : public ::WPEFramework::Core::ProxyPoolType<::WPEFramework::Core::JSONRPC::Message> {
    public:
        JSONFactory() = delete;
        JSONFactory(const JSONFactory&) = delete;
        JSONFactory& operator=(const JSONFactory&) = delete;

        explicit JSONFactory(uint32_t poolSize)
            : ::WPEFramework::Core::ProxyPoolType<::WPEFramework::Core::JSONRPC::Message>(poolSize)
        {
        }

        ~JSONFactory() = default;

        ::WPEFramework::Core::ProxyType<::WPEFramework::Core::JSON::IElement> Element(const string&)
        {
            return ::WPEFramework::Core::ProxyType<::WPEFramework::Core::JSON::IElement>(
                ::WPEFramework::Core::ProxyPoolType<::WPEFramework::Core::JSONRPC::Message>::Element());
        }
    };

    // -----------------------------------------------------------------------
    // Server-side per-connection handler.
    //
    // Accepted by SocketServerType<JSONServer>, so the constructor signature
    // must follow the WPEFramework convention:
    //   (const SOCKET&, const NodeId&, SocketServerType<JSONServer>*)
    //
    // NOTE: _factory is declared AFTER the StreamJSONType base so it is
    // technically initialised after the base-class ctor runs.  The base stores
    // the reference for later use (not in the ctor body), so this is safe in
    // practice and matches the pattern used by the existing unit tests.
    // -----------------------------------------------------------------------
    class JSONServer
        : public ::WPEFramework::Core::StreamJSONType<
              ::WPEFramework::Core::SocketStream,
              JSONFactory&,
              ::WPEFramework::Core::JSON::IElement> {

        using Base = ::WPEFramework::Core::StreamJSONType<
            ::WPEFramework::Core::SocketStream,
            JSONFactory&,
            ::WPEFramework::Core::JSON::IElement>;

    public:
        JSONServer() = delete;
        JSONServer(const JSONServer&) = delete;
        JSONServer& operator=(const JSONServer&) = delete;

        JSONServer(const SOCKET& connector,
                   const ::WPEFramework::Core::NodeId& remoteId,
                   ::WPEFramework::Core::SocketServerType<JSONServer>*)
            : Base(5, s_factory, false, connector, remoteId, 1024, 1024)
        {
            fprintf(stdout, "[Server] Connection accepted from %s\n",
                    remoteId.HostAddress().c_str());
            fflush(stdout);
        }

        ~JSONServer() = default;

    public:
        // Called every time the deserialiser completes one JSON element.
        // With the bug this fires in a tight loop for the last garbage byte;
        // with the fix it fires once for each successfully parsed object.
        void Received(::WPEFramework::Core::ProxyType<::WPEFramework::Core::JSON::IElement>& element) override
        {
            uint32_t n = ++s_receivedCount;
            string text;
            element->ToString(text);
            fprintf(stdout, "[Server] Received() called  (total so far: %u)  payload: %s\n",
                    n, text.c_str());
            if (!text.empty() && text[0] == '{') {
                s_validMessageReceived.store(true);
            }
            fflush(stdout);
        }

        void Send(::WPEFramework::Core::ProxyType<::WPEFramework::Core::JSON::IElement>&) override {}

        void StateChange() override
        {
            fprintf(stdout, "[Server] State -> %s\n", IsOpen() ? "OPEN" : "CLOSED");
            fflush(stdout);
            if (!IsOpen()) {
                s_closedEvent.SetEvent();
            }
        }

    public:
        // Shared across all accepted connections (only one expected in this test).
        static std::atomic<uint32_t>  s_receivedCount;
        static std::atomic<bool>      s_validMessageReceived;
        static ::WPEFramework::Core::Event s_closedEvent;
        static JSONFactory            s_factory;
    };

    std::atomic<uint32_t>  JSONServer::s_receivedCount{ 0 };
    std::atomic<bool>       JSONServer::s_validMessageReceived{ false };
    ::WPEFramework::Core::Event  JSONServer::s_closedEvent{ false, true };
    JSONFactory             JSONServer::s_factory{ 2 };

} // namespace Test
} // namespace WPEFramework

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    using namespace WPEFramework;
    using namespace WPEFramework::Test;

    // Use a high-numbered port that is unlikely to be in use.
    constexpr uint16_t SERVER_PORT  = 19273;
    // How long to wait for the server to signal the connection close.
    // If the bug is present the wait will expire because ReceiveData() never
    // returns, so the socket worker thread can never process the TCP EOF.
    constexpr uint32_t WAIT_MS      = 3000;
    // If Received() is called this many times we know the loop is spinning.
    constexpr uint32_t SPIN_THRESHOLD = 20;

    // ---- 1. Start the server -----------------------------------------------
    ::WPEFramework::Core::NodeId serverNode("0.0.0.0", SERVER_PORT);
    ::WPEFramework::Core::SocketServerType<JSONServer> server(serverNode);

    uint32_t err = server.Open(2000);
    if (err != ::WPEFramework::Core::ERROR_NONE) {
        fprintf(stderr, "[Main]   ERROR: failed to open server socket: %u\n", err);
        return 1;
    }
    fprintf(stdout, "[Main]   Server listening on port %u\n", SERVER_PORT);
    fflush(stdout);

    // Brief pause so the server's accept loop is ready.
    ::SleepMs(200);

    // ---- 2. Connect a raw POSIX client and send garbage --------------------
#ifdef __POSIX__
    int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd < 0) {
        fprintf(stderr, "[Main]   ERROR: socket() failed\n");
        server.Close(2000);
        return 1;
    }

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(SERVER_PORT);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (::connect(clientFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        fprintf(stderr, "[Main]   ERROR: connect() failed\n");
        ::close(clientFd);
        server.Close(2000);
        return 1;
    }
    fprintf(stdout, "[Main]   Raw client connected\n");
    fflush(stdout);

    // Give the server time to call StateChange(OPEN).
    ::SleepMs(200);

    // Send garbage bytes followed immediately by a valid JSONRPC message.
    // With the fix applied:
    //   - The garbage bytes are each rejected with INVALID and discarded.
    //   - The parser resets and successfully deserialises the JSON object.
    //   - Received() is called once with the valid payload.
    // Without the fix, the last garbage byte ('a') stalls the parser
    // forever, so the valid JSON that follows is never reached.
    const char payload[] = "dfsda{\"jsonrpc\":\"2.0\",\"method\":\"test\",\"id\":1}";
    const ssize_t sent   = ::write(clientFd, payload, sizeof(payload) - 1);
    fprintf(stdout, "[Main]   Sent: \"%s\" (%zd bytes)\n", payload, sent);
    fflush(stdout);

    // Small delay so the server's worker thread has time to start processing.
    ::SleepMs(100);

    // Close the client side.  Without the bug fix, this TCP FIN can never
    // propagate to the server because the worker thread is stuck in the loop.
    ::close(clientFd);
#endif // __POSIX__
    fprintf(stdout, "[Main]   Client socket closed (sent TCP FIN)\n");
    fflush(stdout);

    // ---- 3. Wait for the server to process the close -----------------------
    fprintf(stdout, "[Main]   Waiting up to %u ms for server connection-close event ...\n",
            WAIT_MS);
    fflush(stdout);

    uint32_t waitResult = JSONServer::s_closedEvent.Lock(WAIT_MS);

    // ---- 4. Report result --------------------------------------------------
    fprintf(stdout, "\n--- Result ---\n");
    uint32_t received    = JSONServer::s_receivedCount.load();
    bool     validParsed = JSONServer::s_validMessageReceived.load();
    fprintf(stdout, "Received() was called %u time(s) total\n", received);
    fprintf(stdout, "Valid JSON message parsed: %s\n", validParsed ? "YES" : "NO");

    if ((waitResult == ::WPEFramework::Core::ERROR_NONE) && validParsed) {
        fprintf(stdout,
                "PASS: Server processed close normally AND parsed the valid JSON\n"
                "      that followed the garbage input.\n");
    } else {
        fprintf(stdout,
                "FAIL: Timed out after %u ms — server never signalled connection-close.\n",
                WAIT_MS);
        if (received >= SPIN_THRESHOLD) {
            fprintf(stdout,
                    "      Received() has been called %u times, indicating the\n"
                    "      ReceiveData() loop is spinning on the last byte.\n"
                    "      Root cause: IsNullValue() (JSON.h) returns UNKNOWN\n"
                    "      without consuming the byte when maxLength == 1,\n"
                    "      because the guard `loaded + 1 == maxLength` fires\n"
                    "      when loaded==0 and maxLength==1.\n",
                    received);
        } else {
            fprintf(stdout,
                    "      Received() count is low (%u); the loop may not have\n"
                    "      started yet — try increasing WAIT_MS.\n",
                    received);
        }
    }
    fflush(stdout);

    server.Close(2000);
    ::WPEFramework::Core::Singleton::Dispose();

    return ((waitResult == ::WPEFramework::Core::ERROR_NONE) && validParsed) ? 0 : 1;
}
