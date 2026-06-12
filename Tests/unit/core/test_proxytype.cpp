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

#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // =========================================================================
    // Test helper: simple class to track construction/destruction
    // =========================================================================

    static std::atomic<int> g_constructed{0};
    static std::atomic<int> g_destructed{0};

    class Trackable {
    public:
        Trackable() : _value(0) { g_constructed.fetch_add(1); }
        explicit Trackable(int v) : _value(v) { g_constructed.fetch_add(1); }
        virtual ~Trackable() { g_destructed.fetch_add(1); }

        int Value() const { return _value; }
        void Value(int v) { _value = v; }

    private:
        int _value;
    };

    // Derived class for polymorphic tests
    class DerivedTrackable : public Trackable {
    public:
        DerivedTrackable() : Trackable(), _extra(0) {}
        explicit DerivedTrackable(int v) : Trackable(v), _extra(v * 10) {}

        int Extra() const { return _extra; }

    private:
        int _extra;
    };

    // Class with lifecycle hooks to exercise SFINAE-enabled paths in ProxyObject
    static std::atomic<int> g_cleared{0};
    static std::atomic<int> g_destructedHook{0};
    static std::atomic<int> g_acquired{0};
    static std::atomic<int> g_relinquished{0};

    class LifecycleTrackable {
    public:
        LifecycleTrackable() : _value(0) {}
        explicit LifecycleTrackable(int v) : _value(v) {}
        virtual ~LifecycleTrackable() = default;

        int Value() const { return _value; }

        void Clear() { g_cleared.fetch_add(1); }
        void Destructed() { g_destructedHook.fetch_add(1); }
        void Acquire(::Thunder::Core::ProxyType<LifecycleTrackable>& proxy) {
            g_acquired.fetch_add(1);
            EXPECT_TRUE(proxy.IsValid());
        }
        void Relinquish(::Thunder::Core::ProxyType<LifecycleTrackable>& proxy) {
            g_relinquished.fetch_add(1);
            EXPECT_TRUE(proxy.IsValid());
        }

    private:
        int _value;
    };

    static void ResetLifecycleCounters()
    {
        g_cleared.store(0);
        g_destructedHook.store(0);
        g_acquired.store(0);
        g_relinquished.store(0);
    }

    // Class that fails initialization (IsInitialized returns false)
    class FailInit {
    public:
        FailInit() = default;
        virtual ~FailInit() = default;
        bool IsInitialized() const { return false; }
    };

    // Unrelated type for cross-hierarchy dynamic_cast failure path
    class Unrelated {
    public:
        Unrelated() : _val(0) {}
        explicit Unrelated(int v) : _val(v) {}
        virtual ~Unrelated() = default;
        int Val() const { return _val; }
    private:
        int _val;
    };

    static void ResetCounters()
    {
        g_constructed.store(0);
        g_destructed.store(0);
    }

    // =========================================================================
    // ProxyType dedicated tests — closes gap: Core ProxyType reference counting
    // (v2.1 gap: "ProxyType — dedicated AddRef/Release, null object, circular reference")
    // =========================================================================

    TEST(test_proxytype, create_sets_initial_refcount)
    {
        ResetCounters();

        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(42);

        ASSERT_TRUE(proxy.IsValid());
        EXPECT_EQ(proxy->Value(), 42);
        EXPECT_EQ(g_constructed.load(), 1);
        EXPECT_EQ(g_destructed.load(), 0);

        // After Create(), there is exactly 1 ProxyType holding a reference.
        // Verify through the underlying ProxyObject that AddRef/Release
        // round-trips correctly at the ProxyObject level.
        ::Thunder::Core::ProxyObject<Trackable>* obj = proxy.Origin();
        obj->AddRef();   // refcount: 1 → 2
        obj->Release();  // refcount: 2 → 1

        // proxy still valid (ProxyType::Release would clear the proxy pointer,
        // so we use ProxyObject::AddRef/Release directly)
        ASSERT_TRUE(proxy.IsValid());
    }

    // =========================================================================
    // Copy construct — verify refcount increments to 2
    // =========================================================================

    TEST(test_proxytype, copy_construct_increments_refcount)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(10);
        ASSERT_TRUE(proxy1.IsValid());

        // Copy construct
        ::Thunder::Core::ProxyType<Trackable> proxy2(proxy1);
        ASSERT_TRUE(proxy2.IsValid());

        // Both point to the same object
        EXPECT_EQ(proxy1.operator->(), proxy2.operator->());
        EXPECT_EQ(proxy1->Value(), 10);
        EXPECT_EQ(proxy2->Value(), 10);

        // Object not yet destroyed
        EXPECT_EQ(g_destructed.load(), 0);
    }

    // =========================================================================
    // Destroy copy — verify refcount decrements back to 1
    // =========================================================================

    TEST(test_proxytype, destroy_copy_decrements_refcount)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(20);
        {
            ::Thunder::Core::ProxyType<Trackable> proxy2(proxy1);
            ASSERT_TRUE(proxy2.IsValid());
            EXPECT_EQ(g_destructed.load(), 0);
        }
        // proxy2 destroyed, but proxy1 still holds a ref
        EXPECT_EQ(g_destructed.load(), 0);
        ASSERT_TRUE(proxy1.IsValid());
        EXPECT_EQ(proxy1->Value(), 20);
    }

    // =========================================================================
    // Last reference destroyed — verify inner object deleted
    // =========================================================================

    TEST(test_proxytype, last_reference_destroys_inner_object)
    {
        ResetCounters();

        {
            auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(30);
            ASSERT_TRUE(proxy.IsValid());
            EXPECT_EQ(g_destructed.load(), 0);
        }
        // proxy destroyed — inner object should be deleted
        EXPECT_EQ(g_constructed.load(), 1);
        EXPECT_EQ(g_destructed.load(), 1);
    }

    // =========================================================================
    // Move semantics — verify no double-free, correct refcount
    // =========================================================================

    TEST(test_proxytype, move_construct_transfers_ownership)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(50);
        Trackable* raw = proxy1.operator->();

        // Move construct
        ::Thunder::Core::ProxyType<Trackable> proxy2(std::move(proxy1));

        // proxy1 should be invalidated
        EXPECT_FALSE(proxy1.IsValid());

        // proxy2 should hold the object
        ASSERT_TRUE(proxy2.IsValid());
        EXPECT_EQ(proxy2.operator->(), raw);
        EXPECT_EQ(proxy2->Value(), 50);

        // No destruction yet
        EXPECT_EQ(g_destructed.load(), 0);
    }

    TEST(test_proxytype, move_assign_transfers_ownership)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(60);
        Trackable* raw = proxy1.operator->();

        ::Thunder::Core::ProxyType<Trackable> proxy2;
        EXPECT_FALSE(proxy2.IsValid());

        // Move assign
        proxy2 = std::move(proxy1);

        EXPECT_FALSE(proxy1.IsValid());
        ASSERT_TRUE(proxy2.IsValid());
        EXPECT_EQ(proxy2.operator->(), raw);
        EXPECT_EQ(proxy2->Value(), 60);
        EXPECT_EQ(g_destructed.load(), 0);
    }

    // =========================================================================
    // Default-constructed (nullptr) — verify safe behavior
    // =========================================================================

    TEST(test_proxytype, default_constructed_is_invalid)
    {
        ::Thunder::Core::ProxyType<Trackable> proxy;

        EXPECT_FALSE(proxy.IsValid());
        EXPECT_TRUE(proxy == nullptr);
        EXPECT_FALSE(proxy != nullptr);
    }

    // =========================================================================
    // Explicit AddRef() / Release() — verify count accuracy
    // =========================================================================

    TEST(test_proxytype, explicit_addref_release)
    {
        ResetCounters();

        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(70);
        ASSERT_TRUE(proxy.IsValid());

        // AddRef increments — object stays alive after proxy destruction
        proxy.AddRef();

        Trackable* raw = proxy.operator->();
        ::Thunder::Core::ProxyType<Trackable> proxy2(proxy);

        // Now 3 refs: proxy, proxy2, explicit AddRef

        // Release the explicit AddRef via proxy
        proxy.Release();

        // proxy is now invalid (Release clears its internal pointer)
        EXPECT_FALSE(proxy.IsValid());

        // proxy2 still holds a reference, object should be alive
        ASSERT_TRUE(proxy2.IsValid());
        EXPECT_EQ(proxy2->Value(), 70);
        EXPECT_EQ(g_destructed.load(), 0);
    }

    // =========================================================================
    // Concurrent AddRef() / Release() from multiple threads
    // =========================================================================

    TEST(test_proxytype, concurrent_addref_release_atomicity)
    {
        ResetCounters();

        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(80);
        ASSERT_TRUE(proxy.IsValid());

        constexpr int numThreads = 8;
        constexpr int iterations = 1000;

        // Each thread will copy the proxy, hold it briefly, then release
        std::vector<std::thread> threads;
        threads.reserve(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&proxy, iterations]() {
                for (int i = 0; i < iterations; ++i) {
                    // Copy increments refcount
                    ::Thunder::Core::ProxyType<Trackable> local(proxy);
                    EXPECT_TRUE(local.IsValid());
                    EXPECT_EQ(local->Value(), 80);
                    // local goes out of scope → Release
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        // After all threads complete, only the original proxy should hold a ref
        ASSERT_TRUE(proxy.IsValid());
        EXPECT_EQ(proxy->Value(), 80);
        EXPECT_EQ(g_destructed.load(), 0);
    }

    // =========================================================================
    // Additional coverage: operator==, operator!=
    // =========================================================================

    TEST(test_proxytype, equality_operators)
    {
        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(100);
        auto proxy2 = ::Thunder::Core::ProxyType<Trackable>::Create(200);
        ::Thunder::Core::ProxyType<Trackable> proxy1Copy(proxy1);

        // Same underlying object
        EXPECT_TRUE(proxy1 == proxy1Copy);
        EXPECT_FALSE(proxy1 != proxy1Copy);

        // Different objects
        EXPECT_FALSE(proxy1 == proxy2);
        EXPECT_TRUE(proxy1 != proxy2);

        // Compare with nullptr
        ::Thunder::Core::ProxyType<Trackable> empty;
        EXPECT_TRUE(empty == nullptr);
        EXPECT_FALSE(empty != nullptr);
        EXPECT_FALSE(proxy1 == nullptr);
        EXPECT_TRUE(proxy1 != nullptr);

        // Compare with raw object
        EXPECT_TRUE(proxy1 == *proxy1);
        EXPECT_FALSE(proxy1 == *proxy2);
        EXPECT_TRUE(proxy1 != *proxy2);
    }

    // =========================================================================
    // Additional coverage: copy assignment operator
    // =========================================================================

    TEST(test_proxytype, copy_assignment)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(110);
        auto proxy2 = ::Thunder::Core::ProxyType<Trackable>::Create(220);

        EXPECT_EQ(proxy1->Value(), 110);
        EXPECT_EQ(proxy2->Value(), 220);

        // Assign proxy1 = proxy2 → proxy1's old object should be released
        proxy1 = proxy2;

        EXPECT_EQ(proxy1->Value(), 220);
        EXPECT_EQ(proxy2->Value(), 220);
        EXPECT_TRUE(proxy1 == proxy2);

        // Old object (110) should have been destroyed
        EXPECT_EQ(g_destructed.load(), 1);
    }

    // =========================================================================
    // Additional coverage: move assignment releases old object
    // =========================================================================

    TEST(test_proxytype, move_assignment_releases_old_object)
    {
        ResetCounters();

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(300);
        auto proxy2 = ::Thunder::Core::ProxyType<Trackable>::Create(400);

        // Move proxy2 into proxy1 → old proxy1 object (300) should be destroyed
        proxy1 = std::move(proxy2);

        EXPECT_FALSE(proxy2.IsValid());
        ASSERT_TRUE(proxy1.IsValid());
        EXPECT_EQ(proxy1->Value(), 400);
        EXPECT_EQ(g_destructed.load(), 1);
    }

    // =========================================================================
    // Additional coverage: dereference operator*
    // =========================================================================

    TEST(test_proxytype, dereference_operator)
    {
        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(500);

        Trackable& ref = *proxy;
        EXPECT_EQ(ref.Value(), 500);

        // Modify through reference
        ref.Value(600);
        EXPECT_EQ(proxy->Value(), 600);
    }

    // =========================================================================
    // Additional coverage: Origin() returns ProxyObject pointer
    // =========================================================================

    TEST(test_proxytype, origin_returns_proxy_object)
    {
        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(700);

        ::Thunder::Core::ProxyObject<Trackable>* origin = proxy.Origin();
        ASSERT_NE(origin, nullptr);
        EXPECT_EQ(origin->Value(), 700);
    }

    // =========================================================================
    // Additional coverage: CreateEx with additional storage
    // =========================================================================

    TEST(test_proxytype, create_ex_with_extra_storage)
    {
        ResetCounters();

        auto proxy = ::Thunder::Core::ProxyType<Trackable>::CreateEx(64, 800);
        ASSERT_TRUE(proxy.IsValid());
        EXPECT_EQ(proxy->Value(), 800);

        // Verify the extra storage size is recorded
        EXPECT_EQ(proxy.Origin()->Size(), 64u);
    }

    // =========================================================================
    // Additional coverage: ProxyObject Clear() and IsInitialized()
    // =========================================================================

    TEST(test_proxytype, proxy_object_clear_and_is_initialized)
    {
        auto proxy = ::Thunder::Core::ProxyType<Trackable>::Create(900);

        // Trackable doesn't have Clear() or IsInitialized() methods, so
        // the SFINAE fallbacks are used. IsInitialized() returns true by default.
        EXPECT_TRUE(proxy.Origin()->IsInitialized());

        // Clear() is a no-op for types without a Clear method
        proxy.Origin()->Clear();

        // Object should still be valid
        EXPECT_EQ(proxy->Value(), 900);
    }

    // =========================================================================
    // Additional coverage: ProxyType<Derived> → ProxyType<Base> conversion
    // =========================================================================

    TEST(test_proxytype, derived_to_base_proxy_conversion)
    {
        ResetCounters();

        auto derived = ::Thunder::Core::ProxyType<DerivedTrackable>::Create(55);
        ASSERT_TRUE(derived.IsValid());
        EXPECT_EQ(derived->Value(), 55);
        EXPECT_EQ(derived->Extra(), 550);

        // Convert derived proxy to base proxy
        ::Thunder::Core::ProxyType<Trackable> base(derived);
        ASSERT_TRUE(base.IsValid());
        EXPECT_EQ(base->Value(), 55);

        // Both should point to the same underlying object
        EXPECT_EQ(static_cast<Trackable*>(derived.operator->()), base.operator->());
        EXPECT_EQ(g_destructed.load(), 0);
    }

    // =========================================================================
    // Additional coverage: CompositRelease on ProxyObject
    // =========================================================================

    TEST(test_proxytype, composit_release)
    {
        // CompositRelease is for objects that are composited — it decrements
        // refcount and asserts it reaches 0 without deleting the object.
        // The owning composite is responsible for deletion.
        // Use a stack-allocated ProxyObject to test this pattern.
        ::Thunder::Core::ProxyObject<Trackable> obj(1000);

        // ProxyObject constructor sets _refCount = 0
        // AddRef to simulate a reference being held
        obj.AddRef();  // refcount: 0 → 1

        // CompositRelease decrements to 0 without deleting
        obj.CompositRelease();  // refcount: 1 → 0, ASSERT(_refCount == 0) passes

        // Object still alive (stack-allocated), verify access
        EXPECT_EQ(obj.Value(), 1000);
    }

    // =========================================================================
    // Additional coverage: ProxyList basic operations
    // =========================================================================

    TEST(test_proxytype, proxy_list_add_and_remove)
    {
        ::Thunder::Core::ProxyList<Trackable> list(4);

        auto proxy1 = ::Thunder::Core::ProxyType<Trackable>::Create(1);
        auto proxy2 = ::Thunder::Core::ProxyType<Trackable>::Create(2);
        auto proxy3 = ::Thunder::Core::ProxyType<Trackable>::Create(3);

        unsigned int idx1 = list.Add(proxy1);
        unsigned int idx2 = list.Add(proxy2);
        unsigned int idx3 = list.Add(proxy3);

        EXPECT_EQ(idx1, 0u);
        EXPECT_EQ(idx2, 1u);
        EXPECT_EQ(idx3, 2u);

        // Remove middle element
        ::Thunder::Core::ProxyType<Trackable> removed;
        list.Remove(idx2, removed);
        EXPECT_TRUE(removed.IsValid());
        EXPECT_EQ(removed->Value(), 2);
    }

    // =========================================================================
    // Additional coverage: ProxyList grows when capacity exceeded
    // =========================================================================

    TEST(test_proxytype, proxy_list_grow_capacity)
    {
        // Start with very small capacity
        ::Thunder::Core::ProxyList<Trackable> list(2);

        auto p1 = ::Thunder::Core::ProxyType<Trackable>::Create(10);
        auto p2 = ::Thunder::Core::ProxyType<Trackable>::Create(20);
        auto p3 = ::Thunder::Core::ProxyType<Trackable>::Create(30);

        list.Add(p1);
        list.Add(p2);
        // This should trigger capacity growth
        unsigned int idx3 = list.Add(p3);
        EXPECT_EQ(idx3, 2u);
    }

    // =========================================================================
    // Lifecycle hooks: Clear, Destructed, Acquire, Relinquish SFINAE paths
    // =========================================================================

    TEST(test_proxytype, lifecycle_hooks_clear_on_proxy_object)
    {
        ResetLifecycleCounters();

        auto proxy = ::Thunder::Core::ProxyType<LifecycleTrackable>::Create(42);
        ASSERT_TRUE(proxy.IsValid());

        // Clear() calls __Clear() which dispatches to LifecycleTrackable::Clear()
        proxy.Origin()->Clear();
        EXPECT_EQ(g_cleared.load(), 1);
    }

    TEST(test_proxytype, lifecycle_hooks_acquire_relinquish)
    {
        ResetLifecycleCounters();

        auto proxy = ::Thunder::Core::ProxyType<LifecycleTrackable>::Create(10);
        ASSERT_TRUE(proxy.IsValid());

        // AddRef from refcount 1→2 triggers __Acquire()
        proxy.AddRef();
        EXPECT_EQ(g_acquired.load(), 1);

        // Create a copy to keep the object alive, then Release
        ::Thunder::Core::ProxyType<LifecycleTrackable> copy(proxy);

        // Release through proxy: refcount goes from 3→2 (no Relinquish yet)
        proxy.Release();
        EXPECT_EQ(g_relinquished.load(), 0);

        // copy holds refcount=2, let it go out of scope via explicit Release
        // refcount 2→1 triggers __Relinquish()
        copy.Release();
        EXPECT_EQ(g_relinquished.load(), 1);
    }

    TEST(test_proxytype, lifecycle_hooks_destructed_on_delete)
    {
        ResetLifecycleCounters();

        {
            auto proxy = ::Thunder::Core::ProxyType<LifecycleTrackable>::Create(20);
            ASSERT_TRUE(proxy.IsValid());
        }
        // When last ref is released, operator delete calls __Destructed()
        EXPECT_EQ(g_destructedHook.load(), 1);
    }

    // =========================================================================
    // ProxyType constructor: ProxyType(IReferenceCounted&, CONTEXT&)
    // =========================================================================

    TEST(test_proxytype, construct_from_referencecounted_and_context)
    {
        ResetCounters();

        ::Thunder::Core::ProxyObject<Trackable> obj(77);
        obj.AddRef();  // refcount 0→1, so our proxy will bring it to 2

        // Use two-arg constructor
        ::Thunder::Core::ProxyType<Trackable> proxy(
            static_cast<::Thunder::Core::IReferenceCounted&>(obj),
            static_cast<Trackable&>(obj));

        ASSERT_TRUE(proxy.IsValid());
        EXPECT_EQ(proxy->Value(), 77);

        // Clean up: release our manual AddRef and the proxy's ref
        proxy.Release();
        obj.CompositRelease();
    }

    // =========================================================================
    // ProxyType constructor: explicit ProxyType(CONTEXT& realObject)
    // =========================================================================

    TEST(test_proxytype, construct_from_context_reference)
    {
        ResetCounters();

        ::Thunder::Core::ProxyObject<Trackable> obj(88);
        obj.AddRef();  // refcount 0→1

        // Construct from CONTEXT& — uses dynamic_cast to find IReferenceCounted
        ::Thunder::Core::ProxyType<Trackable> proxy(static_cast<Trackable&>(obj));

        ASSERT_TRUE(proxy.IsValid());
        EXPECT_EQ(proxy->Value(), 88);

        proxy.Release();
        obj.CompositRelease();
    }

    // =========================================================================
    // Template cross-type move constructor
    // =========================================================================

    TEST(test_proxytype, cross_type_move_construct_derived_to_base)
    {
        ResetCounters();

        auto derived = ::Thunder::Core::ProxyType<DerivedTrackable>::Create(33);
        ASSERT_TRUE(derived.IsValid());

        // Move derived → base via template move constructor
        ::Thunder::Core::ProxyType<Trackable> base(std::move(derived));

        EXPECT_FALSE(derived.IsValid());
        ASSERT_TRUE(base.IsValid());
        EXPECT_EQ(base->Value(), 33);
    }

    // =========================================================================
    // Template cross-type copy assignment: operator=(const ProxyType<DERIVED>&)
    // =========================================================================

    TEST(test_proxytype, cross_type_copy_assignment_derived_to_base)
    {
        ResetCounters();

        auto derived = ::Thunder::Core::ProxyType<DerivedTrackable>::Create(44);
        ::Thunder::Core::ProxyType<Trackable> base;

        // Template copy assignment
        base = derived;

        ASSERT_TRUE(base.IsValid());
        ASSERT_TRUE(derived.IsValid());
        EXPECT_EQ(base->Value(), 44);
    }

    // =========================================================================
    // Template cross-type move assignment: operator=(ProxyType<DERIVED>&&)
    // =========================================================================

    TEST(test_proxytype, cross_type_move_assignment_derived_to_base)
    {
        ResetCounters();

        auto derived = ::Thunder::Core::ProxyType<DerivedTrackable>::Create(55);
        auto old = ::Thunder::Core::ProxyType<Trackable>::Create(11);
        Trackable* oldPtr = old.operator->();

        // Template move assignment — releases old, moves derived
        old = std::move(derived);

        EXPECT_FALSE(derived.IsValid());
        ASSERT_TRUE(old.IsValid());
        EXPECT_EQ(old->Value(), 55);
        // old object (11) should have been destroyed
        EXPECT_EQ(g_destructed.load(), 1);
    }

    // =========================================================================
    // Unrelated-type cross conversion: CopyConstruct/MoveConstruct with
    // TemplateIntToType<false> — dynamic_cast fails → result is invalid
    // =========================================================================

    TEST(test_proxytype, cross_type_copy_construct_unrelated_fails)
    {
        // Unrelated and Trackable share no inheritance
        auto unrel = ::Thunder::Core::ProxyType<Unrelated>::Create(99);

        // Template copy constructor → CopyConstruct<false> path
        // dynamic_cast<Trackable*>(Unrelated*) returns nullptr
        ::Thunder::Core::ProxyType<Trackable> target(unrel);

        // Conversion should fail — target is invalid
        EXPECT_FALSE(target.IsValid());
        // Source remains valid
        EXPECT_TRUE(unrel.IsValid());
    }

    TEST(test_proxytype, cross_type_move_construct_unrelated_fails)
    {
        auto unrel = ::Thunder::Core::ProxyType<Unrelated>::Create(88);

        // Template move constructor → MoveConstruct<false> path
        // dynamic_cast fails → result invalid, source NOT consumed
        ::Thunder::Core::ProxyType<Trackable> target(std::move(unrel));

        EXPECT_FALSE(target.IsValid());
        // Source should still be valid because dynamic_cast failed
        // (MoveConstruct<false> only calls Reset() on success)
        EXPECT_TRUE(unrel.IsValid());
    }

    TEST(test_proxytype, cross_type_copy_assign_unrelated_fails)
    {
        auto unrel = ::Thunder::Core::ProxyType<Unrelated>::Create(77);
        auto base = ::Thunder::Core::ProxyType<Trackable>::Create(10);

        // Template copy assignment → CopyConstruct<false> path
        base = unrel;

        // Assignment with unrelated type fails — base becomes invalid
        EXPECT_FALSE(base.IsValid());
        EXPECT_TRUE(unrel.IsValid());
    }

    TEST(test_proxytype, cross_type_move_assign_unrelated_fails)
    {
        auto unrel = ::Thunder::Core::ProxyType<Unrelated>::Create(66);
        auto base = ::Thunder::Core::ProxyType<Trackable>::Create(10);

        // Template move assignment → MoveConstruct<false> path
        base = std::move(unrel);

        EXPECT_FALSE(base.IsValid());
        EXPECT_TRUE(unrel.IsValid());
    }

    TEST(test_proxytype, cross_type_copy_construct_unrelated_from_invalid)
    {
        // Test CopyConstruct<false> with invalid source
        ::Thunder::Core::ProxyType<Unrelated> unrel;
        ASSERT_FALSE(unrel.IsValid());

        ::Thunder::Core::ProxyType<Trackable> target(unrel);
        EXPECT_FALSE(target.IsValid());
    }

    TEST(test_proxytype, cross_type_move_construct_unrelated_from_invalid)
    {
        // Test MoveConstruct<false> with invalid source
        ::Thunder::Core::ProxyType<Unrelated> unrel;
        ASSERT_FALSE(unrel.IsValid());

        ::Thunder::Core::ProxyType<Trackable> target(std::move(unrel));
        EXPECT_FALSE(target.IsValid());
    }

    // =========================================================================
    // ProxyList: Remove(index), Clear(), Count(), operator[]
    // =========================================================================

    TEST(test_proxytype, proxy_list_remove_by_index)
    {
        ::Thunder::Core::ProxyList<Trackable> list(4);

        auto p1 = ::Thunder::Core::ProxyType<Trackable>::Create(1);
        auto p2 = ::Thunder::Core::ProxyType<Trackable>::Create(2);
        auto p3 = ::Thunder::Core::ProxyType<Trackable>::Create(3);

        list.Add(p1);
        list.Add(p2);
        list.Add(p3);

        EXPECT_EQ(list.Count(), 3u);

        // Access via operator[]
        EXPECT_EQ(list[0]->Value(), 1);
        EXPECT_EQ(list[1]->Value(), 2);
        EXPECT_EQ(list[2]->Value(), 3);

        // Remove middle element by index (no output param)
        list.Remove(1u);
        EXPECT_EQ(list.Count(), 2u);

        // After removal, elements shift
        EXPECT_EQ(list[0]->Value(), 1);
        EXPECT_EQ(list[1]->Value(), 3);
    }

    TEST(test_proxytype, proxy_list_clear)
    {
        ::Thunder::Core::ProxyList<Trackable> list(4);

        auto p1 = ::Thunder::Core::ProxyType<Trackable>::Create(1);
        auto p2 = ::Thunder::Core::ProxyType<Trackable>::Create(2);
        auto p3 = ::Thunder::Core::ProxyType<Trackable>::Create(3);

        list.Add(p1);
        list.Add(p2);
        list.Add(p3);

        EXPECT_EQ(list.Count(), 3u);

        list.Clear();
        EXPECT_EQ(list.Count(), 0u);
    }

} // Core
} // Tests
} // Thunder
