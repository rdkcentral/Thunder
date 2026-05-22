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

    TEST(Core_Library, simpleSet)
    {
        ::Thunder::Core::Library libObj;
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so"); // For QEMU
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so"); // For PC
        //const string file =  _T("/usr/lib/libwpe-0.2.so"); //For box.
#endif
        const TCHAR* function = _T("HelloWorld");
        const string file1 = _T("libThunder.so");

        ::Thunder::Core::Library LibObj1(file.c_str());
        EXPECT_TRUE(LibObj1.Error().empty());

#ifndef __APPLE__
        LibObj1.LoadFunction(function);
#endif
        EXPECT_TRUE(LibObj1.IsLoaded());
        EXPECT_TRUE(LibObj1.Error().empty());

        ::Thunder::Core::Library LibObj2(LibObj1); // Copy constructor
        EXPECT_TRUE(LibObj1.Error().empty());
        EXPECT_TRUE(LibObj2.Error().empty());

        ::Thunder::Core::Library LibObj3;
        LibObj3 = LibObj2; // Copy assignment

        EXPECT_TRUE(LibObj2.Error().empty());
        EXPECT_TRUE(LibObj3.Error().empty()); // Same as LibObj2
        EXPECT_EQ(LibObj3.Name(), file);
    }

    // =========================================================================
    // Load failure and lifecycle tests — closes gap: Core Library load failure paths
    // (v2.1 gap: "Library — non-existent .so, missing symbols, unload-while-in-use")
    // =========================================================================

    TEST(Core_Library, LoadNonExistent)
    {
        // Load a non-existent .so → Error() should return a meaningful message
        ::Thunder::Core::Library lib(_T("/tmp/this_library_does_not_exist.so"));
        EXPECT_FALSE(lib.IsLoaded());
        EXPECT_FALSE(lib.Error().empty());
        EXPECT_EQ(lib.Name(), ::Thunder::Core::emptyString);
    }

    TEST(Core_Library, LoadFunctionMissingSymbol)
    {
        // Load valid .so, then look up a symbol that doesn't exist
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());
        EXPECT_TRUE(lib.Error().empty());

        void* fn = lib.LoadFunction(_T("ThisSymbolDoesNotExist"));
        EXPECT_EQ(fn, nullptr);
        EXPECT_FALSE(lib.Error().empty());
    }

    TEST(Core_Library, LoadValidFunction)
    {
        // Load valid .so, look up HelloWorld → should succeed
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        void* fn = lib.LoadFunction(_T("HelloWorld"));
        EXPECT_NE(fn, nullptr);
        EXPECT_TRUE(lib.Error().empty());
    }

    TEST(Core_Library, DefaultConstructorNotLoaded)
    {
        // Default-constructed Library is not loaded
        ::Thunder::Core::Library lib;
        EXPECT_FALSE(lib.IsLoaded());
        EXPECT_EQ(lib.Name(), ::Thunder::Core::emptyString);
    }

    TEST(Core_Library, IsLoadedStateAfterClose)
    {
        // Load valid .so → IsLoaded() true → destroy → verify lifecycle
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());
        EXPECT_EQ(lib.Name(), file);

        // Assign default-constructed (empty) library to drop reference
        lib = ::Thunder::Core::Library();
        EXPECT_FALSE(lib.IsLoaded());
    }

    TEST(Core_Library, MoveConstructor)
    {
        // Move constructor transfers ownership
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib1(file.c_str());
        ASSERT_TRUE(lib1.IsLoaded());

        ::Thunder::Core::Library lib2(std::move(lib1));
        EXPECT_TRUE(lib2.IsLoaded());
        EXPECT_FALSE(lib1.IsLoaded());
        EXPECT_EQ(lib2.Name(), file);
    }

    TEST(Core_Library, MoveAssignment)
    {
        // Move assignment transfers ownership
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib1(file.c_str());
        ASSERT_TRUE(lib1.IsLoaded());

        ::Thunder::Core::Library lib2;
        lib2 = std::move(lib1);
        EXPECT_TRUE(lib2.IsLoaded());
        EXPECT_FALSE(lib1.IsLoaded());
        EXPECT_EQ(lib2.Name(), file);
    }

    TEST(Core_Library, MoveAssignmentSameHandle)
    {
        // Move assignment where both point to the same handle
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib1(file.c_str());
        ASSERT_TRUE(lib1.IsLoaded());

        // Copy so both share the same refcounted handle
        ::Thunder::Core::Library lib2(lib1);
        EXPECT_TRUE(lib2.IsLoaded());

        // Move assign lib1 into lib2 — same handle, should drop one ref
        lib2 = std::move(lib1);
        EXPECT_TRUE(lib2.IsLoaded());
        EXPECT_FALSE(lib1.IsLoaded());
    }

    TEST(Core_Library, CopyAssignmentSelf)
    {
        // Self-assignment should be safe
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        lib = lib;
        EXPECT_TRUE(lib.IsLoaded());
        EXPECT_EQ(lib.Name(), file);
    }

    TEST(Core_Library, CopyAssignmentDifferentLibraries)
    {
        // Copy assignment from one library to another (overwriting)
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib1(file.c_str());
        ASSERT_TRUE(lib1.IsLoaded());

        // lib2 starts as a different state (not loaded)
        ::Thunder::Core::Library lib2;
        EXPECT_FALSE(lib2.IsLoaded());

        lib2 = lib1;
        EXPECT_TRUE(lib2.IsLoaded());
        EXPECT_EQ(lib2.Name(), file);
        EXPECT_TRUE(lib1.IsLoaded());
    }

    TEST(Core_Library, LoadWithNullFilename)
    {
        // Loading with nullptr opens global symbols (on Linux)
        ::Thunder::Core::Library lib(static_cast<const TCHAR*>(nullptr));
        // On Linux, dlopen(nullptr) opens the main program handle — succeeds
        EXPECT_TRUE(lib.IsLoaded());
    }

    TEST(Core_Library, LoadFromFunctionAddress)
    {
        // Construct Library from a function address in a loaded library
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        void* fn = lib.LoadFunction(_T("HelloWorld"));
        ASSERT_NE(fn, nullptr);

        // Construct a new Library from that function's address
        ::Thunder::Core::Library lib2(fn);
        EXPECT_TRUE(lib2.IsLoaded());
    }

    TEST(Core_Library, LoadFromInvalidAddress)
    {
        // Construct Library from an address not in any loaded library
        int dummy = 42;
        ::Thunder::Core::Library lib(reinterpret_cast<const void*>(&dummy));
        // Stack variable address — dladdr may succeed (finds main exe) or fail
        // Either way, should not crash
    }

    TEST(Core_Library, DoubleAssignEmpty)
    {
        // Double assign-empty should not crash
#ifdef BUILD_ARM
        const string file = _T("/usr/lib/testdata/libhelloworld.so");
#else
        const string file = string(BUILD_DIR) + _T("/libhelloworld.so");
#endif
        ::Thunder::Core::Library lib(file.c_str());
        ASSERT_TRUE(lib.IsLoaded());

        // First drop
        lib = ::Thunder::Core::Library();
        EXPECT_FALSE(lib.IsLoaded());

        // Second drop on already-empty library — should be safe
        lib = ::Thunder::Core::Library();
        EXPECT_FALSE(lib.IsLoaded());
    }

    TEST(Core_Library, Iterator)
    {
        // Iterate over loaded libraries
        ::Thunder::Core::Library::Iterator it;
        EXPECT_FALSE(it.IsValid());

        uint32_t count = 0;
        while (it.Next()) {
            EXPECT_TRUE(it.IsValid());
            ::Thunder::Core::Library current = it.Current();
            count++;
            if (count > 500) break; // Safety limit
        }
        EXPECT_GT(count, 0u);
        EXPECT_FALSE(it.IsValid());

        // Reset and iterate again
        it.Reset();
        EXPECT_FALSE(it.IsValid());
        EXPECT_TRUE(it.Next());
        EXPECT_TRUE(it.IsValid());
    }

} // Core
} // Tests
} // Thunder
