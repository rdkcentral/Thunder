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

#include <utility>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // Simple client type; no real sockets are needed to test the iterator.
    class DummyClient {
    public:
        explicit DummyClient(const uint32_t id)
            : _id(id)
        {
        }
        virtual ~DummyClient() = default;

        uint32_t Id() const
        {
            return _id;
        }

    private:
        uint32_t _id;
    };

    using Server = ::Thunder::Core::SocketServerType<DummyClient>;
    using Iterator = Server::Iterator;
    using ClientMap = std::map<uint32_t, ::Thunder::Core::ProxyType<DummyClient>>;

    // Builds a map with clients 1..count.
    static ClientMap MakeClients(const uint32_t count)
    {
        ClientMap clients;

        for (uint32_t id = 1; id <= count; ++id) {
            clients.emplace(id, ::Thunder::Core::ProxyType<DummyClient>::Create(id));
        }

        return clients;
    }

    // Counts how many clients Next() still visits.
    static uint32_t Walk(Iterator& iterator)
    {
        uint32_t count = 0;

        while (iterator.Next() == true) {
            ++count;
        }

        return count;
    }

    // ---------------------------------------------------------------------
    // The fix: moving an iterator that sits at end() (empty or fully walked)
    // must not keep the old list's end(). Before the fix these crashed.
    // The moved iterator now restarts at the first client of its own list.
    // ---------------------------------------------------------------------

    TEST(Core_SocketServerIterator, MoveConstructFromEmpty)
    {
        Iterator source;
        Iterator moved(std::move(source));

        EXPECT_EQ(moved.Count(), 0u);
        EXPECT_FALSE(moved.Next());
        EXPECT_FALSE(moved.IsValid());
    }

    TEST(Core_SocketServerIterator, MoveConstructFromEnd)
    {
        ClientMap clients = MakeClients(2);
        Iterator source(clients);
        EXPECT_EQ(Walk(source), 2u); // source is now at end()

        Iterator moved(std::move(source));

        EXPECT_EQ(moved.Count(), 2u);
        EXPECT_FALSE(moved.IsValid()); // restarted before the first client
        EXPECT_EQ(Walk(moved), 2u);
    }

    TEST(Core_SocketServerIterator, MoveAssignFromEnd)
    {
        ClientMap clients = MakeClients(2);
        Iterator source(clients);
        EXPECT_EQ(Walk(source), 2u); // source is now at end()

        Iterator target;
        target = std::move(source);

        EXPECT_EQ(target.Count(), 2u);
        EXPECT_FALSE(target.IsValid()); // restarted before the first client
        EXPECT_EQ(Walk(target), 2u);
    }

    // ---------------------------------------------------------------------
    // Move: the destination takes the clients and starts from the first one,
    // like a copy; the source is left empty and safe to use.
    // ---------------------------------------------------------------------

    TEST(Core_SocketServerIterator, MoveConstruct)
    {
        ClientMap clients = MakeClients(3);
        Iterator source(clients);
        ASSERT_TRUE(source.Next()); // on client 1

        Iterator moved(std::move(source));

        EXPECT_EQ(moved.Count(), 3u);
        EXPECT_FALSE(moved.IsValid()); // starts before the first client
        ASSERT_TRUE(moved.Next());
        EXPECT_EQ(moved.Client()->Id(), 1u);
        EXPECT_EQ(Walk(moved), 2u); // clients 2 and 3 left

        EXPECT_EQ(source.Count(), 0u);
        EXPECT_FALSE(source.IsValid());
        EXPECT_FALSE(source.Next());
    }

    TEST(Core_SocketServerIterator, MoveAssign)
    {
        ClientMap clients = MakeClients(3);
        Iterator source(clients);
        ASSERT_TRUE(source.Next()); // on client 1

        ClientMap old = MakeClients(5);
        Iterator target(old);
        target = std::move(source);

        EXPECT_EQ(target.Count(), 3u);
        EXPECT_FALSE(target.IsValid()); // starts before the first client
        ASSERT_TRUE(target.Next());
        EXPECT_EQ(target.Client()->Id(), 1u);
        EXPECT_EQ(Walk(target), 2u);

        EXPECT_EQ(source.Count(), 0u);
        EXPECT_FALSE(source.Next());
    }

    // ---------------------------------------------------------------------
    // Copy: the copy gets its own list and starts from the first client;
    // the source is not changed.
    // ---------------------------------------------------------------------

    TEST(Core_SocketServerIterator, CopyConstruct)
    {
        ClientMap clients = MakeClients(3);
        Iterator source(clients);
        ASSERT_TRUE(source.Next()); // on client 1

        Iterator copy(source);

        EXPECT_EQ(copy.Count(), 3u);
        EXPECT_FALSE(copy.IsValid()); // starts before the first client
        EXPECT_EQ(Walk(copy), 3u);

        ASSERT_TRUE(source.IsValid()); // source unchanged
        EXPECT_EQ(source.Client()->Id(), 1u);
    }

    TEST(Core_SocketServerIterator, CopyAssign)
    {
        ClientMap clients = MakeClients(3);
        Iterator source(clients);
        ASSERT_TRUE(source.Next()); // on client 1

        ClientMap old = MakeClients(5);
        Iterator target(old);
        target = source;

        EXPECT_EQ(target.Count(), 3u);
        EXPECT_FALSE(target.IsValid()); // starts before the first client
        EXPECT_EQ(Walk(target), 3u);

        ASSERT_TRUE(source.IsValid()); // source unchanged
        EXPECT_EQ(source.Client()->Id(), 1u);
    }

    // ---------------------------------------------------------------------
    // Assignment returns *this (chaining works) and self-assignment is safe.
    // ---------------------------------------------------------------------

    TEST(Core_SocketServerIterator, ChainedAssign)
    {
        ClientMap clients = MakeClients(2);
        Iterator source(clients);
        Iterator a;
        Iterator b;

        a = b = source;

        EXPECT_EQ(a.Count(), 2u);
        EXPECT_EQ(b.Count(), 2u);
        EXPECT_EQ(Walk(a), 2u);
    }

    TEST(Core_SocketServerIterator, SelfAssign)
    {
        ClientMap clients = MakeClients(2);
        Iterator iterator(clients);
        Iterator& alias = iterator; // alias avoids self-assignment warnings

        iterator = alias;
        EXPECT_EQ(Walk(iterator), 2u);

        iterator = std::move(alias);
        EXPECT_EQ(iterator.Count(), 2u);
    }

} // namespace Core
} // namespace Tests
} // namespace Thunder
