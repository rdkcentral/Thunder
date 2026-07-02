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

#include <atomic>
#include <thread>
#include <vector>

namespace Thunder {
namespace Tests {
namespace Core {

    // Test callback that tracks Destructed() calls
    class TestCallback : public ::Thunder::Core::ServiceAdministrator::ICallback {
    public:
        TestCallback() : _destructedCount(0) {}
        ~TestCallback() override = default;

        void Destructed() override {
            _destructedCount++;
        }

        uint32_t DestructedCount() const { return _destructedCount; }

    private:
        uint32_t _destructedCount;
    };

    // Simple base class for SinkType tests
    class SimpleSink {
    public:
        SimpleSink() = default;
        virtual ~SimpleSink() = default;
    };

    // Mock IMetadata for Instantiate tests
    class MockMetadata : public ::Thunder::Core::IService::IMetadata {
    public:
        MockMetadata(const TCHAR* name, uint8_t major, uint8_t minor)
            : _name(name), _major(major), _minor(minor) {}

        const TCHAR* Name() const override { return _name; }
        const TCHAR* Module() const override { return _T("TestModule"); }
        uint8_t Major() const override { return _major; }
        uint8_t Minor() const override { return _minor; }
        uint8_t Patch() const override { return 0; }

    private:
        const TCHAR* _name;
        uint8_t _major;
        uint8_t _minor;
    };

    // Mock IService for Instantiate tests
    class MockService : public ::Thunder::Core::IService {
    public:
        MockService(const TCHAR* name, uint8_t major, uint8_t minor, void* createResult, const ::Thunder::Core::IService* next = nullptr)
            : _metadata(name, major, minor), _createResult(createResult), _next(next) {}

        void* Create(const uint32_t /*interfaceNumber*/) const override { return _createResult; }
        const IMetadata* Info() const override { return &_metadata; }
        const ::Thunder::Core::IService* Next() const override { return _next; }

    private:
        MockMetadata _metadata;
        void* _createResult;
        const ::Thunder::Core::IService* _next;
    };

    TEST(Core_ServiceAdministrator, SingletonInstance)
    {
        // ServiceAdministrator::Instance() should return the same object each time
        auto& instance1 = ::Thunder::Core::ServiceAdministrator::Instance();
        auto& instance2 = ::Thunder::Core::ServiceAdministrator::Instance();
        EXPECT_EQ(&instance1, &instance2);
    }

    TEST(Core_ServiceAdministrator, CreatedAndDroppedTracking)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();
        uint32_t baseline = admin.Instances();

        // Created increments instance count
        admin.Created();
        EXPECT_EQ(admin.Instances(), baseline + 1);

        admin.Created();
        EXPECT_EQ(admin.Instances(), baseline + 2);

        // Dropped decrements instance count
        admin.Dropped();
        EXPECT_EQ(admin.Instances(), baseline + 1);

        admin.Dropped();
        EXPECT_EQ(admin.Instances(), baseline);
    }

    TEST(Core_ServiceAdministrator, CallbackNotification)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        TestCallback callback;
        admin.Callback(&callback);

        // Create some instances
        admin.Created();
        admin.Created();

        // Dropping should trigger the callback
        admin.Dropped();
        EXPECT_EQ(callback.DestructedCount(), 1u);

        admin.Dropped();
        EXPECT_EQ(callback.DestructedCount(), 2u);

        // Unregister callback
        admin.Callback(nullptr);

        // Further drops should not trigger callback
        admin.Created();
        admin.Dropped();
        EXPECT_EQ(callback.DestructedCount(), 2u);
    }

    TEST(Core_ServiceAdministrator, FlushLibrariesWhenEmpty)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        // Flushing with no unreferenced libraries should be safe (no-op)
        admin.FlushLibraries();
    }

    TEST(Core_ServiceAdministrator, ReleaseAndFlushLibrary)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        // Load a real library, then release it to the administrator
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        // Move the library to the administrator's unreferenced list
        admin.ReleaseLibrary(std::move(lib));
        EXPECT_FALSE(lib.IsLoaded()); // moved-from

        // Flush should process the unreferenced library
        admin.FlushLibraries();

        // Flushing again should be a no-op
        admin.FlushLibraries();
    }

    TEST(Core_ServiceAdministrator, ConcurrentCreatedDropped)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();
        uint32_t baseline = admin.Instances();

        const uint32_t numThreads = 4;
        const uint32_t opsPerThread = 100;

        // Each thread does Created() then Dropped() — net effect is zero
        std::vector<std::thread> threads;
        for (uint32_t t = 0; t < numThreads; t++) {
            threads.emplace_back([&]() {
                for (uint32_t i = 0; i < opsPerThread; i++) {
                    admin.Created();
                    admin.Dropped();
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }

        EXPECT_EQ(admin.Instances(), baseline);
    }

    TEST(Core_ServiceAdministrator, CallbackSetAndClear)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        TestCallback callback;
        admin.Callback(&callback);

        // Verify callback receives notifications
        admin.Created();
        admin.Dropped();
        EXPECT_GE(callback.DestructedCount(), 1u);

        // Clear callback
        admin.Callback(nullptr);
    }

    TEST(Core_ServiceAdministrator, MultipleReleaseAndFlush)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        // Release multiple libraries then flush them all
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif

        // Release 3 copies
        for (int i = 0; i < 3; i++) {
            ::Thunder::Core::Library lib(file.c_str());
            ASSERT_TRUE(lib.IsLoaded());
            admin.ReleaseLibrary(std::move(lib));
        }

        // Flush should process all 3
        admin.FlushLibraries();
    }

    // =========================================================================
    // SinkType tests — closes gap: Core ServiceAdministrator shutdown notification
    // (v2.1 gap: "ServiceAdministrator — instance tracking, shutdown notification")
    // =========================================================================

    TEST(Core_ServiceAdministrator, SinkTypeAddRefRelease)
    {
        ::Thunder::Core::SinkType<SimpleSink> sink;

        // AddRef returns ERROR_COMPOSIT_OBJECT (composit objects are not ref-counted for lifetime)
        EXPECT_EQ(sink.AddRef(), ::Thunder::Core::ERROR_COMPOSIT_OBJECT);
        EXPECT_EQ(sink.AddRef(), ::Thunder::Core::ERROR_COMPOSIT_OBJECT);

        // Release also returns ERROR_COMPOSIT_OBJECT
        EXPECT_EQ(sink.Release(), ::Thunder::Core::ERROR_COMPOSIT_OBJECT);
        EXPECT_EQ(sink.Release(), ::Thunder::Core::ERROR_COMPOSIT_OBJECT);
    }

    TEST(Core_ServiceAdministrator, SinkTypeWaitReleasedImmediate)
    {
        ::Thunder::Core::SinkType<SimpleSink> sink;

        // No references held → WaitReleased returns immediately
        EXPECT_EQ(sink.WaitReleased(1000), ::Thunder::Core::ERROR_NONE);
    }

    TEST(Core_ServiceAdministrator, SinkTypeWaitReleasedTimeout)
    {
        ::Thunder::Core::SinkType<SimpleSink> sink;

        // Add a reference so WaitReleased will time out
        sink.AddRef();
        EXPECT_EQ(sink.WaitReleased(100), ::Thunder::Core::ERROR_TIMEDOUT);

        // Release the reference so destructor is clean
        sink.Release();
        EXPECT_EQ(sink.WaitReleased(1000), ::Thunder::Core::ERROR_NONE);
    }

    TEST(Core_ServiceAdministrator, SinkTypeWaitReleasedFromThread)
    {
        ::Thunder::Core::SinkType<SimpleSink> sink;

        sink.AddRef();

        // Release from another thread while main thread waits
        std::thread releaser([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            sink.Release();
        });

        EXPECT_EQ(sink.WaitReleased(5000), ::Thunder::Core::ERROR_NONE);
        releaser.join();
    }

    // =========================================================================
    // Instantiate tests — closes gap: Core ServiceAdministrator
    // (v2.1 gap: "ServiceAdministrator — instance tracking, shutdown notification")
    // =========================================================================

    TEST(Core_ServiceAdministrator, InstantiateFindsMatchingService)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        int dummy = 42;
        MockService service(_T("TestService"), 1, 0, &dummy);

        // Version matches: (major << 8) | minor
        void* result = admin.Instantiate(&service, _T("TestService"), (1 << 8) | 0, 0);
        EXPECT_EQ(result, &dummy);
    }

    TEST(Core_ServiceAdministrator, InstantiateNameNotFound)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        int dummy = 42;
        MockService service(_T("TestService"), 1, 0, &dummy);

        // Name mismatch → should return nullptr
        void* result = admin.Instantiate(&service, _T("NonExistent"), (1 << 8) | 0, 0);
        EXPECT_EQ(result, nullptr);
    }

    TEST(Core_ServiceAdministrator, InstantiateVersionMismatch)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        int dummy = 42;
        MockService service(_T("TestService"), 1, 0, &dummy);

        // Version mismatch → should return nullptr
        void* result = admin.Instantiate(&service, _T("TestService"), (2 << 8) | 0, 0);
        EXPECT_EQ(result, nullptr);
    }

    TEST(Core_ServiceAdministrator, InstantiateWildcardVersion)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        int dummy = 42;
        MockService service(_T("TestService"), 3, 5, &dummy);

        // ~0 means any version
        void* result = admin.Instantiate(&service, _T("TestService"), static_cast<uint32_t>(~0), 0);
        EXPECT_EQ(result, &dummy);
    }

    TEST(Core_ServiceAdministrator, InstantiateTraversesLinkedList)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        int dummy1 = 1, dummy2 = 2;
        MockService service2(_T("SecondService"), 1, 0, &dummy2);
        MockService service1(_T("FirstService"), 1, 0, &dummy1, &service2);

        // Should skip service1 and find service2
        void* result = admin.Instantiate(&service1, _T("SecondService"), (1 << 8) | 0, 0);
        EXPECT_EQ(result, &dummy2);
    }

    TEST(Core_ServiceAdministrator, InstantiateNullStartPoint)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        // nullptr start point → should return nullptr
        void* result = admin.Instantiate(nullptr, _T("Anything"), 0, 0);
        EXPECT_EQ(result, nullptr);
    }

    TEST(Core_ServiceAdministrator, InstantiateCreateReturnsNull)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();

        // Service found by name/version but Create() returns nullptr
        MockService service(_T("TestService"), 1, 0, nullptr);

        void* result = admin.Instantiate(&service, _T("TestService"), (1 << 8) | 0, 42);
        EXPECT_EQ(result, nullptr);
    }

    TEST(Core_ServiceAdministrator, LibraryToServiceNoExport)
    {
        // helloworld.so doesn't export GetModuleServices → should return nullptr
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        const ::Thunder::Core::IService* service = ::Thunder::Core::ServiceAdministrator::LibraryToService(lib);
        EXPECT_EQ(service, nullptr);
    }

    // =========================================================================
    // ServiceType tests — closes gap: Core ServiceAdministrator
    // =========================================================================

    // Interface for ServiceType testing
    struct ITestCompute : virtual public ::Thunder::Core::IUnknown {
        enum { ID = ::Thunder::Core::IUnknown::ID_OFFSET_CUSTOM + 0x0099 };
        virtual uint32_t Compute() const = 0;
    };

    // Concrete service implementing ITestCompute
    class TestComputeService : public ITestCompute {
    public:
        TestComputeService() : _value(42) {}
        ~TestComputeService() override = default;

        uint32_t Compute() const override { return _value; }

        BEGIN_INTERFACE_MAP(TestComputeService)
            INTERFACE_ENTRY(ITestCompute)
        END_INTERFACE_MAP

    private:
        uint32_t _value;
    };

    TEST(Core_ServiceAdministrator, ServiceTypeCreateSameType)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();
        uint32_t baseline = admin.Instances();

        // Create with ACTUALSERVICE == INTERFACE → exercises Extract(TemplateIntToType<true>)
        TestComputeService* svc = ::Thunder::Core::ServiceType<TestComputeService>::Create<TestComputeService>();
        ASSERT_NE(svc, nullptr);
        EXPECT_EQ(svc->Compute(), 42u);

        // ServiceType constructor calls Created()
        EXPECT_EQ(admin.Instances(), baseline + 1);

        // Release triggers destructor → Dropped() and Destructed() → ReleaseLibrary()
        svc->Release();

        EXPECT_EQ(admin.Instances(), baseline);
    }

    TEST(Core_ServiceAdministrator, ServiceTypeCreateDifferentType)
    {
        auto& admin = ::Thunder::Core::ServiceAdministrator::Instance();
        uint32_t baseline = admin.Instances();

        // Create with ACTUALSERVICE != INTERFACE → exercises Extract(TemplateIntToType<false>) via QueryInterface
        ITestCompute* iface = ::Thunder::Core::ServiceType<TestComputeService>::Create<ITestCompute>();
        ASSERT_NE(iface, nullptr);
        EXPECT_EQ(iface->Compute(), 42u);

        EXPECT_EQ(admin.Instances(), baseline + 1);

        iface->Release();

        EXPECT_EQ(admin.Instances(), baseline);
    }

} // Core
} // Tests
} // Thunder
