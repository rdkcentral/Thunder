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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include "DiagnosticsEvents.h"
#include "DiagnosticsManager.h"
#include "DiagnosticsMetrics.h"

#include <atomic>
#include <thread>
#include <vector>

namespace Thunder {
namespace Tests {
namespace Diagnostics {

    using namespace PluginHost::Diagnostics;

    TEST(ThunderDiagnostics_Configuration, DefaultConfigurationIsOffAndBounded)
    {
        const Configuration configuration;

        EXPECT_EQ(configuration.level, ObservabilityLevel::OFF);
        EXPECT_EQ(configuration.slowRpcThresholdMs, 0u);
        EXPECT_GT(configuration.eventBufferCapacity, 0u);
        EXPECT_GT(configuration.transactionBufferCapacity, 0u);
        EXPECT_GT(configuration.maxMethodsPerPlugin, 0u);
        EXPECT_GT(configuration.percentileSampleCapacity, 0u);
        EXPECT_FALSE(configuration.enableConnectionMetrics);
        EXPECT_FALSE(configuration.enableResourceMetrics);
        EXPECT_FALSE(configuration.enableCorrelationRecords);
    }

    TEST(ThunderDiagnostics_Manager, OffConfigurationCreatesNoRecords)
    {
        DiagnosticsManager manager;
        const Configuration configuration;

        manager.Initialize(configuration);
        EXPECT_FALSE(manager.Enabled(ObservabilityLevel::BASIC));

        manager.OnRpcCompleted(
            "Controller", "activate", "correlation-id", Outcome::INTERNAL_ERROR, Core::ERROR_GENERAL, 25);

        const MetricSnapshot runtime = manager.SnapshotRuntime();
        const EventPage events = manager.SnapshotEvents(0, 10);

        EXPECT_EQ(runtime.total, 0u);
        EXPECT_FALSE(runtime.hasLatency);
        EXPECT_TRUE(events.events.empty());
        EXPECT_EQ(events.overwritten, 0u);
    }

    TEST(ThunderDiagnostics_Metrics, LatencyReservoirIsBoundedAndMetricsAggregateAllSamples)
    {
        MetricRecord record(2);

        record.Complete(Outcome::SUCCESS, 10, false);
        record.Complete(Outcome::SUCCESS, 20, false);
        record.Complete(Outcome::TIMEOUT, 30, true);

        const MetricSnapshot snapshot = record.Snapshot();

        EXPECT_EQ(snapshot.total, 3u);
        EXPECT_EQ(snapshot.success, 2u);
        EXPECT_EQ(snapshot.error, 1u);
        EXPECT_EQ(snapshot.timeout, 1u);
        EXPECT_EQ(snapshot.slow, 1u);
        EXPECT_TRUE(snapshot.hasLatency);
        EXPECT_EQ(snapshot.minimumLatencyMs, 10u);
        EXPECT_EQ(snapshot.maximumLatencyMs, 30u);
        EXPECT_EQ(snapshot.averageLatencyMs, 20u);

        // The two-slot reservoir retains the latest two samples: 20 and 30.
        EXPECT_EQ(snapshot.percentile95LatencyMs, 20u);
        EXPECT_EQ(snapshot.percentile99LatencyMs, 20u);
    }

    TEST(ThunderDiagnostics_Metrics, MethodCardinalityOverflowUsesOtherBucket)
    {
        MetricsStore store(1, 4);

        store.Complete("Controller", "activate", Outcome::SUCCESS, 1, false);
        store.Complete("Controller", "deactivate", Outcome::SUCCESS, 2, false);
        store.Complete("Controller", "suspend", Outcome::INTERNAL_ERROR, 3, false);

        const MetricSnapshot plugin = store.Plugin("Controller");
        const MetricSnapshot retainedMethod = store.Method("Controller", "activate");
        const MetricSnapshot overflowBucket = store.Method("Controller", "deactivate");

        EXPECT_EQ(plugin.total, 3u);
        EXPECT_EQ(retainedMethod.total, 1u);
        EXPECT_EQ(overflowBucket.total, 2u);
        EXPECT_EQ(overflowBucket.success, 1u);
        EXPECT_EQ(overflowBucket.error, 1u);
    }

    TEST(ThunderDiagnostics_Events, RingWrapAroundRetainsNewestEventsAndReportsOverwrite)
    {
        EventStore events(2);

        EventSnapshot first;
        first.callsign = "Controller";
        first.method = "first";
        events.Add(first);

        EventSnapshot second;
        second.callsign = "Controller";
        second.method = "second";
        events.Add(second);

        EventSnapshot third;
        third.callsign = "Controller";
        third.method = "third";
        events.Add(third);

        const EventPage page = events.Snapshot(0, 10);

        ASSERT_EQ(page.events.size(), 2u);
        EXPECT_EQ(page.events[0].sequence, 2u);
        EXPECT_EQ(page.events[0].method, "second");
        EXPECT_EQ(page.events[1].sequence, 3u);
        EXPECT_EQ(page.events[1].method, "third");
        EXPECT_EQ(page.nextSequence, 3u);
        EXPECT_EQ(page.overwritten, 1u);
        EXPECT_FALSE(page.truncated);
    }

    TEST(ThunderDiagnostics_Manager, ResetClearsMetricsAndEventsWithoutReenablingDisabledCollection)
    {
        DiagnosticsManager manager;
        Configuration configuration;
        configuration.level = ObservabilityLevel::BASIC;

        manager.Initialize(configuration);
        manager.OnRpcCompleted(
            "Controller", "activate", "correlation-id", Outcome::INTERNAL_ERROR, Core::ERROR_GENERAL, 25);

        ASSERT_EQ(manager.SnapshotRuntime().total, 1u);
        ASSERT_EQ(manager.SnapshotEvents(0, 10).events.size(), 1u);

        manager.ResetMetrics();

        EXPECT_EQ(manager.SnapshotRuntime().total, 0u);
        EXPECT_TRUE(manager.SnapshotEvents(0, 10).events.empty());

        manager.Shutdown();
        EXPECT_FALSE(manager.Enabled(ObservabilityLevel::BASIC));
        EXPECT_EQ(manager.SnapshotRuntime().total, 0u);
    }

    TEST(ThunderDiagnostics_Manager, ErrorEventsRetainOnlyDiagnosticMetadata)
    {
        DiagnosticsManager manager;
        Configuration configuration;
        configuration.level = ObservabilityLevel::BASIC;

        manager.Initialize(configuration);
        manager.OnRpcCompleted(
            "Controller", "activate", "opaque-correlation-id", Outcome::INTERNAL_ERROR, Core::ERROR_GENERAL, 25);

        const EventPage page = manager.SnapshotEvents(0, 10);

        ASSERT_EQ(page.events.size(), 1u);
        const EventSnapshot& event = page.events.front();
        EXPECT_EQ(event.callsign, "Controller");
        EXPECT_EQ(event.method, "activate");
        EXPECT_EQ(event.correlationId, "opaque-correlation-id");
        EXPECT_EQ(event.rawResult, Core::ERROR_GENERAL);
        EXPECT_EQ(event.outcome, Outcome::INTERNAL_ERROR);
        EXPECT_EQ(event.durationMs, 25u);

        // EventSnapshot deliberately has no request parameters, payload, response,
        // token, password, or credential field. The manager API accepts none of
        // those values, preventing their retention by the Phase 1 core.
    }

    TEST(ThunderDiagnostics_Manager, ConcurrentSnapshotsUpdatesAndShutdownAreSafe)
    {
        DiagnosticsManager manager;
        Configuration configuration;
        configuration.level = ObservabilityLevel::BASIC;
        configuration.eventBufferCapacity = 16;
        manager.Initialize(configuration);

        constexpr uint32_t updatesPerThread = 100;
        std::atomic<bool> start(false);
        std::atomic<bool> writersFinished(false);

        std::vector<std::thread> writers;
        for (uint32_t index = 0; index < 4; ++index) {
            writers.emplace_back([&manager, &start]() {
                while (start.load() == false) {
                }

                for (uint32_t update = 0; update < updatesPerThread; ++update) {
                    manager.OnRpcCompleted(
                        "Controller", "activate", "correlation-id", Outcome::SUCCESS, Core::ERROR_NONE, update);
                }
            });
        }

        std::thread reader([&manager, &start, &writersFinished]() {
            while (start.load() == false) {
            }

            while (writersFinished.load() == false) {
                const MetricSnapshot runtime = manager.SnapshotRuntime();
                const EventPage events = manager.SnapshotEvents(0, 16);
                EXPECT_LE(events.events.size(), 16u);
                EXPECT_LE(runtime.success, runtime.total);
            }
        });

        start.store(true);
        for (std::thread& writer : writers) {
            writer.join();
        }
        writersFinished.store(true);
        reader.join();

        EXPECT_EQ(manager.SnapshotRuntime().total, 4u * updatesPerThread);

        // A late observer call after shutdown is intentionally ignored.
        manager.Shutdown();
        manager.OnRpcCompleted(
            "Controller", "activate", "late-correlation-id", Outcome::SUCCESS, Core::ERROR_NONE, 1);
        EXPECT_EQ(manager.SnapshotRuntime().total, 0u);
    }

} // namespace Diagnostics
} // namespace Tests
} // namespace Thunder
