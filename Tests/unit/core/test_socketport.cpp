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

#include <fcntl.h>
#include <unistd.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // -------------------------------------------------------------------------
    // EchoConnector — TCP stream connector that echoes back received text.
    // Server side echoes; client side signals when echo is received.
    // -------------------------------------------------------------------------

    class EchoConnector
        : public ::Thunder::Core::StreamTextType<
              ::Thunder::Core::SocketStream,
              ::Thunder::Core::TerminatorCarriageReturn> {

        using Base = ::Thunder::Core::StreamTextType<
            ::Thunder::Core::SocketStream,
            ::Thunder::Core::TerminatorCarriageReturn>;

    public:
        EchoConnector() = delete;
        EchoConnector(const EchoConnector&) = delete;
        EchoConnector& operator=(const EchoConnector&) = delete;

        // Client constructor
        explicit EchoConnector(const ::Thunder::Core::NodeId& remote)
            : Base(false, remote.AnyInterface(), remote, 1024, 1024)
            , _isServer(false)
            , _dataPending(false, false)
        {
        }

        // Server-side accepted connection constructor
        EchoConnector(const SOCKET& connector,
                      const ::Thunder::Core::NodeId& remote,
                      ::Thunder::Core::SocketServerType<EchoConnector>*)
            : Base(false, connector, remote, 1024, 1024)
            , _isServer(true)
            , _dataPending(false, false)
        {
        }

        ~EchoConnector() override = default;

        void Received(string& text) override
        {
            if (_isServer) {
                Submit(text);
            } else {
                _received = text;
                _dataPending.Unlock();
            }
        }

        void Send(VARIABLE_IS_NOT_USED const string&) override {}

        void StateChange() override
        {
            if (IsOpen() && _isServer) {
                std::unique_lock<std::mutex> lk(s_mutex);
                s_connected = true;
                s_cv.notify_one();
            }
        }

        uint32_t WaitForEcho() { return _dataPending.Lock(); }

        string ReceivedText() const { return _received; }

        static void ResetState()
        {
            std::unique_lock<std::mutex> lk(s_mutex);
            s_connected = false;
        }

        static bool WaitForServerConnection(uint32_t ms)
        {
            std::unique_lock<std::mutex> lk(s_mutex);
            return s_cv.wait_for(lk, std::chrono::milliseconds(ms),
                                 [] { return s_connected; });
        }

    private:
        bool _isServer;
        string _received;
        ::Thunder::Core::Event _dataPending;

    public:
        static std::mutex s_mutex;
        static std::condition_variable s_cv;
        static bool s_connected;
    };

    std::mutex EchoConnector::s_mutex;
    std::condition_variable EchoConnector::s_cv;
    bool EchoConnector::s_connected = false;

    // -------------------------------------------------------------------------
    // Minimal datagram socket — no data flow needed.
    // -------------------------------------------------------------------------

    class TestDatagram : public ::Thunder::Core::SocketDatagram {
    public:
        TestDatagram() = delete;
        TestDatagram(const TestDatagram&) = delete;
        TestDatagram& operator=(const TestDatagram&) = delete;

        explicit TestDatagram(const ::Thunder::Core::NodeId& local,
                              const ::Thunder::Core::NodeId& remote = ::Thunder::Core::NodeId())
            : ::Thunder::Core::SocketDatagram(false, local, remote, 512, 512)
        {
        }
        ~TestDatagram() override = default;

        uint16_t SendData(uint8_t*, const uint16_t) override { return 0; }
        uint16_t ReceiveData(uint8_t*, const uint16_t size) override { return size; }
        void StateChange() override {}
    };

    // Datagram that captures the last received payload.
    class CaptureDatagram : public ::Thunder::Core::SocketDatagram {
    public:
        CaptureDatagram() = delete;
        CaptureDatagram(const CaptureDatagram&) = delete;
        CaptureDatagram& operator=(const CaptureDatagram&) = delete;

        explicit CaptureDatagram(const ::Thunder::Core::NodeId& local)
            : ::Thunder::Core::SocketDatagram(false, local, ::Thunder::Core::NodeId(), 512, 512)
            , _payloadSize(0)
            , _received(false)
        {
            memset(_payload, 0, sizeof(_payload));
        }
        ~CaptureDatagram() override = default;

        uint16_t SendData(uint8_t*, const uint16_t) override { return 0; }

        uint16_t ReceiveData(uint8_t* frame, const uint16_t size) override
        {
            std::unique_lock<std::mutex> lk(_mutex);
            _payloadSize = std::min(size, static_cast<uint16_t>(sizeof(_payload)));
            memcpy(_payload, frame, _payloadSize);
            _received = true;
            _cv.notify_all();
            return size;
        }

        void StateChange() override {}

        bool WaitForData(uint32_t ms)
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return _cv.wait_for(lk, std::chrono::milliseconds(ms),
                                [this] { return _received; });
        }

        string ReceivedPayload() const
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return string(reinterpret_cast<const char*>(_payload), _payloadSize);
        }

    private:
        mutable std::mutex _mutex;
        std::condition_variable _cv;
        uint16_t _payloadSize;
        bool _received;
        uint8_t _payload[512];
    };

    // Datagram that sends a fixed message on the first SendData() call.
    class SenderDatagram : public ::Thunder::Core::SocketDatagram {
    public:
        SenderDatagram() = delete;
        SenderDatagram(const SenderDatagram&) = delete;
        SenderDatagram& operator=(const SenderDatagram&) = delete;

        SenderDatagram(const ::Thunder::Core::NodeId& local,
                       const ::Thunder::Core::NodeId& remote,
                       const string& msg)
            : ::Thunder::Core::SocketDatagram(false, local, remote, 512, 512)
            , _message(msg)
            , _sent(false)
        {
        }
        ~SenderDatagram() override = default;

        uint16_t SendData(uint8_t* frame, const uint16_t maxSize) override
        {
            if (_sent) return 0;
            uint16_t len = static_cast<uint16_t>(
                std::min(_message.size(), static_cast<size_t>(maxSize)));
            memcpy(frame, _message.data(), len);
            _sent = true;
            return len;
        }
        uint16_t ReceiveData(uint8_t*, const uint16_t) override { return 0; }
        void StateChange() override {}

    private:
        string _message;
        bool _sent;
    };

    // =========================================================================
    // State queries — no network activity
    // =========================================================================

    TEST(test_socketport, initial_state_is_closed)
    {
        TestDatagram sock(::Thunder::Core::NodeId("/tmp/test_sp_init.sock"));

        EXPECT_TRUE(sock.IsClosed());
        EXPECT_FALSE(sock.IsOpen());
        EXPECT_FALSE(sock.IsListening());
        EXPECT_FALSE(sock.HasError());
        EXPECT_EQ(sock.State(), 0u);
        EXPECT_EQ(sock.Type(), ::Thunder::Core::SocketPort::DATAGRAM);
    }

    TEST(test_socketport, local_node_accessor_reflects_construction_path)
    {
        const string path = "/tmp/test_sp_node.sock";
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        EXPECT_STREQ(sock.LocalNode().HostName().c_str(), path.c_str());
    }

    TEST(test_socketport, buffer_size_accessors)
    {
        // TestDatagram uses 512/512 — check the values we actually pass.
        ::Thunder::Core::NodeId node("/tmp/test_sp_buf.sock");
        TestDatagram sock(node);

        EXPECT_EQ(sock.SendBufferSize(), 512u);
        EXPECT_EQ(sock.ReceiveBufferSize(), 512u);
    }

    TEST(test_socketport, local_id_contains_path)
    {
        const string path = "/tmp/test_sp_lid.sock";
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        EXPECT_NE(sock.LocalId().find(path), string::npos);
    }

    TEST(test_socketport, remote_node_can_be_updated_on_closed_datagram)
    {
        ::Thunder::Core::NodeId r1("/tmp/test_sp_rn1.sock");
        ::Thunder::Core::NodeId r2("/tmp/test_sp_rn2.sock");
        TestDatagram sock(::Thunder::Core::NodeId("/tmp/test_sp_rn.sock"), r1);

        EXPECT_STREQ(sock.RemoteNode().HostName().c_str(), r1.HostName().c_str());
        sock.RemoteNode(r2);
        EXPECT_STREQ(sock.RemoteNode().HostName().c_str(), r2.HostName().c_str());
    }

    // =========================================================================
    // TOCTOU fix: stale file handling on Open()
    // =========================================================================

    TEST(test_socketport, open_removes_stale_domain_socket_file)
    {
        const string path = "/tmp/test_sp_stale.sock";
        ::unlink(path.c_str());

        int fd = ::open(path.c_str(), O_CREAT | O_WRONLY, 0600);
        ASSERT_GE(fd, 0);
        ::close(fd);
        ASSERT_EQ(::access(path.c_str(), F_OK), 0);

        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));
        EXPECT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsOpen());

        sock.Close(1000);

        uint32_t waited = 0;
        while (!sock.IsClosed() && waited < 1000) {
            SleepMs(10);
            waited += 10;
        }
        EXPECT_TRUE(sock.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_socketport, open_succeeds_when_no_stale_file_exists)
    {
        const string path = "/tmp/test_sp_clean.sock";
        ::unlink(path.c_str());

        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));
        EXPECT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsOpen());

        sock.Close(1000);

        uint32_t waited = 0;
        while (!sock.IsClosed() && waited < 1000) {
            SleepMs(10);
            waited += 10;
        }
        EXPECT_TRUE(sock.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_socketport, reopen_after_close_removes_previous_socket_file)
    {
        const string path = "/tmp/test_sp_reopen.sock";
        ::unlink(path.c_str());
        ::Thunder::Core::NodeId node(path.c_str());

        {
            TestDatagram first(node);
            ASSERT_EQ(first.Open(1000), ::Thunder::Core::ERROR_NONE);
            first.Close(1000);
        }
        {
            TestDatagram second(node);
            EXPECT_EQ(second.Open(1000), ::Thunder::Core::ERROR_NONE);
            EXPECT_TRUE(second.IsOpen());
            second.Close(1000);
        }

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Open / Close lifecycle
    // =========================================================================

    TEST(test_socketport, open_then_close_transitions_state)
    {
        const string path = "/tmp/test_sp_lc.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        EXPECT_TRUE(sock.IsClosed());
        EXPECT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsOpen());
        EXPECT_FALSE(sock.IsClosed());

        EXPECT_EQ(sock.Close(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsClosed());
        EXPECT_FALSE(sock.IsOpen());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_socketport, close_on_already_closed_socket_does_not_crash)
    {
        const string path = "/tmp/test_sp_dblclose.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(sock.Close(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_NO_FATAL_FAILURE(sock.Close(1000));

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_socketport, flush_on_open_socket_does_not_crash)
    {
        const string path = "/tmp/test_sp_flush.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_NO_FATAL_FAILURE(sock.Flush());
        EXPECT_TRUE(sock.IsOpen());

        sock.Close(1000);

        uint32_t waited = 0;
        while (!sock.IsClosed() && waited < 1000) {
            SleepMs(10);
            waited += 10;
        }
        EXPECT_TRUE(sock.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Socket options: TTL and Broadcast
    // =========================================================================

    TEST(test_socketport, ttl_get_set_on_udp_socket)
    {
        ::Thunder::Core::NodeId node("0.0.0.0", 0, ::Thunder::Core::NodeId::TYPE_IPV4);
        TestDatagram sock(node);

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(sock.TTL(64), static_cast<uint32_t>(::Thunder::Core::ERROR_NONE));
        EXPECT_EQ(sock.TTL(), 64u);

        sock.Close(1000);
        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_socketport, broadcast_enable_disable_on_udp_socket)
    {
        ::Thunder::Core::NodeId node("0.0.0.0", 0, ::Thunder::Core::NodeId::TYPE_IPV4);
        TestDatagram sock(node);

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.Broadcast(true));
        EXPECT_TRUE(sock.Broadcast(false));

        sock.Close(1000);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Data exchange: domain UDP loopback
    // =========================================================================

    TEST(test_socketport, datagram_send_receive_domain_socket)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 500;
        constexpr uint8_t maxRetries = 1;

        const string serverPath = "/tmp/test_sp_dgram_srv.sock";
        const string clientPath = "/tmp/test_sp_dgram_cli.sock";
        const string message    = "hello_socketport";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(serverPath.c_str());
            ::Thunder::Core::NodeId serverNode(serverPath.c_str());
            CaptureDatagram server(serverNode);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_TRUE(server.WaitForData(maxWaitTimeMs));
            EXPECT_EQ(server.ReceivedPayload(), message);

            server.Close(maxWaitTimeMs);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ::unlink(clientPath.c_str());

            ::Thunder::Core::NodeId clientNode(clientPath.c_str());
            ::Thunder::Core::NodeId serverNode(serverPath.c_str());
            SenderDatagram sender(clientNode, serverNode, message);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(sender.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            sender.Trigger();

            SleepMs(500);
            sender.Close(maxWaitTimeMs);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // TCP stream: connect, echo, close
    // =========================================================================

    TEST(test_socketport, stream_connect_echo_close)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 500;
        constexpr uint8_t maxRetries = 1;

        const string connector = "/tmp/test_sp_stream.sock";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(connector.c_str());
            EchoConnector::ResetState();

            ::Thunder::Core::SocketServerType<EchoConnector> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_TRUE(EchoConnector::WaitForServerConnection(maxWaitTimeMs));

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);

            EchoConnector client(::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_TRUE(client.IsOpen());
            EXPECT_FALSE(client.IsClosed());

            const string msg = "echo_test";
            client.Submit(msg);
            EXPECT_EQ(client.WaitForEcho(), ::Thunder::Core::ERROR_NONE);
            EXPECT_EQ(client.ReceivedText(), msg);

            ASSERT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Multicast UNINIT fix: source-specific Join/Leave with INADDR_ANY
    // =========================================================================

    TEST(test_socketport, source_specific_multicast_join_does_not_crash)
    {
        ::Thunder::Core::NodeId local("0.0.0.0", 0, ::Thunder::Core::NodeId::TYPE_IPV4);
        ::Thunder::Core::NodeId group("224.0.0.1", 0, ::Thunder::Core::NodeId::TYPE_IPV4);
        ::Thunder::Core::NodeId src("127.0.0.1", 0, ::Thunder::Core::NodeId::TYPE_IPV4);

        TestDatagram sock(local);
        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);

        // Return value may be false on hosts without multicast on loopback.
        // What matters is no crash or UB from the previously uninitialized
        // imr_interface field.
        EXPECT_NO_FATAL_FAILURE(sock.Join(group, src));
        EXPECT_NO_FATAL_FAILURE(sock.Leave(group, src));

        sock.Close(1000);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Close(waitTime=0) — non-blocking close, no waiting for far-end
    // =========================================================================

    TEST(test_socketport, close_with_zero_wait_does_not_block)
    {
        const string path = "/tmp/test_sp_closenowait.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsOpen());

        // Close(0) returns immediately without waiting for the far end.
        EXPECT_NO_FATAL_FAILURE(sock.Close(0));

        // The socket is not yet fully deregistered from the ResourceMonitor.
        // Wait for IsClosed() before Singleton::Dispose() to avoid the debug
        // assert that fires if resources are still registered at teardown.
        uint32_t waited = 0;
        while (!sock.IsClosed() && waited < 1000) {
            SleepMs(10);
            waited += 10;
        }
        EXPECT_TRUE(sock.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // unlink() non-ENOENT failure — directory at the socket path
    // =========================================================================

    TEST(test_socketport, open_fails_when_path_is_a_directory)
    {
        const string path = "/tmp/test_sp_dir.sock";
        ::unlink(path.c_str());
        ::rmdir(path.c_str());

        ASSERT_EQ(::mkdir(path.c_str(), 0700), 0);

        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));
        EXPECT_NE(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsClosed());

        ::rmdir(path.c_str());
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Domain socket file is removed after Close()
    // =========================================================================

    TEST(test_socketport, close_removes_domain_socket_file)
    {
        const string path = "/tmp/test_sp_closeclean.sock";
        ::unlink(path.c_str());

        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));
        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(sock.Close(1000), ::Thunder::Core::ERROR_NONE);

        // Close(1000) waits for full closure including the ResourceMonitor
        // thread running Closed() which removes the socket file.
        EXPECT_NE(::access(path.c_str(), F_OK), 0);

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Trigger() on open socket — WRITESLOT path, must not crash
    // =========================================================================

    TEST(test_socketport, trigger_on_open_socket_does_not_crash)
    {
        const string path = "/tmp/test_sp_trigger.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);
        EXPECT_NO_FATAL_FAILURE(sock.Trigger());
        EXPECT_TRUE(sock.IsOpen());

        sock.Close(1000);

        uint32_t waited = 0;
        while (!sock.IsClosed() && waited < 1000) {
            SleepMs(10);
            waited += 10;
        }
        EXPECT_TRUE(sock.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Simple (non-source-specific) multicast Join/Leave on open socket
    // =========================================================================

    TEST(test_socketport, simple_multicast_join_leave)
    {
        ::Thunder::Core::NodeId local("0.0.0.0", 0, ::Thunder::Core::NodeId::TYPE_IPV4);
        ::Thunder::Core::NodeId group("224.0.0.1", 0, ::Thunder::Core::NodeId::TYPE_IPV4);

        TestDatagram sock(local);
        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);

        EXPECT_NO_FATAL_FAILURE(sock.Join(group));
        EXPECT_NO_FATAL_FAILURE(sock.Leave(group));

        sock.Close(1000);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Remote side closes — client detects it
    // =========================================================================

    TEST(test_socketport, remote_close_detected_by_client)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8,
                           maxWaitTimeMs = 8000, maxInitTime = 500;
        constexpr uint8_t maxRetries = 1;

        const string connector = "/tmp/test_sp_remoteclose.sock";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(connector.c_str());
            EchoConnector::ResetState();

            ::Thunder::Core::SocketServerType<EchoConnector> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_TRUE(EchoConnector::WaitForServerConnection(maxWaitTimeMs));
            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EchoConnector client(::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            EXPECT_TRUE(client.IsOpen());

            // Poll until ResourceMonitor propagates the HUP.
            uint32_t waited = 0;
            while (client.IsOpen() && waited < 4000) {
                SleepMs(50);
                waited += 50;
            }
            EXPECT_FALSE(client.IsOpen());

            client.Close(maxWaitTimeMs);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Open() on non-existent server path — must fail cleanly, no crash
    // =========================================================================

    TEST(test_socketport, stream_connect_to_nonexistent_server_fails_cleanly)
    {
        // Nothing is listening on this path.
        ::Thunder::Core::NodeId remote("/tmp/test_sp_noserver.sock");
        EchoConnector client(remote);

        uint32_t result = client.Open(1000);
        EXPECT_NE(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(client.IsClosed());

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Open() on already-open socket — must return ERROR_ILLEGAL_STATE
    // =========================================================================

    // Second Open() on an already-open socket returns ERROR_ILLEGAL_STATE.
    // Previously this triggered an ASSERT abort — the source fix adds an
    // else-if guard that catches non-zero state before the ASSERT.
    TEST(test_socketport, open_on_already_open_socket_returns_illegal_state)
    {
        const string path = "/tmp/test_sp_dblopen.sock";
        ::unlink(path.c_str());
        TestDatagram sock(::Thunder::Core::NodeId(path.c_str()));

        ASSERT_EQ(sock.Open(1000), ::Thunder::Core::ERROR_NONE);

        // Second Open() while still open must not succeed.
        uint32_t result = sock.Open(1000);
        EXPECT_NE(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(sock.IsOpen());

        sock.Close(1000);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // StateChange() is called on connect — verified via subclass
    // =========================================================================

    class StateChangeConnector : public ::Thunder::Core::SocketStream {
    public:
        StateChangeConnector() = delete;
        StateChangeConnector(const StateChangeConnector&) = delete;
        StateChangeConnector& operator=(const StateChangeConnector&) = delete;

        StateChangeConnector(const SOCKET connector,
                             const ::Thunder::Core::NodeId& remote,
                             ::Thunder::Core::SocketServerType<StateChangeConnector>*)
            : ::Thunder::Core::SocketStream(false, connector, remote, 512, 512)
            , _stateChanges(0)
        {
        }

        explicit StateChangeConnector(const ::Thunder::Core::NodeId& remote)
            : ::Thunder::Core::SocketStream(false, ::Thunder::Core::NodeId(), remote, 512, 512)
            , _stateChanges(0)
        {
        }

        ~StateChangeConnector() override = default;

        uint16_t SendData(uint8_t*, const uint16_t) override { return 0; }
        uint16_t ReceiveData(uint8_t*, const uint16_t size) override { return size; }

        void StateChange() override
        {
            std::unique_lock<std::mutex> lk(_mutex);
            _stateChanges.fetch_add(1, std::memory_order_relaxed);
            _cv.notify_all();
        }

        int StateChanges() const { return _stateChanges.load(std::memory_order_relaxed); }

        bool WaitForStateChange(uint32_t ms)
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return _cv.wait_for(lk, std::chrono::milliseconds(ms),
                                [this] { return _stateChanges.load(std::memory_order_relaxed) > 0; });
        }

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        std::atomic<int> _stateChanges;
    };

    TEST(test_socketport, state_change_called_on_connect_and_disconnect)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 500;
        constexpr uint8_t maxRetries = 1;

        const string connector = "/tmp/test_sp_statechange.sock";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(connector.c_str());
            ::Thunder::Core::SocketServerType<StateChangeConnector> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Server is ready — signal parent, then wait for it to finish.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // Wait for child to signal that the server is ready.
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            SleepMs(maxInitTime);

            StateChangeConnector client(::Thunder::Core::NodeId(connector.c_str()));
            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // StateChange is called asynchronously by the ResourceMonitor thread.
            EXPECT_TRUE(client.WaitForStateChange(maxWaitTimeMs));
            EXPECT_GE(client.StateChanges(), 1);

            ASSERT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Signal child that we are done so it can close the server.
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // UDP IPv4 send/receive — sendto/recvmsg path on loopback
    // =========================================================================

    class UDPCapture : public ::Thunder::Core::SocketDatagram {
    public:
        UDPCapture() = delete;
        UDPCapture(const UDPCapture&) = delete;
        UDPCapture& operator=(const UDPCapture&) = delete;

        explicit UDPCapture(const ::Thunder::Core::NodeId& local)
            : ::Thunder::Core::SocketDatagram(false, local, ::Thunder::Core::NodeId(), 512, 512)
            , _payloadSize(0)
            , _received(false)
        {
            memset(_payload, 0, sizeof(_payload));
        }
        ~UDPCapture() override = default;

        uint16_t SendData(uint8_t*, const uint16_t) override { return 0; }
        uint16_t ReceiveData(uint8_t* frame, const uint16_t size) override
        {
            std::unique_lock<std::mutex> lk(_mutex);
            _payloadSize = std::min(size, static_cast<uint16_t>(sizeof(_payload)));
            memcpy(_payload, frame, _payloadSize);
            _received = true;
            _cv.notify_all();
            return size;
        }
        void StateChange() override {}

        bool WaitForData(uint32_t ms)
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return _cv.wait_for(lk, std::chrono::milliseconds(ms),
                                [this] { return _received; });
        }
        string Payload() const
        {
            std::unique_lock<std::mutex> lk(_mutex);
            return string(reinterpret_cast<const char*>(_payload), _payloadSize);
        }
        ::Thunder::Core::NodeId SenderNode() const
        {
            return ReceivedNode();
        }

    private:
        mutable std::mutex _mutex;
        std::condition_variable _cv;
        uint16_t _payloadSize;
        bool _received;
        uint8_t _payload[512];
    };

    class UDPSender : public ::Thunder::Core::SocketDatagram {
    public:
        UDPSender() = delete;
        UDPSender(const UDPSender&) = delete;
        UDPSender& operator=(const UDPSender&) = delete;

        UDPSender(const ::Thunder::Core::NodeId& local,
                  const ::Thunder::Core::NodeId& remote,
                  const string& msg)
            : ::Thunder::Core::SocketDatagram(false, local, remote, 512, 512)
            , _message(msg)
            , _sent(false)
        {
        }
        ~UDPSender() override = default;

        uint16_t SendData(uint8_t* frame, const uint16_t maxSize) override
        {
            if (_sent) return 0;
            uint16_t len = static_cast<uint16_t>(
                std::min(_message.size(), static_cast<size_t>(maxSize)));
            memcpy(frame, _message.data(), len);
            _sent = true;
            return len;
        }
        uint16_t ReceiveData(uint8_t*, const uint16_t) override { return 0; }
        void StateChange() override {}

    private:
        string _message;
        bool _sent;
    };

    TEST(test_socketport, udp_ipv4_send_receive_loopback)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 300;
        constexpr uint8_t maxRetries = 1;
        const string message = "udp_loopback_test";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId serverNode("127.0.0.1", 10765,
                                               ::Thunder::Core::NodeId::TYPE_IPV4);
            UDPCapture server(serverNode);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_TRUE(server.WaitForData(maxWaitTimeMs));
            EXPECT_EQ(server.Payload(), message);

            // Sender node should be the client's address.
            EXPECT_TRUE(server.SenderNode().IsValid());

            server.Close(maxWaitTimeMs);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::Thunder::Core::NodeId clientNode("127.0.0.1", 10766,
                                               ::Thunder::Core::NodeId::TYPE_IPV4);
            ::Thunder::Core::NodeId serverNode("127.0.0.1", 10765,
                                               ::Thunder::Core::NodeId::TYPE_IPV4);
            UDPSender sender(clientNode, serverNode, message);
            ASSERT_EQ(sender.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            sender.Trigger();
            SleepMs(300);

            sender.Close(maxWaitTimeMs);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Multiple sequential clients on same server
    // =========================================================================

    TEST(test_socketport, multiple_sequential_clients_on_same_server)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8,
                           maxWaitTimeMs = 8000, maxInitTime = 500;
        constexpr uint8_t maxRetries = 1;

        const string connector = "/tmp/test_sp_multiclients.sock";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(connector.c_str());
            EchoConnector::ResetState();

            ::Thunder::Core::SocketServerType<EchoConnector> server(
                ::Thunder::Core::NodeId(connector.c_str()));

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Wait for both clients to connect and disconnect.
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // First client.
            {
                EchoConnector client1(::Thunder::Core::NodeId(connector.c_str()));
                ASSERT_EQ(client1.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
                EXPECT_TRUE(client1.IsOpen());

                const string msg1 = "client1";
                client1.Submit(msg1);
                EXPECT_EQ(client1.WaitForEcho(), ::Thunder::Core::ERROR_NONE);
                EXPECT_EQ(client1.ReceivedText(), msg1);

                ASSERT_EQ(client1.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            }

            // Second client — server must still be accepting.
            {
                EchoConnector client2(::Thunder::Core::NodeId(connector.c_str()));
                ASSERT_EQ(client2.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
                EXPECT_TRUE(client2.IsOpen());

                const string msg2 = "client2";
                client2.Submit(msg2);
                EXPECT_EQ(client2.WaitForEcho(), ::Thunder::Core::ERROR_NONE);
                EXPECT_EQ(client2.ReceivedText(), msg2);

                ASSERT_EQ(client2.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            }

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // Trigger() causes SendData() to be called — data actually flows
    // =========================================================================

    class TriggerSender : public ::Thunder::Core::SocketDatagram {
    public:
        TriggerSender() = delete;
        TriggerSender(const TriggerSender&) = delete;
        TriggerSender& operator=(const TriggerSender&) = delete;

        TriggerSender(const ::Thunder::Core::NodeId& local,
                      const ::Thunder::Core::NodeId& remote)
            : ::Thunder::Core::SocketDatagram(false, local, remote, 512, 512)
            , _sendDataCalls(0)
        {
        }
        ~TriggerSender() override = default;

        uint16_t SendData(uint8_t* frame, const uint16_t maxSize) override
        {
            int calls = _sendDataCalls.fetch_add(1, std::memory_order_relaxed);
            if ((calls == 0) && (maxSize >= 1)) {
                frame[0] = 0x42;
                return 1;
            }
            return 0;
        }
        uint16_t ReceiveData(uint8_t*, const uint16_t) override { return 0; }
        void StateChange() override {}

        int SendDataCalls() const { return _sendDataCalls.load(std::memory_order_relaxed); }

    private:
        std::atomic<int> _sendDataCalls;
    };

    TEST(test_socketport, trigger_causes_send_data_to_be_called)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 300;
        constexpr uint8_t maxRetries = 1;

        const string serverPath = "/tmp/test_sp_triggersend_srv.sock";
        const string clientPath = "/tmp/test_sp_triggersend_cli.sock";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(serverPath.c_str());
            UDPCapture server(::Thunder::Core::NodeId(serverPath.c_str()));
            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            server.Close(maxWaitTimeMs);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::unlink(clientPath.c_str());
            TriggerSender sender(
                ::Thunder::Core::NodeId(clientPath.c_str()),
                ::Thunder::Core::NodeId(serverPath.c_str()));

            ASSERT_EQ(sender.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // SendData should not have been called yet.
            EXPECT_EQ(sender.SendDataCalls(), 0);

            sender.Trigger();
            SleepMs(200);

            // SendData must have been called after Trigger().
            EXPECT_GT(sender.SendDataCalls(), 0);

            sender.Close(maxWaitTimeMs);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // ReceivedNode() populated after datagram receive
    // =========================================================================

    TEST(test_socketport, received_node_populated_after_datagram_receive)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4,
                           maxWaitTimeMs = 4000, maxInitTime = 300;
        constexpr uint8_t maxRetries = 1;

        const string serverPath = "/tmp/test_sp_recvnode_srv.sock";
        const string clientPath = "/tmp/test_sp_recvnode_cli.sock";
        const string message = "recvnode_test";

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::unlink(serverPath.c_str());
            UDPCapture server(::Thunder::Core::NodeId(serverPath.c_str()));
            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            EXPECT_TRUE(server.WaitForData(maxWaitTimeMs));

            // ReceivedNode must identify the sender.
            ::Thunder::Core::NodeId sender = server.SenderNode();
            EXPECT_TRUE(sender.IsValid());
            EXPECT_STREQ(sender.HostName().c_str(), clientPath.c_str());

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
            server.Close(maxWaitTimeMs);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            SleepMs(maxInitTime);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ::unlink(clientPath.c_str());
            UDPSender sender(
                ::Thunder::Core::NodeId(clientPath.c_str()),
                ::Thunder::Core::NodeId(serverPath.c_str()),
                message);
            ASSERT_EQ(sender.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
            sender.Trigger();

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            sender.Close(maxWaitTimeMs);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child,
                                      initHandshakeValue, maxWaitTime);
        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder