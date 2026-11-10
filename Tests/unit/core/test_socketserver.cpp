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

#include <type_traits>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include <unistd.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // Minimal stream client. The server creates one of these for every
    // accepted connection; the test also uses it to connect to the server.
    class StreamClient : public ::Thunder::Core::SocketStream {
    public:
        StreamClient() = delete;
        StreamClient(const StreamClient&) = delete;
        StreamClient& operator=(const StreamClient&) = delete;

        // Server side: created by SocketServerType when a connection is accepted.
        StreamClient(const SOCKET& connector, const ::Thunder::Core::NodeId& remote,
            ::Thunder::Core::SocketServerType<StreamClient>*)
            : ::Thunder::Core::SocketStream(false, connector, remote, 512, 512)
        {
        }
        // Client side: connects to the server.
        explicit StreamClient(const ::Thunder::Core::NodeId& remote)
            : ::Thunder::Core::SocketStream(false, remote.AnyInterface(), remote, 512, 512)
        {
        }
        ~StreamClient() override
        {
            Close(::Thunder::Core::infinite);
        }

        uint16_t SendData(uint8_t*, const uint16_t) override { return 0; }
        uint16_t ReceiveData(uint8_t*, const uint16_t size) override { return size; }
        void StateChange() override { }
    };

    using Server = ::Thunder::Core::SocketServerType<StreamClient>;
    using Iterator = Server::Iterator;

    static uint32_t Walk(Iterator& iterator)
    {
        uint32_t count = 0;

        while (iterator.Next() == true) {
            EXPECT_TRUE(iterator.IsValid());
            EXPECT_TRUE(iterator.Client().IsValid());
            ++count;
        }

        return count;
    }

    // Waits until the server has accepted the expected number of clients.
    // Uses Clients().Count() rather than server.Count(): Clients() takes the
    // server's lock, server.Count() reads the client map without it.
    static bool WaitForClients(const Server& server, const uint32_t expected)
    {
        uint32_t waited = 0;

        while ((server.Clients().Count() < expected) && (waited < 2000)) {
            SleepMs(10);
            waited += 10;
        }

        return (server.Clients().Count() == expected);
    }

    // Copy and move are private: only the server (through Clients()) may
    // create iterators. Outside code must not be able to copy or move one.
    static_assert(!std::is_copy_constructible<Iterator>::value, "copy constructor must not be public");
    static_assert(!std::is_move_constructible<Iterator>::value, "move constructor must not be public");
    static_assert(!std::is_copy_assignable<Iterator>::value, "copy assignment must not be public");
    static_assert(!std::is_move_assignable<Iterator>::value, "move assignment must not be public");

    // The original startup crash: Clients() on a server with no clients
    // returns an empty iterator that must be safe to use.
    TEST(Core_SocketServerIterator, ClientsOnEmptyServer)
    {
        const ::Thunder::Core::NodeId node("/tmp/test_socketserver_empty.sock");
        Server server(node);

        Iterator iterator = server.Clients();

        EXPECT_EQ(iterator.Count(), 0u);
        EXPECT_FALSE(iterator.IsValid());
        EXPECT_FALSE(iterator.Next());
        EXPECT_FALSE(iterator.IsValid());

        iterator.Reset();
        EXPECT_FALSE(iterator.Next());
    }

    // Clients() returns an iterator over every connected client.
    TEST(Core_SocketServerIterator, ClientsWithConnectedClients)
    {
        const char* path = "/tmp/test_socketserver_clients.sock";
        ::unlink(path);

        const ::Thunder::Core::NodeId node(path);
        Server server(node);
        ASSERT_EQ(server.Open(1000), ::Thunder::Core::ERROR_NONE);

        StreamClient first(node);
        StreamClient second(node);
        ASSERT_EQ(first.Open(1000), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(second.Open(1000), ::Thunder::Core::ERROR_NONE);
        ASSERT_TRUE(WaitForClients(server, 2));

        Iterator iterator = server.Clients();

        EXPECT_EQ(iterator.Count(), 2u);
        EXPECT_FALSE(iterator.IsValid()); // starts before the first client
        EXPECT_EQ(Walk(iterator), 2u);
        EXPECT_FALSE(iterator.IsValid()); // past the last client

        // Reset() walks the same clients again.
        iterator.Reset();
        EXPECT_EQ(Walk(iterator), 2u);

        first.Close(1000);
        second.Close(1000);
        server.Close(1000);
    }

    // Each Clients() call returns its own iterator: walking one does not
    // move the other.
    TEST(Core_SocketServerIterator, ClientsReturnsIndependentIterators)
    {
        const char* path = "/tmp/test_socketserver_independent.sock";
        ::unlink(path);

        const ::Thunder::Core::NodeId node(path);
        Server server(node);
        ASSERT_EQ(server.Open(1000), ::Thunder::Core::ERROR_NONE);

        StreamClient first(node);
        StreamClient second(node);
        ASSERT_EQ(first.Open(1000), ::Thunder::Core::ERROR_NONE);
        ASSERT_EQ(second.Open(1000), ::Thunder::Core::ERROR_NONE);
        ASSERT_TRUE(WaitForClients(server, 2));

        Iterator a = server.Clients();
        Iterator b = server.Clients();

        EXPECT_EQ(Walk(a), 2u);

        EXPECT_FALSE(b.IsValid()); // b is still at its start
        EXPECT_EQ(Walk(b), 2u);

        first.Close(1000);
        second.Close(1000);
        server.Close(1000);
    }

} // namespace Core
} // namespace Tests
} // namespace Thunder
