/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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
#include <unistd.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // =========================================================================
    // TEST FILE: test_assertionunit.cpp
    //
    // Purpose:
    //   Tests the AssertionUnitProxy routing mechanism (Gap 14).
    //   Verifies that AssertionUnitProxy can register a handler and
    //   route assertion events through it.
    // =========================================================================

    // Custom assertion handler that captures routed events
    class TestAssertionHandler : public ::Thunder::Assertion::IAssertionUnit {
    public:
        TestAssertionHandler()
            : _eventCount(0)
        {
        }
        ~TestAssertionHandler() override = default;

        void AssertionEvent(
            ::Thunder::Core::Messaging::IStore::Assert& /* metadata */,
            const ::Thunder::Core::Messaging::TextMessage& /* message */,
            ::Thunder::Core::Messaging::OutputMode /* outputMode */) override
        {
            _eventCount++;
        }

        int EventCount() const { return _eventCount; }

    private:
        std::atomic<int> _eventCount;
    };

    // AssertionUnitProxy singleton is accessible
    TEST(AssertionUnit, ProxySingletonExists)
    {
        auto& proxy = ::Thunder::Assertion::AssertionUnitProxy::Instance();
        // Should not crash — just verify we can access the singleton
        (void)proxy;
    }

    // Register and unregister a handler without crash
    TEST(AssertionUnit, RegisterHandler)
    {
        auto& proxy = ::Thunder::Assertion::AssertionUnitProxy::Instance();

        TestAssertionHandler handler;

        // Register the handler
        proxy.Handle(&handler);
        EXPECT_EQ(handler.EventCount(), 0);

        // Unregister by passing nullptr
        proxy.Handle(nullptr);
    }

    // Route an assertion event through the proxy
    TEST(AssertionUnit, EventRouting)
    {
        auto& proxy = ::Thunder::Assertion::AssertionUnitProxy::Instance();

        TestAssertionHandler handler;
        proxy.Handle(&handler);

        EXPECT_EQ(handler.EventCount(), 0);

        // Create assertion metadata and message
        ::Thunder::Core::Messaging::Metadata metaBase(
            ::Thunder::Core::Messaging::Metadata::type::ASSERT,
            _T("TestCategory"), _T("TestModule"));
        ::Thunder::Core::Messaging::MessageInfo msgInfo(metaBase);
        ::Thunder::Core::Messaging::IStore::Assert metadata(
            msgInfo, getpid(), _T("test"), _T(__FILE__), __LINE__, _T(""));

        ::Thunder::Core::Messaging::TextMessage message(_T("Test assertion message"));

        // Route through proxy
        proxy.AssertionEvent(metadata, message, ::Thunder::Core::Messaging::OutputMode::HANDLER);

        EXPECT_EQ(handler.EventCount(), 1);

        // Route another
        proxy.AssertionEvent(metadata, message, ::Thunder::Core::Messaging::OutputMode::DIRECT);

        EXPECT_EQ(handler.EventCount(), 2);

        proxy.Handle(nullptr);
    }

    // Without handler, AssertionEvent does not crash
    TEST(AssertionUnit, EventWithoutHandler_NoCrash)
    {
        auto& proxy = ::Thunder::Assertion::AssertionUnitProxy::Instance();

        // Ensure no handler is registered
        proxy.Handle(nullptr);

        ::Thunder::Core::Messaging::Metadata metaBase(
            ::Thunder::Core::Messaging::Metadata::type::ASSERT,
            _T("TestCategory"), _T("TestModule"));
        ::Thunder::Core::Messaging::MessageInfo msgInfo(metaBase);
        ::Thunder::Core::Messaging::IStore::Assert metadata(
            msgInfo, getpid(), _T("test"), _T(__FILE__), __LINE__, _T(""));

        ::Thunder::Core::Messaging::TextMessage message(_T("No handler"));

        // Should not crash
        proxy.AssertionEvent(metadata, message, ::Thunder::Core::Messaging::OutputMode::HANDLER);
    }

} // Core
} // Tests
} // Thunder
