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
// How to build (from your Thunder build directory):
//   cmake -DSTREAMJSON_GARBAGE_TEST=ON ...
//   make StreamJSONGarbageTest
//
// Expected output WITHOUT the infinite-loop fix:
//   [Server] Received() called  x4  (bytes 'd','f','s','d' processed, each fires Received)
//   [Main]   FAIL: Timed out — server is stuck in ReceiveData() infinite loop
//            (IsNullValue returned UNKNOWN without consuming the last byte 'a')
//
// Expected output WITH both fixes applied (PR #2129 + PR #2133):
//   [Server] CRITICAL CONDITION! StreamJSONType failed: ...  (x5, one per garbage byte)
//   [Server] VALID   Received()   payload: {"jsonrpc":"2.0",...}
//   [Server] State -> CLOSED
//   [Main]   PASS
//
// Note: PR #2133 (log json parse error) changed DeserializerImpl to only call
// Received() on a SUCCESSFUL parse, not on the error path. As a result, garbage
// bytes are now silently discarded (with SYSLOG error) rather than triggering a
// spurious Received() delivery. Spurious count is therefore 0 in the current code.
//

#include "Module.h"

#include <cstdio>
#include <cstring>
#include <atomic>

#include <core/JSONRPC.h>

#ifdef __POSIX__
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // __POSIX__

namespace Thunder {
namespace Test {

    // -----------------------------------------------------------------------
    // Factory — hands out JSONRPC::Message objects as the deserialization target.
    // JSONRPC::Message extends JSON::Container, so Container::Deserialize is the
    // code path that calls IsNullValue when the first byte is not '{'.
    // -----------------------------------------------------------------------
    class JSONFactory : public ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message> {
    public:
        JSONFactory() = delete;
        JSONFactory(const JSONFactory&) = delete;
        JSONFactory& operator=(const JSONFactory&) = delete;

        explicit JSONFactory(uint32_t poolSize)
            : ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message>(poolSize)
        {
        }

        ~JSONFactory() = default;

        ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement> Element(const string&)
        {
            return ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(
                ::Thunder::Core::ProxyPoolType<::Thunder::Core::JSONRPC::Message>::Element());
        }
    };

    // -----------------------------------------------------------------------
    // Server-side per-connection handler.
    //
    // Accepted by SocketServerType<JSONServer>, so the constructor signature
    // must follow the Thunder convention:
    //   (const SOCKET&, const NodeId&, SocketServerType<JSONServer>*)
    //
    // NOTE: _factory is declared AFTER the StreamJSONType base so it is
    // technically initialised after the base-class ctor runs.  The base stores
    // the reference for later use (not in the ctor body), so this is safe in
    // practice and matches the pattern used by the existing unit tests.
    // -----------------------------------------------------------------------
    class JSONServer
        : public ::Thunder::Core::StreamJSONType<
              ::Thunder::Core::SocketStream,
              JSONFactory&,
              ::Thunder::Core::JSON::IElement> {

        using Base = ::Thunder::Core::StreamJSONType<
            ::Thunder::Core::SocketStream,
            JSONFactory&,
            ::Thunder::Core::JSON::IElement>;

    public:
        JSONServer() = delete;
        JSONServer(const JSONServer&) = delete;
        JSONServer& operator=(const JSONServer&) = delete;

        JSONServer(const SOCKET& connector,
                   const ::Thunder::Core::NodeId& remoteId,
                   ::Thunder::Core::SocketServerType<JSONServer>*)
            : Base(5, s_factory, false, connector, remoteId, 1024, 1024)
        {
            fprintf(stdout, "[Server] Connection accepted from %s\n",
                    remoteId.HostAddress().c_str());
            fflush(stdout);
        }

        ~JSONServer() = default;

    public:
        // Called every time the deserialiser successfully parses one JSON element.
        //
        // After PR #2133, DeserializerImpl only calls Received() on a SUCCESSFUL parse.
        // Garbage bytes that cause a parse error are discarded with an error log;
        // Received() is NOT invoked for them. Only genuine, fully-parsed JSON objects
        // arrive here.
        //
        // NOTE: element->IsSet() is NOT reliable to distinguish valid vs spurious
        //     deliveries even if spurious ones did occur. JSONRPC::Message::Clear()
        //     always reassigns JSONRPC = DefaultVersion ("2.0"), so Container::IsSet()
        //     returns true even for a freshly-cleared element. Use Designator.IsSet()
        //     (which is only true when a "method" or "result" field was parsed) to
        //     classify deliveries in test scenarios where spurious calls may appear.
        void Received(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& element) override
        {
            ++s_receivedCount;

            // Downcast to JSONRPC::Message to inspect individual fields.
            auto* msg = dynamic_cast<::Thunder::Core::JSONRPC::Message*>(element.operator->());
            bool hasMethod = (msg != nullptr && msg->Designator.IsSet());

            if (!hasMethod) {
                // Spurious delivery: element was cleared by the error path.
                // Only the default JSONRPC="2.0" field is present.
                uint32_t n = ++s_spuriousCount;
                string text;
                element->ToString(text);
                fprintf(stdout,
                        "[Server] SPURIOUS Received()  #%u  "
                        "(garbage byte — Designator not set, text: %s)\n", n, text.c_str());
            } else {
                // Genuine delivery: Designator carries the parsed method name.
                string text;
                element->ToString(text);
                fprintf(stdout,
                        "[Server] VALID   Received()   payload: %s\n", text.c_str());
                s_validMessageReceived.store(true);
            }
            fflush(stdout);
        }

        void Send(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>&) override {}

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
        static std::atomic<uint32_t>  s_receivedCount;   // total Received() calls
        static std::atomic<uint32_t>  s_spuriousCount;   // calls with IsSet()==false (garbage bytes)
        static std::atomic<bool>      s_validMessageReceived;
        static ::Thunder::Core::Event s_closedEvent;
        static JSONFactory            s_factory;
    };

    std::atomic<uint32_t>  JSONServer::s_receivedCount{ 0 };
    std::atomic<uint32_t>  JSONServer::s_spuriousCount{ 0 };
    std::atomic<bool>       JSONServer::s_validMessageReceived{ false };
    ::Thunder::Core::Event  JSONServer::s_closedEvent{ false, true };
    JSONFactory             JSONServer::s_factory{ 2 };

} // namespace Test
} // namespace Thunder

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    using namespace Thunder;
    using namespace Thunder::Test;

    // Use a high-numbered port that is unlikely to be in use.
    constexpr uint16_t SERVER_PORT  = 19273;
    // How long to wait for the server to signal the connection close.
    // If the bug is present the wait will expire because ReceiveData() never
    // returns, so the socket worker thread can never process the TCP EOF.
    constexpr uint32_t WAIT_MS      = 3000;
    // If Received() is called this many times we know the loop is spinning.
    constexpr uint32_t SPIN_THRESHOLD = 20;

    // ---- 1. Start the server -----------------------------------------------
    ::Thunder::Core::NodeId serverNode("0.0.0.0", SERVER_PORT);
    ::Thunder::Core::SocketServerType<JSONServer> server(serverNode);

    uint32_t err = server.Open(2000);
    if (err != ::Thunder::Core::ERROR_NONE) {
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
    // Payload "dfsda{...}" has 5 garbage bytes before the valid JSON object.
    // After PR #2133, garbage bytes are discarded with an error log and do NOT
    // trigger Received() — so we expect 0 spurious and exactly 1 valid delivery.
    constexpr uint32_t EXPECTED_SPURIOUS = 0;
    constexpr uint32_t EXPECTED_VALID    = 1;

    fprintf(stdout, "\n--- Result ---\n");
    uint32_t totalReceived = JSONServer::s_receivedCount.load();
    uint32_t spurious      = JSONServer::s_spuriousCount.load();
    uint32_t valid         = totalReceived - spurious;
    bool     validParsed   = JSONServer::s_validMessageReceived.load();

    fprintf(stdout, "Received() total calls  : %u\n",  totalReceived);
    fprintf(stdout, "  Spurious (garbage)    : %u  (expected %u)\n", spurious, EXPECTED_SPURIOUS);
    fprintf(stdout, "  Valid    (parsed JSON): %u  (expected %u)\n", valid,    EXPECTED_VALID);
    fprintf(stdout, "Connection closed       : %s\n",
            waitResult == ::Thunder::Core::ERROR_NONE ? "YES" : "NO (timed out)");

    if (spurious > 0) {
        fprintf(stdout,
                "NOTE: %u spurious Received() call(s) detected.\n"
                "      This would indicate a regression in DeserializerImpl —\n"
                "      Received() should only be called on successful parses.\n",
                spurious);
    }

    bool pass = (waitResult == ::Thunder::Core::ERROR_NONE)
             && validParsed
             && (spurious == EXPECTED_SPURIOUS)
             && (valid    == EXPECTED_VALID);

    if (pass) {
        fprintf(stdout,
                "PASS: connection closed normally; no spurious deliveries, %u valid delivery.\n",
                valid);
    } else if (waitResult != ::Thunder::Core::ERROR_NONE) {
        fprintf(stdout,
                "FAIL: Timed out after %u ms — server never signalled connection-close.\n",
                WAIT_MS);
        if (totalReceived >= SPIN_THRESHOLD) {
            fprintf(stdout,
                    "      Received() has been called %u times — ReceiveData() is\n"
                    "      spinning on the last byte (infinite loop bug still present).\n",
                    totalReceived);
        }
    } else {
        fprintf(stdout,
                "FAIL: unexpected delivery counts (spurious=%u, valid=%u).\n",
                spurious, valid);
    }
    fflush(stdout);

    server.Close(2000);
    ::Thunder::Core::Singleton::Dispose();

    return ((waitResult == ::Thunder::Core::ERROR_NONE) && validParsed) ? 0 : 1;
}
