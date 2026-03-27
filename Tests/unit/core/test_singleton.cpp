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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // -------------------------------------------------------------------------
    // Test types
    // -------------------------------------------------------------------------

    class SingletonTypeOne {
    public:
        SingletonTypeOne() = default;
        virtual ~SingletonTypeOne() = default;
        bool operator==(const SingletonTypeOne&) const { return true; }
    };

    class SingletonTypeTwo {
    public:
        SingletonTypeTwo() = default;
        explicit SingletonTypeTwo(string) {}
        virtual ~SingletonTypeTwo() = default;
    };

    class SingletonTypeThree {
    public:
        SingletonTypeThree() = default;
        explicit SingletonTypeThree(string, string) {}
        virtual ~SingletonTypeThree() = default;
    };

    // Tracks construction and destruction counts to verify lifetime behaviour.
    class TrackedSingleton {
    public:
        static int constructCount;
        static int destructCount;

        TrackedSingleton()  { ++constructCount; }
        ~TrackedSingleton() { ++destructCount;  }

        static void Reset() { constructCount = 0; destructCount = 0; }
    };
    int TrackedSingleton::constructCount = 0;
    int TrackedSingleton::destructCount  = 0;

    // -------------------------------------------------------------------------
    // Original test — preserved exactly
    // -------------------------------------------------------------------------

    TEST(test_singleton, simple_singleton)
    {
        // 'old' use
        ::Thunder::Tests::Core::SingletonTypeOne& objectTypeOne = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();

        // Multiple inheritance: SingletonType<T> derives from both Singleton and T.
        // Instance() returns T& but it is safe to downcast to SingletonType<T>*.
        ASSERT_NE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne), nullptr);
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&objectTypeOne)->ImplementationName().c_str(), "SingletonTypeOne");

        // Well-known lifetime!
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("My custom 2-string");
        ::Thunder::Tests::Core::SingletonTypeTwo& objectTypeTwo = ::Thunder::Core::SingletonType<SingletonTypeTwo>::Instance();
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeTwo>*>(&objectTypeTwo)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeTwo>*>(&objectTypeTwo)->ImplementationName().c_str(), "SingletonTypeTwo");
        EXPECT_TRUE(::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose());

        // Well-known lifetime!
        ::Thunder::Core::SingletonType<SingletonTypeThree>::Create("My first custom 3-string", "My second custom 3-string");
        ::Thunder::Tests::Core::SingletonTypeThree& objectTypeThree = ::Thunder::Core::SingletonType<SingletonTypeThree>::Instance();
        EXPECT_FALSE(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeThree>*>(&objectTypeThree)->ImplementationName().empty());
        EXPECT_STREQ(dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeThree>*>(&objectTypeThree)->ImplementationName().c_str(), "SingletonTypeThree");
        EXPECT_TRUE(::Thunder::Core::SingletonType<SingletonTypeThree>::Dispose());

        // SingletonTypeOne has not yet been destroyed
        EXPECT_EQ(::Thunder::Core::SingletonType<SingletonTypeOne>::Instance(), ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance());

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Instance identity — same reference across multiple calls
    // -------------------------------------------------------------------------

    TEST(test_singleton, instance_returns_same_object)
    {
        SingletonTypeOne& a = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        SingletonTypeOne& b = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        SingletonTypeOne& c = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();

        EXPECT_EQ(&a, &b);
        EXPECT_EQ(&b, &c);

        ::Thunder::Core::Singleton::Dispose();
    }

    // -------------------------------------------------------------------------
    // Dispose() return value — true first time, false if already disposed
    // -------------------------------------------------------------------------

    TEST(test_singleton, dispose_returns_false_when_already_disposed)
    {
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("test");

        EXPECT_TRUE(::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose());
        EXPECT_FALSE(::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose());
    }

    // -------------------------------------------------------------------------
    // Re-init cycle — Create -> Dispose -> Create -> Instance works correctly
    // -------------------------------------------------------------------------

    TEST(test_singleton, reinit_after_dispose)
    {
        TrackedSingleton::Reset();

        ::Thunder::Core::SingletonType<TrackedSingleton>::Create();
        EXPECT_EQ(TrackedSingleton::constructCount, 1);
        EXPECT_EQ(TrackedSingleton::destructCount,  0);

        EXPECT_TRUE(::Thunder::Core::SingletonType<TrackedSingleton>::Dispose());
        EXPECT_EQ(TrackedSingleton::constructCount, 1);
        EXPECT_EQ(TrackedSingleton::destructCount,  1);

        // Re-create after dispose — must work cleanly
        ::Thunder::Core::SingletonType<TrackedSingleton>::Create();
        EXPECT_EQ(TrackedSingleton::constructCount, 2);
        EXPECT_EQ(TrackedSingleton::destructCount,  1);

        TrackedSingleton& instance = ::Thunder::Core::SingletonType<TrackedSingleton>::Instance();
        EXPECT_NE(&instance, nullptr);

        EXPECT_TRUE(::Thunder::Core::SingletonType<TrackedSingleton>::Dispose());
        EXPECT_EQ(TrackedSingleton::constructCount, 2);
        EXPECT_EQ(TrackedSingleton::destructCount,  2);
    }

    // -------------------------------------------------------------------------
    // Destructor is called exactly once on Dispose()
    // -------------------------------------------------------------------------

    TEST(test_singleton, destructor_called_exactly_once_on_dispose)
    {
        TrackedSingleton::Reset();

        ::Thunder::Core::SingletonType<TrackedSingleton>::Create();
        EXPECT_EQ(TrackedSingleton::destructCount, 0);

        ::Thunder::Core::SingletonType<TrackedSingleton>::Dispose();
        EXPECT_EQ(TrackedSingleton::destructCount, 1);

        // Second Dispose() must not call destructor again
        ::Thunder::Core::SingletonType<TrackedSingleton>::Dispose();
        EXPECT_EQ(TrackedSingleton::destructCount, 1);
    }

    // -------------------------------------------------------------------------
    // Singleton::Dispose() cleans up all registered singletons
    // -------------------------------------------------------------------------

    TEST(test_singleton, global_dispose_cleans_all)
    {
        TrackedSingleton::Reset();

        ::Thunder::Core::SingletonType<TrackedSingleton>::Create();
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("global-dispose-test");

        EXPECT_EQ(TrackedSingleton::constructCount, 1);
        EXPECT_EQ(TrackedSingleton::destructCount,  0);

        ::Thunder::Core::Singleton::Dispose();

        EXPECT_EQ(TrackedSingleton::destructCount, 1);

        // After global dispose both singletons should be gone — Dispose() returns false
        EXPECT_FALSE(::Thunder::Core::SingletonType<TrackedSingleton>::Dispose());
        EXPECT_FALSE(::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose());
    }

    // -------------------------------------------------------------------------
    // Instance() via GetObject path — lazy construction on first call
    // -------------------------------------------------------------------------

    TEST(test_singleton, lazy_construction_via_instance)
    {
        TrackedSingleton::Reset();

        // No Create() called — Instance() should construct lazily
        EXPECT_EQ(TrackedSingleton::constructCount, 0);

        TrackedSingleton& instance = ::Thunder::Core::SingletonType<TrackedSingleton>::Instance();
        EXPECT_NE(&instance, nullptr);
        EXPECT_EQ(TrackedSingleton::constructCount, 1);

        // Second call must not construct again
        ::Thunder::Core::SingletonType<TrackedSingleton>::Instance();
        EXPECT_EQ(TrackedSingleton::constructCount, 1);

        ::Thunder::Core::Singleton::Dispose();
        EXPECT_EQ(TrackedSingleton::destructCount, 1);
    }

    // -------------------------------------------------------------------------
    // ImplementationName — correct on both Instance() and Create() paths
    // -------------------------------------------------------------------------

    TEST(test_singleton, implementation_name_via_instance_path)
    {
        SingletonTypeOne& obj = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        auto* typed = dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeOne>*>(&obj);

        ASSERT_NE(typed, nullptr);
        EXPECT_STREQ(typed->ImplementationName().c_str(), "SingletonTypeOne");

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_singleton, implementation_name_via_create_path)
    {
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("name-test");
        SingletonTypeTwo& obj = ::Thunder::Core::SingletonType<SingletonTypeTwo>::Instance();
        auto* typed = dynamic_cast<::Thunder::Core::SingletonType<SingletonTypeTwo>*>(&obj);

        ASSERT_NE(typed, nullptr);
        EXPECT_STREQ(typed->ImplementationName().c_str(), "SingletonTypeTwo");

        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose();
    }

    // -------------------------------------------------------------------------
    // Multiple independent singleton types do not interfere with each other
    // -------------------------------------------------------------------------

    TEST(test_singleton, independent_singleton_types_do_not_interfere)
    {
        SingletonTypeOne& one = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Create("interference-test");
        SingletonTypeTwo& two = ::Thunder::Core::SingletonType<SingletonTypeTwo>::Instance();

        // Suppress unused warning — the point is both are alive simultaneously
        (void)two;

        // Disposing two must not affect one
        ::Thunder::Core::SingletonType<SingletonTypeTwo>::Dispose();

        SingletonTypeOne& oneAgain = ::Thunder::Core::SingletonType<SingletonTypeOne>::Instance();
        EXPECT_EQ(&one, &oneAgain);

        ::Thunder::Core::Singleton::Dispose();
    }

        // A singleton with detectable partial construction.
    // The constructor sets two sentinel values with a yield() between them
    // to widen the window in which another thread can observe an
    // incompletely constructed object.
    class DCLPSingleton {
    public:
        static constexpr uint32_t SENTINEL_A = 0xDEADBEEF;
        static constexpr uint32_t SENTINEL_B = 0xCAFEBABE;

        DCLPSingleton()
        {
            _a = SENTINEL_A;
            // Yield to maximise the chance that another thread observes
            // the pointer as non-null before _b is written.
            std::this_thread::yield();
            _b = SENTINEL_B;
        }
        ~DCLPSingleton() = default;

        bool IsFullyConstructed() const
        {
            return (_a == SENTINEL_A) && (_b == SENTINEL_B);
        }

    private:
        // Padding between the two sentinels increases the probability that
        // the CPU will commit _a and the pointer write before _b on weakly-
        // ordered architectures (ARM).
        uint32_t _a;
        uint8_t  _pad[64];
        uint32_t _b;
    };

    // Stress test: N threads all call Instance() simultaneously through a
    // barrier, then verify the returned object is fully constructed.
    //
    // On x86 this test may pass even with the bug present due to the strong
    // memory model. On ARM it is more likely to expose the race. In both
    // cases ThreadSanitizer will flag the underlying data race deterministically.
    TEST(test_singleton, concurrent_instance_returns_fully_constructed_object)
    {
        constexpr int NUM_THREADS = 32;

        // Release all threads simultaneously against a cold singleton.
        // The yield() inside DCLPSingleton's constructor widens the window
        // enough that a broken DCLP implementation will hand out a partially
        // constructed object to one or more threads on this single attempt.
        std::atomic<bool> go{ false };
        std::atomic<int>  failures{ 0 };
        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);

        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&]() {
                while (!go.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }

                DCLPSingleton& instance = ::Thunder::Core::SingletonType<DCLPSingleton>::Instance();

                if (!instance.IsFullyConstructed()) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        go.store(true, std::memory_order_release);

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_EQ(failures.load(), 0) << "Partially constructed singleton observed — DCLP race triggered.";

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
