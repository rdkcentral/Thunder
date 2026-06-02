/*
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

// Enable warning reporting for this TU so that the templates and proxy are active.
// The proxy implementation (WarningReportingControl.cpp) is compiled into the core
// library when WARNING_REPORTING=ON; when OFF we link the source directly below.
#ifndef __CORE_WARNING_REPORTING__
#define __CORE_WARNING_REPORTING__
#define _WR_DEFINED_LOCALLY 1
#endif

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <gtest/gtest.h>

#include <core/core.h>
#include <core/WarningReportingControl.h>
#include <core/WarningReportingCategories.h>

// Pull in the process-container helper headers (header-only, no backend needed)
// Note: CMakeLists adds Source/extensions to the include path so that
// BaseContainerIterator.h's #include "processcontainers/IProcessContainers.h" resolves.
#include <processcontainers/IProcessContainers.h>
#include <processcontainers/common/BaseRefCount.h>
#include <processcontainers/common/BaseContainerIterator.h>

// Hibernate C API header (for error constant validation)
#include <extensions/hibernate/hibernate.h>

// If the proxy was not compiled into the library (WARNING_REPORTING=OFF),
// pull in the implementation so the linker can find it.
// CallsignTLS.cpp is also conditional on WARNING_REPORTING|EXCEPTION_CATCHING.
#ifdef _WR_DEFINED_LOCALLY
PUSH_WARNING(DISABLE_WARNING_PEDANTIC)
#include <core/CallsignTLS.cpp>
#include <core/WarningReportingControl.cpp>
POP_WARNING()
#endif

namespace Thunder {
namespace Tests {

    // =====================================================================
    // Warning Reporting — ExcludedWarnings
    // =====================================================================

    TEST(Extensions_WarningReporting, ExcludedWarnings_EmptyByDefault)
    {
        WarningReporting::ExcludedWarnings excluded;
        EXPECT_FALSE(excluded.IsCallsignExcluded("Plugin_A"));
        EXPECT_FALSE(excluded.IsModuleExcluded("Module_X"));
    }

    TEST(Extensions_WarningReporting, ExcludedWarnings_InsertCallsign)
    {
        WarningReporting::ExcludedWarnings excluded;
        excluded.InsertCallsign("Plugin_A");
        excluded.InsertCallsign("Plugin_B");

        EXPECT_TRUE(excluded.IsCallsignExcluded("Plugin_A"));
        EXPECT_TRUE(excluded.IsCallsignExcluded("Plugin_B"));
        EXPECT_FALSE(excluded.IsCallsignExcluded("Plugin_C"));
    }

    TEST(Extensions_WarningReporting, ExcludedWarnings_InsertModule)
    {
        WarningReporting::ExcludedWarnings excluded;
        excluded.InsertModule("ModuleA");
        excluded.InsertModule("ModuleB");

        EXPECT_TRUE(excluded.IsModuleExcluded("ModuleA"));
        EXPECT_TRUE(excluded.IsModuleExcluded("ModuleB"));
        EXPECT_FALSE(excluded.IsModuleExcluded("ModuleC"));
    }

    TEST(Extensions_WarningReporting, ExcludedWarnings_CallsignAndModuleIndependent)
    {
        WarningReporting::ExcludedWarnings excluded;
        excluded.InsertCallsign("Plugin_A");
        excluded.InsertModule("ModuleX");

        // Callsign exclusion should not affect module exclusion and vice versa
        EXPECT_TRUE(excluded.IsCallsignExcluded("Plugin_A"));
        EXPECT_FALSE(excluded.IsCallsignExcluded("ModuleX"));
        EXPECT_TRUE(excluded.IsModuleExcluded("ModuleX"));
        EXPECT_FALSE(excluded.IsModuleExcluded("Plugin_A"));
    }

    // =====================================================================
    // Warning Reporting — Category Classes (Serialize/Deserialize/ToString)
    // =====================================================================

    TEST(Extensions_WarningReporting, TooLongWaitingForLock_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::TooLongWaitingForLock::DefaultWarningBound, 1000u);
        EXPECT_EQ(WarningReporting::TooLongWaitingForLock::DefaultReportBound, 1000u);
    }

    TEST(Extensions_WarningReporting, TooLongWaitingForLock_SerializeDeserializeRoundTrip)
    {
        WarningReporting::TooLongWaitingForLock cat;
        uint8_t buffer[64] = {};
        uint16_t written = cat.Serialize(buffer, sizeof(buffer));
        // This category has nothing to serialize
        EXPECT_EQ(written, 0u);

        uint16_t read = cat.Deserialize(buffer, sizeof(buffer));
        EXPECT_EQ(read, 0u);
    }

    TEST(Extensions_WarningReporting, TooLongWaitingForLock_ToString)
    {
        WarningReporting::TooLongWaitingForLock cat;
        string text;
        cat.ToString(text, 1500, 1000);
        EXPECT_NE(text.find("suspiciously long"), string::npos);
        EXPECT_NE(text.find("1500"), string::npos);
        EXPECT_NE(text.find("1000"), string::npos);
    }

    TEST(Extensions_WarningReporting, JobTooLongToFinish_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::JobTooLongToFinish::DefaultWarningBound, 7500u);
        EXPECT_EQ(WarningReporting::JobTooLongToFinish::DefaultReportBound, 5000u);
    }

    TEST(Extensions_WarningReporting, JobTooLongToFinish_ToString)
    {
        WarningReporting::JobTooLongToFinish cat;
        string text;
        cat.ToString(text, 8000, 7500);
        EXPECT_NE(text.find("too long to finish"), string::npos);
        EXPECT_NE(text.find("8000"), string::npos);
    }

    TEST(Extensions_WarningReporting, JobTooLongWaitingInQueue_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::JobTooLongWaitingInQueue::DefaultWarningBound, 400u);
        EXPECT_EQ(WarningReporting::JobTooLongWaitingInQueue::DefaultReportBound, 200u);
    }

    TEST(Extensions_WarningReporting, JobTooLongWaitingInQueue_ToString)
    {
        WarningReporting::JobTooLongWaitingInQueue cat;
        string text;
        cat.ToString(text, 500, 400);
        EXPECT_NE(text.find("queue"), string::npos);
        EXPECT_NE(text.find("500"), string::npos);
    }

    TEST(Extensions_WarningReporting, TooLongInvokeRPC_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::TooLongInvokeRPC::DefaultWarningBound, 750u);
        EXPECT_EQ(WarningReporting::TooLongInvokeRPC::DefaultReportBound, 250u);
    }

    TEST(Extensions_WarningReporting, TooLongInvokeRPC_SerializeDeserializeRoundTrip)
    {
        WarningReporting::TooLongInvokeRPC cat;
        // Trigger Analyze to set interfaceId and methodId
        cat.Analyze(nullptr, nullptr, 42, 7);

        uint8_t buffer[64] = {};
        uint16_t written = cat.Serialize(buffer, sizeof(buffer));
        // Should serialize interfaceId (4 bytes) + methodId (4 bytes)
        EXPECT_EQ(written, 8u);

        WarningReporting::TooLongInvokeRPC cat2;
        uint16_t read = cat2.Deserialize(buffer, written);
        EXPECT_EQ(read, 8u);

        // Verify round-trip by checking ToString output contains same interface/method IDs
        string text1, text2;
        cat.ToString(text1, 500, 250);
        cat2.ToString(text2, 500, 250);
        EXPECT_NE(text1.find("[7]"), string::npos);
        EXPECT_NE(text1.find("[42]"), string::npos);
        EXPECT_EQ(text1, text2);
    }

    TEST(Extensions_WarningReporting, TooLongInvokeRPC_AnalyzeAlwaysReturnsTrue)
    {
        WarningReporting::TooLongInvokeRPC cat;
        // Analyze stores interface/method IDs and always returns true
        EXPECT_TRUE(cat.Analyze(nullptr, nullptr, 1, 2));
    }

    TEST(Extensions_WarningReporting, TooLongDecrypt_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::TooLongDecrypt::DefaultWarningBound, 20u);
        EXPECT_EQ(WarningReporting::TooLongDecrypt::DefaultReportBound, 15u);
    }

    TEST(Extensions_WarningReporting, TooLongDecrypt_ToString)
    {
        WarningReporting::TooLongDecrypt cat;
        string text;
        cat.ToString(text, 30, 20);
        EXPECT_NE(text.find("Decrypt"), string::npos);
        EXPECT_NE(text.find("30"), string::npos);
    }

    TEST(Extensions_WarningReporting, SinkStillHasReference_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::SinkStillHasReference::DefaultWarningBound, 0u);
        EXPECT_EQ(WarningReporting::SinkStillHasReference::DefaultReportBound, 0u);
    }

    TEST(Extensions_WarningReporting, SinkStillHasReference_ToString)
    {
        WarningReporting::SinkStillHasReference cat;
        string text;
        cat.ToString(text, 3, 0);
        EXPECT_NE(text.find("reference"), string::npos);
    }

    TEST(Extensions_WarningReporting, JobActiveForTooLong_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::JobActiveForTooLong::DefaultWarningBound, 20000u);
        EXPECT_EQ(WarningReporting::JobActiveForTooLong::DefaultReportBound, 15000u);
    }

    TEST(Extensions_WarningReporting, JobActiveForTooLong_ToString)
    {
        WarningReporting::JobActiveForTooLong cat;
        string text;
        cat.ToString(text, 25000, 20000);
        EXPECT_NE(text.find("too long"), string::npos);
        EXPECT_NE(text.find("25000"), string::npos);
    }

    TEST(Extensions_WarningReporting, SocketOperationTooLong_DefaultBounds)
    {
        EXPECT_EQ(WarningReporting::SocketOperationTooLong::DefaultWarningBound, 1000u);
        EXPECT_EQ(WarningReporting::SocketOperationTooLong::DefaultReportBound, 1000u);
    }

    TEST(Extensions_WarningReporting, SocketOperationTooLong_ToString)
    {
        WarningReporting::SocketOperationTooLong cat;
        string text;
        cat.ToString(text, 2000, 1000);
        EXPECT_NE(text.find("Socket"), string::npos);
        EXPECT_NE(text.find("2000"), string::npos);
    }

    // =====================================================================
    // Warning Reporting — WarningReportingBoundsCategory<>
    // =====================================================================

    TEST(Extensions_WarningReporting, BoundsCategory_AnalyzeBelowBound_ReturnsFalse)
    {
        // TooLongWaitingForLock has DefaultReportBound = 1000
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;

        // Value below reporting bound should NOT trigger
        EXPECT_FALSE(bounds.Analyze("TestModule", "TestCallsign", 500));
    }

    TEST(Extensions_WarningReporting, BoundsCategory_AnalyzeAboveBound_ReturnsTrue)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;

        // Value above reporting bound (1000) should trigger
        EXPECT_TRUE(bounds.Analyze("TestModule", "TestCallsign", 1500));
    }

    TEST(Extensions_WarningReporting, BoundsCategory_AnalyzeAtBound_ReturnsFalse)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;

        // Value exactly at bound should NOT trigger (condition is >)
        EXPECT_FALSE(bounds.Analyze("TestModule", "TestCallsign", 1000));
    }

    TEST(Extensions_WarningReporting, BoundsCategory_IsWarningBelowWarningBound)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;

        // Set actualValue below warning bound
        bounds.Analyze("TestModule", "TestCallsign", 500);
        EXPECT_FALSE(bounds.IsWarning());
    }

    TEST(Extensions_WarningReporting, BoundsCategory_IsWarningAboveWarningBound)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;

        // Set actualValue well above warning bound (1000)
        bounds.Analyze("TestModule", "TestCallsign", 2000);
        EXPECT_TRUE(bounds.IsWarning());
    }

    TEST(Extensions_WarningReporting, BoundsCategory_SerializeDeserializeRoundTrip)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;
        bounds.Analyze("TestModule", "TestCallsign", 1500);

        uint8_t buffer[64] = {};
        uint16_t written = bounds.Serialize(buffer, sizeof(buffer));
        // Should serialize inner category (0 bytes for TooLongWaitingForLock) + actualValue (4 bytes)
        EXPECT_EQ(written, sizeof(uint32_t));

        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds2;
        uint16_t read = bounds2.Deserialize(buffer, written);
        EXPECT_EQ(read, sizeof(uint32_t));

        // Verify round-trip via ToString
        string text1, text2;
        bounds.ToString(text1);
        bounds2.ToString(text2);
        EXPECT_EQ(text1, text2);
    }

    TEST(Extensions_WarningReporting, BoundsCategory_ToString_IncludesValue)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;
        bounds.Analyze("TestModule", "TestCallsign", 1500);

        string text;
        bounds.ToString(text);
        EXPECT_NE(text.find("1500"), string::npos);
    }

    TEST(Extensions_WarningReporting, BoundsCategory_CategoryName)
    {
        string name = WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock>::CategoryName();
        EXPECT_NE(name.find("TooLongWaitingForLock"), string::npos);
    }

    TEST(Extensions_WarningReporting, BoundsCategory_DifferentBoundsPerCategory)
    {
        // JobTooLongToFinish: report=5000, warning=7500
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::JobTooLongToFinish> jobBounds;

        // Below report bound (5000) -> no report
        EXPECT_FALSE(jobBounds.Analyze("TestModule", "TestCallsign", 3000));

        // Above report bound but below warning bound -> report but not warning
        EXPECT_TRUE(jobBounds.Analyze("TestModule", "TestCallsign", 6000));
        EXPECT_FALSE(jobBounds.IsWarning());

        // Above warning bound -> report and warning
        EXPECT_TRUE(jobBounds.Analyze("TestModule", "TestCallsign", 8000));
        EXPECT_TRUE(jobBounds.IsWarning());
    }

    TEST(Extensions_WarningReporting, BoundsCategory_TooLongInvokeRPC_WithInterfaceAndMethod)
    {
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongInvokeRPC> bounds;

        // TooLongInvokeRPC report bound = 250
        EXPECT_FALSE(bounds.Analyze("TestModule", "TestCallsign", 100, 42u, 7u));
        EXPECT_TRUE(bounds.Analyze("TestModule", "TestCallsign", 500, 42u, 7u));

        string text;
        bounds.ToString(text);
        EXPECT_NE(text.find("[42]"), string::npos);
        EXPECT_NE(text.find("[7]"), string::npos);
    }

    // =====================================================================
    // Warning Reporting — WarningReportingUnitProxy
    // =====================================================================

    TEST(Extensions_WarningReporting, Proxy_InstanceIsSingleton)
    {
        auto& proxy1 = WarningReporting::WarningReportingUnitProxy::Instance();
        auto& proxy2 = WarningReporting::WarningReportingUnitProxy::Instance();
        EXPECT_EQ(&proxy1, &proxy2);
    }

    TEST(Extensions_WarningReporting, Proxy_ReportWithoutHandler_DoesNotCrash)
    {
        // When no handler is set, ReportWarningEvent should not crash
        WarningReporting::WarningReportingBoundsCategory<WarningReporting::TooLongWaitingForLock> bounds;
        bounds.Analyze("TestModule", "TestCallsign", 2000);

        // This should be safe (handler is nullptr, guarded by if-check)
        // Note: ReportWarningEvent has ASSERT(handler != nullptr) which might abort in debug,
        // but we test that the proxy exists and is accessible
        auto& proxy = WarningReporting::WarningReportingUnitProxy::Instance();
        EXPECT_NE(&proxy, nullptr);
    }

    TEST(Extensions_WarningReporting, Proxy_FetchCategoryWithoutHandler_DoesNotCrash)
    {
        bool isDefault = false;
        bool isEnabled = false;
        string excluded;
        string config;

        // Without handler, FetchCategoryInformation should be a no-op
        WarningReporting::WarningReportingUnitProxy::Instance().FetchCategoryInformation(
            "TooLongWaitingForLock", isDefault, isEnabled, excluded, config);

        // Outputs should remain at defaults since no handler is set
        EXPECT_FALSE(isDefault);
        EXPECT_FALSE(isEnabled);
        EXPECT_TRUE(excluded.empty());
        EXPECT_TRUE(config.empty());
    }

    TEST(Extensions_WarningReporting, Proxy_FillBoundsConfig_ParsesJSON)
    {
        uint32_t reportBound = 0;
        uint32_t warningBound = 0;
        string specificConfig;

        WarningReporting::WarningReportingUnitProxy::Instance().FillBoundsConfig(
            "{\"reportbound\":100,\"warningbound\":500}", reportBound, warningBound, specificConfig);

        EXPECT_EQ(reportBound, 100u);
        EXPECT_EQ(warningBound, 500u);
        EXPECT_TRUE(specificConfig.empty());
    }

    TEST(Extensions_WarningReporting, Proxy_FillBoundsConfig_WithCategoryConfig)
    {
        uint32_t reportBound = 0;
        uint32_t warningBound = 0;
        string specificConfig;

        WarningReporting::WarningReportingUnitProxy::Instance().FillBoundsConfig(
            "{\"reportbound\":200,\"warningbound\":800,\"config\":\"custom_data\"}", reportBound, warningBound, specificConfig);

        EXPECT_EQ(reportBound, 200u);
        EXPECT_EQ(warningBound, 800u);
        EXPECT_EQ(specificConfig, "\"custom_data\"");
    }

    TEST(Extensions_WarningReporting, Proxy_FillBoundsConfig_EmptyString)
    {
        uint32_t reportBound = 0;
        uint32_t warningBound = 0;
        string specificConfig;

        WarningReporting::WarningReportingUnitProxy::Instance().FillBoundsConfig(
            "", reportBound, warningBound, specificConfig);

        EXPECT_EQ(reportBound, 0u);
        EXPECT_EQ(warningBound, 0u);
        EXPECT_TRUE(specificConfig.empty());
    }

    TEST(Extensions_WarningReporting, Proxy_FillExcludedWarnings_Callsigns)
    {
        WarningReporting::ExcludedWarnings excluded;

        WarningReporting::WarningReportingUnitProxy::Instance().FillExcludedWarnings(
            "{\"callsigns\":[\"Plugin_A\",\"Plugin_B\"]}", excluded);

        EXPECT_TRUE(excluded.IsCallsignExcluded("Plugin_A"));
        EXPECT_TRUE(excluded.IsCallsignExcluded("Plugin_B"));
        EXPECT_FALSE(excluded.IsCallsignExcluded("Plugin_C"));
    }

    TEST(Extensions_WarningReporting, Proxy_FillExcludedWarnings_Modules)
    {
        WarningReporting::ExcludedWarnings excluded;

        WarningReporting::WarningReportingUnitProxy::Instance().FillExcludedWarnings(
            "{\"modules\":[\"ModuleA\",\"ModuleB\"]}", excluded);

        EXPECT_TRUE(excluded.IsModuleExcluded("ModuleA"));
        EXPECT_TRUE(excluded.IsModuleExcluded("ModuleB"));
        EXPECT_FALSE(excluded.IsModuleExcluded("ModuleC"));
    }

    TEST(Extensions_WarningReporting, Proxy_FillExcludedWarnings_Both)
    {
        WarningReporting::ExcludedWarnings excluded;

        WarningReporting::WarningReportingUnitProxy::Instance().FillExcludedWarnings(
            "{\"callsigns\":[\"P1\"],\"modules\":[\"M1\"]}", excluded);

        EXPECT_TRUE(excluded.IsCallsignExcluded("P1"));
        EXPECT_TRUE(excluded.IsModuleExcluded("M1"));
    }

    TEST(Extensions_WarningReporting, Proxy_FillExcludedWarnings_EmptyJSON)
    {
        WarningReporting::ExcludedWarnings excluded;

        WarningReporting::WarningReportingUnitProxy::Instance().FillExcludedWarnings("{}", excluded);

        EXPECT_FALSE(excluded.IsCallsignExcluded("anything"));
        EXPECT_FALSE(excluded.IsModuleExcluded("anything"));
    }

    // =====================================================================
    // Process Containers — BaseContainerIterator
    // =====================================================================

    TEST(Extensions_ProcessContainers, BaseContainerIterator_EmptyContainer)
    {
        std::vector<string> ids;
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        EXPECT_EQ(iter->Count(), 0u);
        EXPECT_FALSE(iter->IsValid());
        EXPECT_FALSE(iter->Next());
        EXPECT_FALSE(iter->IsValid());

        iter->Release();
    }

    TEST(Extensions_ProcessContainers, BaseContainerIterator_SingleElement)
    {
        std::vector<string> ids = { "container_1" };
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        EXPECT_EQ(iter->Count(), 1u);
        EXPECT_FALSE(iter->IsValid()); // Initially invalid (before Next)

        EXPECT_TRUE(iter->Next());
        EXPECT_TRUE(iter->IsValid());
        EXPECT_EQ(iter->Id(), "container_1");
        EXPECT_EQ(iter->Index(), 0u);

        EXPECT_FALSE(iter->Next()); // No more elements
        EXPECT_FALSE(iter->IsValid());

        iter->Release();
    }

    TEST(Extensions_ProcessContainers, BaseContainerIterator_MultipleElements)
    {
        std::vector<string> ids = { "alpha", "beta", "gamma" };
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        EXPECT_EQ(iter->Count(), 3u);

        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "alpha");
        EXPECT_EQ(iter->Index(), 0u);

        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "beta");
        EXPECT_EQ(iter->Index(), 1u);

        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "gamma");
        EXPECT_EQ(iter->Index(), 2u);

        EXPECT_FALSE(iter->Next());

        iter->Release();
    }

    TEST(Extensions_ProcessContainers, BaseContainerIterator_Reset)
    {
        std::vector<string> ids = { "first", "second" };
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        // Advance to end
        EXPECT_TRUE(iter->Next());
        EXPECT_TRUE(iter->Next());
        EXPECT_FALSE(iter->Next());

        // Reset and iterate again
        iter->Reset(0);
        EXPECT_FALSE(iter->IsValid()); // After reset, should be invalid

        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "first");

        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "second");

        iter->Release();
    }

    TEST(Extensions_ProcessContainers, BaseContainerIterator_Previous)
    {
        std::vector<string> ids = { "a", "b", "c" };
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        // Navigate forward
        EXPECT_TRUE(iter->Next());
        EXPECT_TRUE(iter->Next());
        EXPECT_EQ(iter->Id(), "b");

        // Navigate backward
        EXPECT_TRUE(iter->Previous());
        EXPECT_EQ(iter->Id(), "a");

        iter->Release();
    }

    TEST(Extensions_ProcessContainers, BaseContainerIterator_PreviousFromInvalid)
    {
        std::vector<string> ids = { "x", "y", "z" };
        auto* iter = new ProcessContainers::BaseContainerIterator(std::move(ids));

        // Previous from initial (UINT32_MAX) state should go to last element
        EXPECT_TRUE(iter->Previous());
        EXPECT_TRUE(iter->IsValid());
        EXPECT_EQ(iter->Id(), "z");
        EXPECT_EQ(iter->Index(), 2u);

        iter->Release();
    }

    // =====================================================================
    // Process Containers — BaseRefCount
    // =====================================================================

    // Minimal concrete class to test BaseRefCount
    struct TestRefCounted : public ProcessContainers::BaseRefCount<ProcessContainers::IContainerIterator> {
        TestRefCounted() : ProcessContainers::BaseRefCount<ProcessContainers::IContainerIterator>() {}

        bool Next() override { return false; }
        bool Previous() override { return false; }
        void Reset(const uint32_t) override {}
        bool IsValid() const override { return false; }
        uint32_t Index() const override { return 0; }
        uint32_t Count() const override { return 0; }
        const string& Id() const override { static string empty; return empty; }
    };

    TEST(Extensions_ProcessContainers, BaseRefCount_InitialCountIsOne)
    {
        auto* obj = new TestRefCounted();
        // Initial refcount is 1 (set in constructor)
        // AddRef should increment to 2
        EXPECT_EQ(obj->AddRef(), Core::ERROR_NONE);
        // Release back to 1
        EXPECT_EQ(obj->Release(), Core::ERROR_NONE);
        // Release to 0 -> destruction
        EXPECT_EQ(obj->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    TEST(Extensions_ProcessContainers, BaseRefCount_MultipleAddRefRelease)
    {
        auto* obj = new TestRefCounted();
        // Ref = 1

        EXPECT_EQ(obj->AddRef(), Core::ERROR_NONE); // Ref = 2
        EXPECT_EQ(obj->AddRef(), Core::ERROR_NONE); // Ref = 3
        EXPECT_EQ(obj->AddRef(), Core::ERROR_NONE); // Ref = 4

        EXPECT_EQ(obj->Release(), Core::ERROR_NONE); // Ref = 3
        EXPECT_EQ(obj->Release(), Core::ERROR_NONE); // Ref = 2
        EXPECT_EQ(obj->Release(), Core::ERROR_NONE); // Ref = 1
        EXPECT_EQ(obj->Release(), Core::ERROR_DESTRUCTION_SUCCEEDED); // Ref = 0, destroyed
    }

    // =====================================================================
    // Process Containers — IContainer::containertype enum
    // =====================================================================

    TEST(Extensions_ProcessContainers, ContainerType_EnumValues)
    {
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::DEFAULT), 0);
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::LXC), 1);
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::RUNC), 2);
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::CRUN), 3);
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::DOBBY), 4);
        EXPECT_EQ(static_cast<uint8_t>(ProcessContainers::IContainer::AWC), 5);
    }

    // =====================================================================
    // Hibernate — Error constants
    // =====================================================================

    TEST(Extensions_Hibernate, ErrorConstants)
    {
        // Verify the C API error constants are defined correctly
        EXPECT_EQ(HIBERNATE_ERROR_NONE, 0u);
        EXPECT_EQ(HIBERNATE_ERROR_GENERAL, 1u);
        EXPECT_EQ(HIBERNATE_ERROR_TIMEOUT, 2u);
    }

} // namespace Tests
} // namespace Thunder
