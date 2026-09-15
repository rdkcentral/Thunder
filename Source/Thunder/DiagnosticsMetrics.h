#pragma once

#include "DiagnosticsTypes.h"

#include <map>
#include <mutex>
#include <vector>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

/**
 * @brief Thread-safe fixed-capacity latency reservoir.
 *
 * The reservoir retains at most its configured number of samples. Once full,
 * replacement uses a deterministic round-robin slot. Percentiles are therefore
 * bounded approximations and snapshots sort a copy of at most that capacity.
 */
class LatencyReservoir {
public:
    explicit LatencyReservoir(const uint32_t capacity);

    void Add(const uint64_t durationMs);
    void Reset();
    uint64_t Percentile(const uint8_t percentile) const;

private:
    const uint32_t _capacity;
    uint64_t _next;
    std::vector<uint64_t> _samples;
};

/**
 * @brief Bounded, synchronized counter and latency aggregate.
 */
class MetricRecord {
public:
    explicit MetricRecord(const uint32_t percentileSampleCapacity);

    void Complete(const Outcome outcome, const uint64_t durationMs, const bool slow);
    void Reset();
    MetricSnapshot Snapshot() const;

private:
    mutable std::mutex _lock;
    LatencyReservoir _latencies;
    uint64_t _total;
    uint64_t _success;
    uint64_t _error;
    uint64_t _timeout;
    uint64_t _slow;
    uint64_t _dropped;
    uint64_t _minimum;
    uint64_t _maximum;
    uint64_t _latencyTotal;
    uint64_t _latencyCount;
};

/**
 * @brief Runtime/plugin/method metric hierarchy with an explicit other bucket.
 */
class MetricsStore {
public:
    MetricsStore(const uint32_t maxMethodsPerPlugin, const uint32_t percentileSampleCapacity);

    void Complete(const std::string& callsign, const std::string& method, const Outcome outcome,
        const uint64_t durationMs, const bool slow);
    MetricSnapshot Runtime() const;
    MetricSnapshot Plugin(const std::string& callsign) const;
    MetricSnapshot Method(const std::string& callsign, const std::string& method) const;
    void Reset();

private:
    struct PluginRecords {
        explicit PluginRecords(const uint32_t capacity)
            : aggregate(capacity)
            , other(capacity)
        {
        }

        MetricRecord aggregate;
        MetricRecord other;
        std::map<std::string, MetricRecord> methods;
        uint64_t cardinalityDrops { 0 };
    };

    MetricRecord& MethodRecord(PluginRecords& plugin, const std::string& method);

    const uint32_t _maxMethodsPerPlugin;
    const uint32_t _percentileSampleCapacity;
    mutable std::mutex _lock;
    MetricRecord _runtime;
    std::map<std::string, PluginRecords> _plugins;
};

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder
