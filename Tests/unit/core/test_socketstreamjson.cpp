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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

#ifdef __POSIX__
#include "../RawSender.h"
#endif

namespace Thunder {
namespace Tests {
namespace Core {

    enum class CommandTypeSocketStreamJSON {
        EXECUTESHELL,
        WIFISETTINGS,
        FANCONTROL,
        PLAYERCONTROL
    };

    class Parameters : public ::Thunder::Core::JSON::Container {
    public:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

        Parameters()
            : ::Thunder::Core::JSON::Container()
            , Speed(0)
            , Duration(0)
            , Command()
            , Settings()
        {
            Add(_T("speed"), &Speed);
            Add(_T("duration"), &Duration);
            Add(_T("command"), &Command);
            Add(_T("settings"), &Settings);
        }

       ~Parameters()
        {
        }

    public:
        ::Thunder::Core::JSON::OctSInt16 Speed;
        ::Thunder::Core::JSON::DecUInt16 Duration;
        ::Thunder::Core::JSON::EnumType<CommandTypeSocketStreamJSON> Command;
        ::Thunder::Core::JSON::ArrayType<::Thunder::Core::JSON::DecUInt16> Settings;
    };

    class Command : public ::Thunder::Core::JSON::Container {
    public:
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        Command()
            : ::Thunder::Core::JSON::Container()
            , Identifier(0)
            , Name()
            , BaseAddress(0)
            , TrickFlag(false)
            , Params()
        {
            Add(_T("id"), &Identifier);
            Add(_T("name"), &Name);
            Add(_T("baseAddress"), &BaseAddress);
            Add(_T("trickFlag"), &TrickFlag);
            Add(_T("parameters"), &Params);
        }

        ~Command()
        {
        }

    public:
        ::Thunder::Core::JSON::DecUInt32 Identifier;
        ::Thunder::Core::JSON::String Name;
        ::Thunder::Core::JSON::HexUInt32 BaseAddress;
        ::Thunder::Core::JSON::Boolean TrickFlag;
        Parameters Params;
    };

    class JSONObjectFactory : public ::Thunder::Core::ProxyPoolType<Command> {
    public:
        JSONObjectFactory() = delete;
        JSONObjectFactory(const JSONObjectFactory&) = delete;
        JSONObjectFactory& operator= (const JSONObjectFactory&) = delete;

        JSONObjectFactory(const uint32_t number) : ::Thunder::Core::ProxyPoolType<Command>(number)
        {
        }

        virtual ~JSONObjectFactory()
        {
        }

    public:
        ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement> Element(const string&)
        {
            return (::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(::Thunder::Core::ProxyPoolType<Command>::Element()));
        }
    };

    template<typename INTERFACE>
    class JSONConnector : public ::Thunder::Core::StreamJSONType<::Thunder::Core::SocketStream, JSONObjectFactory&, INTERFACE> {
    private:
        typedef ::Thunder::Core::StreamJSONType<::Thunder::Core::SocketStream, JSONObjectFactory&, INTERFACE> BaseClass;

    public:
        JSONConnector() = delete;
        JSONConnector(const JSONConnector& copy) = delete;
        JSONConnector& operator=(const JSONConnector&) = delete;

        JSONConnector(const ::Thunder::Core::NodeId& remoteNode)
            : BaseClass(5, _objectFactory, false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
            , _serverSocket(false)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }

        JSONConnector(const SOCKET& connector, const ::Thunder::Core::NodeId& remoteId, ::Thunder::Core::SocketServerType<JSONConnector<INTERFACE>>*)
            : BaseClass(5, _objectFactory, false, connector, remoteId, 1024, 1024)
            , _serverSocket(true)
            , _dataPending(false, false)
            , _objectFactory(1)
        {
        }

        virtual ~JSONConnector()
        {
        }

    public:
        virtual void Received(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& newElement)
        {
            string textElement;
            newElement->ToString(textElement);

            if (_serverSocket)
                this->Submit(newElement);
            else {
                _dataReceived = textElement;
                _dataPending.Unlock();
            }
        }

        virtual void Send(VARIABLE_IS_NOT_USED ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& newElement)
        {
        }

        virtual void StateChange()
        {
            if (this->IsOpen()) {
                if (_serverSocket) {
                    std::unique_lock<std::mutex> lk(_mutex);
                    _done = true;
                    _cv.notify_one();
                 }
            }
        }

        virtual bool IsIdle() const
        {
            return (true);
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

        static bool GetState()
        {
            return _done;
        }

    private:
        bool _serverSocket;
        string _dataReceived;
        mutable ::Thunder::Core::Event _dataPending;
        JSONObjectFactory _objectFactory;
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    template<typename INTERFACE>
    std::mutex JSONConnector<INTERFACE>::_mutex;
    template<typename INTERFACE>
    std::condition_variable JSONConnector<INTERFACE>::_cv;
    template<typename INTERFACE>
    bool JSONConnector<INTERFACE>::_done = false;

    TEST(Core_Socket, StreamJSON)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string connector = "/tmp/wpestreamjson0";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::SocketServerType<JSONConnector<::Thunder::Core::JSON::IElement>> jsonSocketServer(::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(jsonSocketServer.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            std::unique_lock<std::mutex> lk(JSONConnector<::Thunder::Core::JSON::IElement>::_mutex);

            while (!JSONConnector<::Thunder::Core::JSON::IElement>::GetState()) {
                JSONConnector<::Thunder::Core::JSON::IElement>::_cv.wait(lk);
            }

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(jsonSocketServer.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::ProxyType<Command> sendObject = ::Thunder::Core::ProxyType<Command>::Create();
            ASSERT_TRUE(sendObject.IsValid());

            sendObject->Identifier = 1;
            sendObject->Name = _T("TestCase");
            sendObject->Params.Duration = 100;

            std::string sendString;
            EXPECT_TRUE(sendObject->ToString(sendString));

            JSONConnector<::Thunder::Core::JSON::IElement> jsonSocketClient(::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(jsonSocketClient.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            jsonSocketClient.Submit(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>(sendObject));

            EXPECT_EQ(jsonSocketClient.Wait(), ::Thunder::Core::ERROR_NONE);

            string received;
            jsonSocketClient.Retrieve(received);

            EXPECT_STREQ(sendString.c_str(), received.c_str());

            EXPECT_EQ(jsonSocketClient.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Non-happy-day tests
    //
    // Use TCP loopback + raw POSIX socket instead of IPTestAdministrator.
    // TCP_NODELAY on the sender ensures each send() becomes its own TCP segment,
    // giving the server ResourceMonitor one poll()/read() per call. This lets
    // us control exactly how many bytes arrive per ReceiveData() invocation —
    // which is what we need to reliably trigger the IsNullValue fragmentation
    // bug (issue #1963).
    //
    // Ports 19274-19285 avoid conflict with PR #2129's GarbageTest (19273).

#ifdef __POSIX__

    namespace {

        static constexpr uint32_t WAIT_TIMEOUT_MS = 3000;

        // Server-side connection handler.
        //
        // Uses a static factory to avoid the reference-before-construction UB
        // present in JSONConnector, where _objectFactory is a member initialised
        // after BaseClass in the MIL yet passed by reference to BaseClass's ctor.
        //
        // Received() caps stored messages at 16 to bound memory under spin
        // conditions, but tracks the total call count via an atomic so tests
        // can detect runaway spurious callbacks.
        class StreamJSONTestServer
            : public ::Thunder::Core::StreamJSONType<
                  ::Thunder::Core::SocketStream,
                  JSONObjectFactory&,
                  ::Thunder::Core::JSON::IElement>
        {
            using Base = ::Thunder::Core::StreamJSONType<
                ::Thunder::Core::SocketStream,
                JSONObjectFactory&,
                ::Thunder::Core::JSON::IElement>;

        public:
            StreamJSONTestServer() = delete;
            StreamJSONTestServer(const StreamJSONTestServer&) = delete;
            StreamJSONTestServer& operator=(const StreamJSONTestServer&) = delete;

            StreamJSONTestServer(const SOCKET& connector,
                                 const ::Thunder::Core::NodeId& remoteId,
                                 ::Thunder::Core::SocketServerType<StreamJSONTestServer>*)
                : Base(5, s_factory, false, connector, remoteId, 1024, 1024)
            {
            }

            void Received(::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>& element) override
            {
                ++s_receiveCount;
                string text;
                element->ToString(text);
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    if (s_messages.size() < 16)
                        s_messages.push_back(text);
                }
                s_messageEvent.SetEvent();
            }

            void Send(VARIABLE_IS_NOT_USED ::Thunder::Core::ProxyType<::Thunder::Core::JSON::IElement>&) override {}

            void StateChange() override
            {
                if (IsOpen()) s_connectEvent.SetEvent();
            }

            static void Reset()
            {
                s_receiveCount = 0;
                std::lock_guard<std::mutex> lock(s_mutex);
                s_messages.clear();
                s_messageEvent.ResetEvent();
                s_connectEvent.ResetEvent();
            }

            // Polls s_receiveCount rather than counting event signals, so it is
            // safe when multiple Received() calls fire before Lock() runs.
            static uint32_t WaitForMessages(uint32_t count, uint32_t timeoutMs = WAIT_TIMEOUT_MS)
            {
                auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(timeoutMs);
                while (s_receiveCount.load() < count) {
                    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                        deadline - std::chrono::steady_clock::now()).count();
                    if (remaining <= 0) return ::Thunder::Core::ERROR_TIMEDOUT;
                    s_messageEvent.Lock(
                        static_cast<uint32_t>(std::min<long long>(remaining, 50)));
                }
                return ::Thunder::Core::ERROR_NONE;
            }

            static uint32_t WaitForConnect(uint32_t timeoutMs = 2000)
            {
                return s_connectEvent.Lock(timeoutMs);
            }

            static uint32_t ReceiveCount() { return s_receiveCount.load(); }

            static std::vector<string> Messages()
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                return s_messages;
            }

            static JSONObjectFactory         s_factory;
            static ::Thunder::Core::Event    s_messageEvent;
            static ::Thunder::Core::Event    s_connectEvent;
            static std::mutex                s_mutex;
            static std::vector<string>       s_messages;
            static std::atomic<uint32_t>     s_receiveCount;
        };

        JSONObjectFactory                   StreamJSONTestServer::s_factory{5};
        ::Thunder::Core::Event              StreamJSONTestServer::s_messageEvent{false, true};
        ::Thunder::Core::Event              StreamJSONTestServer::s_connectEvent{false, true};
        std::mutex                          StreamJSONTestServer::s_mutex;
        std::vector<string>                 StreamJSONTestServer::s_messages;
        std::atomic<uint32_t>               StreamJSONTestServer::s_receiveCount{0};

    } // anonymous namespace

    // -------------------------------------------------------------------------
    // Test 2: null field fragmentation — the IsNullValue spin trigger
    //
    // "parameters":null forces the container deserialiser to call IsNullValue().
    // Sending the message byte-by-byte guarantees that 'n' arrives as a 1-byte
    // chunk (maxLength == 1). With the bug (loaded + 1 == maxLength guard),
    // IsNullValue returns UNKNOWN without consuming the byte, Deserialize()
    // returns 0, and the do/while loop in ReceiveData() spins indefinitely.
    //
    // DISABLED: reveals a secondary issue not addressed by PR #2129 or #2133.
    // When 'n' arrives mid-object the container deserialiser consumes it
    // (loaded=1) and holds _current waiting for 'u','l','l'. Those bytes arrive
    // as a fresh 1-byte chunk in the wrong parse context → parse error →
    // _current released → message lost, no Received() call.
    // Root cause: DeserializerImpl needs to preserve _current and _offset when
    // UNKNOWN is returned so multi-byte tokens ("null") can be reassembled
    // across chunk boundaries. Tracked separately.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, DISABLED_StreamJSON_NullFieldFragmented)
    {
        constexpr uint16_t PORT = 19274;

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // parameters is a ::Thunder::Core::JSON::Container subclass; the value null
            // routes through IsNullValue() in the container deserialiser.
            const string json =
                R"({"id":1,"name":"Test","baseAddress":0,"trickFlag":false,"parameters":null})";
            sender.Send(json, 1);
        } // sender closes here

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));

        // Give any spin-generated spurious Received() calls time to accumulate
        // before checking the count. With the bug the spin rate is CPU-bound
        // (thousands per 100 ms); with the fix the count stays at 1.
        SleepMs(100);
        EXPECT_EQ(1u, StreamJSONTestServer::ReceiveCount());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
    }

    // -------------------------------------------------------------------------
    // Test 3: byte-by-byte reassembly of a valid message
    //
    // A complete Command arrives one byte per TCP segment. Exercises the
    // _offset continuation path across many separate ReceiveData() calls and
    // verifies the parser reconstructs the object correctly.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_ByteByByteReassembly)
    {
        constexpr uint16_t PORT = 19275;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier      = 42;
        cmd->Name            = _T("ByteByByteTest");
        cmd->TrickFlag       = true;
        cmd->Params.Duration = 200;
        string sent;
        EXPECT_TRUE(cmd->ToString(sent));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("127.0.0.1", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(sent, 1);
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        ASSERT_EQ(1u, StreamJSONTestServer::ReceiveCount());
        EXPECT_STREQ(sent.c_str(), StreamJSONTestServer::Messages()[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 4: two messages concatenated in a single write
    //
    // Exercises the do/while loop's ability to extract multiple complete
    // messages from one ReceiveData() call. Without the loop both messages
    // would land in the same buffer but only the first would be delivered.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_BackToBackMessages)
    {
        constexpr uint16_t PORT = 19276;

        ::Thunder::Core::ProxyType<Command> cmd1 = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd1.IsValid());
        cmd1->Identifier = 1;
        cmd1->Name       = _T("First");
        string s1; EXPECT_TRUE(cmd1->ToString(s1));

        ::Thunder::Core::ProxyType<Command> cmd2 = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd2.IsValid());
        cmd2->Identifier = 2;
        cmd2->Name       = _T("Second");
        string s2; EXPECT_TRUE(cmd2->ToString(s2));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(s1 + s2);
            SleepMs(100);  // let server process both messages before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(2));
        SleepMs(50);

        auto messages = StreamJSONTestServer::Messages();
        ASSERT_EQ(2u, messages.size());
        EXPECT_STREQ(s1.c_str(), messages[0].c_str());
        EXPECT_STREQ(s2.c_str(), messages[1].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 5: partial message followed by connection close
    //
    // The closing '}' never arrives. Verifies that no Received() callback
    // fires for an incomplete parse and that the server neither crashes
    // nor hangs after the connection drops.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_PartialThenClose)
    {
        constexpr uint16_t PORT = 19277;

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(R"({"id":1,"name":"Partial")"); // no closing '}'
        } // sender closes here — server sees TCP FIN mid-parse

        EXPECT_EQ(::Thunder::Core::ERROR_TIMEDOUT, StreamJSONTestServer::WaitForMessages(1, 500));
        EXPECT_EQ(0u, StreamJSONTestServer::ReceiveCount());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 6: corrupt data spam
    //
    // Floods the parser with bytes that are invalid JSON. Each byte starts a
    // parse attempt that immediately fails (or returns UNKNOWN for 'n'/'t'/'f'),
    // exercising the error recovery path on every iteration.
    //
    // Bug  → first byte that returns loaded == 0 causes infinite spin
    // DISABLED: when 'n' arrives as the first byte of a fresh parse context
    // (after a prior error reset _current), IsNullValue returns UNKNOWN with
    // loaded=0. The else-branch satisfies (loaded != length) and calls
    // Received() with an empty object — one spurious call per 'n' byte in the
    // input. The garbage array contains 7 'n' bytes → 7 spurious deliveries.
    // Root cause: same as NullFieldFragmented — UNKNOWN must not trigger
    // Received(). Tracked as issue #<nr>.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, DISABLED_StreamJSON_GarbageSpam)
    {
        constexpr uint16_t PORT = 19278;

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // Mix of bytes that can never form valid JSON: control chars, mid-value
            // starts ('n','t','f' without completing "null"/"true"/"false"), and
            // random high bytes. Sent byte-by-byte so each arrives as its own
            // 1-byte ReceiveData() call — the worst case for the spin bug.
            // Uses uint8_t array + explicit length because the sequence contains
            // \x00 which would truncate a c_str()-based send.
            static const uint8_t garbage[] = {
                0x01, 0x02, 0x03, 'n',  't',  'n',  'f',  'n',
                0x80, 0x81, 'x',  'y',  'z',  'n',  0xff, 0xfe,
                '!',  '!',  '@',  '@',  '#',  '#',  0x00, 0x01,
                'n',  'n',  'n'
            };
            sender.Send(garbage, sizeof(garbage), 1);
        }

        EXPECT_EQ(::Thunder::Core::ERROR_TIMEDOUT, StreamJSONTestServer::WaitForMessages(1, 500));
        EXPECT_EQ(0u, StreamJSONTestServer::ReceiveCount());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 7: garbage followed by a valid message — recovery
    //
    // After corrupt bytes the parser must reset cleanly so that the next well-
    // formed message is still delivered. This is the key recovery invariant:
    // a bad sender should not permanently wedge the receive path for messages
    // that arrive later on the same connection.
    //
    // DISABLED: same spurious Received() issue as GarbageSpam. The poison
    // sequence contains 2 'n' bytes → 2 spurious deliveries before the valid
    // message, so messages.size() == 3 instead of 1. The valid message IS
    // received last, but the extra spurious ones break the count assertion.
    // Tracked as issue #<nr>.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, DISABLED_StreamJSON_GarbageFollowedByValid)
    {
        constexpr uint16_t PORT = 19279;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier      = 7;
        cmd->Name            = _T("AfterGarbage");
        cmd->Params.Duration = 50;
        string valid;
        EXPECT_TRUE(cmd->ToString(valid));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // Poison the stream first, then follow with a valid message.
            // The delay between the two gives the server time to finish
            // processing the garbage before the valid bytes arrive.
            static const uint8_t poison[] = { 0x01, 0x02, 'n', 't', 'n', 0x80, 'x', 'y', 'z' };
            sender.Send(poison, sizeof(poison), 1);
            SleepMs(50);
            sender.Send(valid);
            SleepMs(100);  // let server process before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        auto messages = StreamJSONTestServer::Messages();
        ASSERT_EQ(1u, messages.size());
        EXPECT_STREQ(valid.c_str(), messages[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 8: reconnect after partial message
    //
    // First connection sends an incomplete JSON object and drops. A second
    // connection then sends a complete message. Verifies that the parser state
    // from the first connection (held in the DeserializerImpl of that server
    // instance) does not bleed into the new connection's instance, and that
    // the server accepts and correctly processes the second connection.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_ReconnectAfterPartial)
    {
        constexpr uint16_t PORT = 19280;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier      = 99;
        cmd->Name            = _T("AfterReconnect");
        cmd->Params.Duration = 10;
        string valid;
        EXPECT_TRUE(cmd->ToString(valid));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // Partial message — no closing '}'
            sender.Send(R"({"id":99,"name":"AfterReconnect")");
        } // first connection drops here

        // Brief pause so the server processes the FIN before we reconnect
        SleepMs(50);

        StreamJSONTestServer::Reset();

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(valid);
            SleepMs(100);  // let server process before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        ASSERT_EQ(1u, StreamJSONTestServer::ReceiveCount());
        EXPECT_STREQ(valid.c_str(), StreamJSONTestServer::Messages()[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 9: reconnect after null fragmentation mid-stream
    //
    // First connection sends everything up to and including the 'n' of a null
    // value — exactly the byte that triggers IsNullValue UNKNOWN — then drops.
    // A second connection then sends a complete valid message. Verifies that
    // the UNKNOWN state from the aborted parse does not persist across
    // connections and that the second message is delivered correctly.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_ReconnectAfterNullFragment)
    {
        constexpr uint16_t PORT = 19281;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier = 55;
        cmd->Name       = _T("PostNull");
        string valid;
        EXPECT_TRUE(cmd->ToString(valid));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // Send up to and including 'n' of null — the UNKNOWN trigger byte.
            // Byte-by-byte so 'n' arrives as its own 1-byte chunk.
            sender.Send(
                R"({"id":55,"name":"PostNull","baseAddress":0,"trickFlag":false,"parameters":n)",
                1);
        } // connection drops with parser in UNKNOWN state

        SleepMs(50);

        StreamJSONTestServer::Reset();

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(valid);
            SleepMs(100);  // let server process before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        ASSERT_EQ(1u, StreamJSONTestServer::ReceiveCount());
        EXPECT_STREQ(valid.c_str(), StreamJSONTestServer::Messages()[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 10: partial second message survives a chunk boundary
    //
    // First chunk: complete message 1 + first half of message 2.
    // Second chunk: second half of message 2.
    //
    // After delivering message 1, DeserializerImpl must preserve _current and
    // _offset for the partial message 2 so the next ReceiveData() call can
    // continue the parse from where it stopped. This exercises the normal
    // multi-call continuation path for split messages.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_ChunkSpanningTwoMessages)
    {
        constexpr uint16_t PORT = 19282;

        ::Thunder::Core::ProxyType<Command> cmd1 = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd1.IsValid());
        cmd1->Identifier = 10;
        cmd1->Name       = _T("First");
        string s1; EXPECT_TRUE(cmd1->ToString(s1));

        ::Thunder::Core::ProxyType<Command> cmd2 = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd2.IsValid());
        cmd2->Identifier      = 20;
        cmd2->Name            = _T("Second");
        cmd2->Params.Duration = 42;
        string s2; EXPECT_TRUE(cmd2->ToString(s2));

        // Split s2 in half so the chunk boundary falls mid-message
        const size_t splitAt = s2.size() / 2;

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(s1 + s2.substr(0, splitAt));  // msg1 + first half of msg2
            SleepMs(50);
            sender.Send(s2.substr(splitAt));           // second half of msg2
            SleepMs(100);  // let server process before connection closes
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(2));
        SleepMs(50);

        auto messages = StreamJSONTestServer::Messages();
        ASSERT_EQ(2u, messages.size());
        EXPECT_STREQ(s1.c_str(), messages[0].c_str());
        EXPECT_STREQ(s2.c_str(), messages[1].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 11: message larger than the receive buffer (> 1024 bytes)
    //
    // SocketPort reads at most receiveBufferSize (1024) bytes per poll cycle.
    // A message larger than that arrives in at least two ReceiveData() calls.
    // _current must be preserved between calls and _offset must accumulate
    // correctly so the parser can reconstruct the complete object.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_MessageLargerThanReceiveBuffer)
    {
        constexpr uint16_t PORT = 19283;

        // Build a name long enough to push the serialized size above 1024 bytes.
        // Base object overhead is ~80 bytes; 1000-char name puts total at ~1080.
        const string longName(1000, 'A');

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier      = 99;
        cmd->Name            = longName.c_str();
        cmd->Params.Duration = 7;
        string sent; EXPECT_TRUE(cmd->ToString(sent));
        ASSERT_GT(sent.size(), 1024u);  // confirm we actually exceed the buffer

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(sent);  // single write; SocketPort splits across multiple reads
            SleepMs(100);  // let server drain all chunks before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        ASSERT_EQ(1u, StreamJSONTestServer::ReceiveCount());
        EXPECT_STREQ(sent.c_str(), StreamJSONTestServer::Messages()[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 12: 200+ consecutive errors followed by a valid message
    //
    // _consecutiveErrors suppresses logging after 200 errors to avoid log
    // flooding. Verify that: the system never crashes, the 200-error cap is
    // survivable, and _consecutiveErrors resets correctly on the first
    // successful parse so a valid message is still delivered.
    //
    // Garbage is sent byte-by-byte (one chunk per byte) so each byte is a
    // separate ReceiveData() call. This avoids the loop-break data-loss issue
    // (see DISABLED_StreamJSON_GarbagePrefixedValidInSingleChunk) and exercises
    // the error counter in isolation.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, StreamJSON_ConsecutiveErrorsSuppression)
    {
        constexpr uint16_t PORT = 19284;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier = 1;
        cmd->Name       = _T("AfterManyErrors");
        string valid; EXPECT_TRUE(cmd->ToString(valid));

        // 210 individual invalid bytes — exceeds the maxConsecutiveErrors cap of 200
        std::vector<uint8_t> spam(210, 0x02);

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("0.0.0.0", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            sender.Send(spam.data(), static_cast<uint32_t>(spam.size()), 1);  // byte-by-byte
            SleepMs(50);
            sender.Send(valid);
            SleepMs(100);  // let server process valid message before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        // Valid message must arrive despite the preceding 210 errors
        auto messages = StreamJSONTestServer::Messages();
        ASSERT_EQ(1u, messages.size());
        EXPECT_STREQ(valid.c_str(), messages[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Test 13: garbage byte immediately before valid JSON in one chunk
    //
    // DISABLED: exposes a data-loss bug in ReceiveData().
    //
    // When DeserializerImpl::Deserialize() returns processed=0 (parse error
    // at position 0), the do/while loop breaks immediately:
    //
    //   do {
    //       processed = _deserializer.Deserialize(&dataFrame[handled], ...);
    //       handled += processed;   // 0 added
    //   } while ((processed != 0) && ...);  // breaks
    //
    // ReceiveData() returns handled=0. SocketPort discards the entire receive
    // buffer. Any valid bytes that followed the error byte in the same chunk
    // are silently dropped — they will never be presented to the parser again.
    //
    // Affected scenario: sender writes "\x02{valid json}" in a single send().
    // Expected: valid message delivered (only \x02 should be skipped).
    // Actual:   nothing delivered (entire buffer discarded).
    //
    // Fix: when processed=0 due to a parse error (not UNKNOWN), advance
    // handled by 1 to skip the offending byte before continuing the loop,
    // rather than returning immediately with the unprocessed buffer.
    // -------------------------------------------------------------------------
    TEST(Core_Socket, DISABLED_StreamJSON_GarbagePrefixedValidInSingleChunk)
    {
        constexpr uint16_t PORT = 19285;

        ::Thunder::Core::ProxyType<Command> cmd = ::Thunder::Core::ProxyType<Command>::Create();
        ASSERT_TRUE(cmd.IsValid());
        cmd->Identifier = 5;
        cmd->Name       = _T("AfterGarbageByte");
        string valid; EXPECT_TRUE(cmd->ToString(valid));

        StreamJSONTestServer::Reset();
        ::Thunder::Core::SocketServerType<StreamJSONTestServer> server(::Thunder::Core::NodeId("127.0.0.1", PORT));
        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Open(2000));

        {
            RawSender sender;
            ASSERT_TRUE(sender.Open("127.0.0.1", PORT));
            ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForConnect());

            // Single write: one invalid byte immediately followed by valid JSON.
            // The invalid byte causes a parse error with loaded=0. The loop
            // breaks and the valid JSON is dropped with it.
            static const uint8_t prefix[] = { 0x02 };
            sender.Send(prefix, sizeof(prefix));
            SleepMs(1);
            sender.Send(valid);
            SleepMs(100);  // let server process before FIN arrives
        }

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, StreamJSONTestServer::WaitForMessages(1));
        SleepMs(50);

        ASSERT_EQ(1u, StreamJSONTestServer::ReceiveCount());
        EXPECT_STREQ(valid.c_str(), StreamJSONTestServer::Messages()[0].c_str());

        ASSERT_EQ(::Thunder::Core::ERROR_NONE, server.Close(2000));
        ::Thunder::Core::Singleton::Dispose();
    }

#endif // __POSIX__

} // Core
} // Tests

ENUM_CONVERSION_BEGIN(Tests::Core::CommandTypeSocketStreamJSON)
    { Tests::Core::CommandTypeSocketStreamJSON::EXECUTESHELL, _TXT("ExecuteShell") },
    { Tests::Core::CommandTypeSocketStreamJSON::WIFISETTINGS, _TXT("WiFiSettings") },
    { Tests::Core::CommandTypeSocketStreamJSON::FANCONTROL, _TXT("FanControl") },
    { Tests::Core::CommandTypeSocketStreamJSON::PLAYERCONTROL, _TXT("PlayerControl") },
ENUM_CONVERSION_END(Tests::Core::CommandTypeSocketStreamJSON)

} // Thunder
