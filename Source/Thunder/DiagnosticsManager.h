#pragma once

#include "DiagnosticsEvents.h"
#include "DiagnosticsMetrics.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

/**
 * @brief Fail-open internal facade for future lifecycle and RPC observers.
 *
 * Observer methods intentionally retain no request payloads, tokens, parameters,
 * or response data. All storage is bounded by the configuration supplied at
 * initialization.
 */
class DiagnosticsManager {
public:
    DiagnosticsManager();
    ~DiagnosticsManager();

    DiagnosticsManager(const DiagnosticsManager&) = delete;
    DiagnosticsManager& operator=(const DiagnosticsManager&) = delete;

    void Initialize(const Configuration& configuration);
    void Shutdown();
    bool Enabled(const ObservabilityLevel minimum) const;

    void OnRpcCompleted(const std::string& callsign, const std::string& method,
        const std::string& correlationId, const Outcome outcome, const uint32_t rawResult,
        const uint64_t durationMs) noexcept;

    MetricSnapshot SnapshotRuntime() const;
    MetricSnapshot SnapshotPlugin(const std::string& callsign) const;
    EventPage SnapshotEvents(const uint64_t afterSequence, const uint32_t limit) const;
    void ResetMetrics();

private:
    mutable std::mutex _lock;
    bool _enabled;
    Configuration _configuration;
    std::unique_ptr<MetricsStore> _metrics;
    std::unique_ptr<EventStore> _events;
    uint64_t _epoch;
};

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder
