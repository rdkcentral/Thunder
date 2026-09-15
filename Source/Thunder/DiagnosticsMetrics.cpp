#include "DiagnosticsMetrics.h"

#include <algorithm>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

LatencyReservoir::LatencyReservoir(const uint32_t capacity)
    : _capacity(std::max<uint32_t>(capacity, 1))
    , _next(0)
    , _samples()
{
    _samples.reserve(_capacity);
}

void LatencyReservoir::Add(const uint64_t durationMs)
{
    if (_samples.size() < _capacity) {
        _samples.emplace_back(durationMs);
    } else {
        _samples[_next % _capacity] = durationMs;
    }
    ++_next;
}

void LatencyReservoir::Reset()
{
    _samples.clear();
    _next = 0;
}

uint64_t LatencyReservoir::Percentile(const uint8_t percentile) const
{
    if (_samples.empty() == true) {
        return 0;
    }

    std::vector<uint64_t> sorted(_samples);
    std::sort(sorted.begin(), sorted.end());
    const size_t index = ((sorted.size() - 1) * std::min<uint8_t>(percentile, 100)) / 100;

    return sorted[index];
}

MetricRecord::MetricRecord(const uint32_t percentileSampleCapacity)
    : _lock()
    , _latencies(percentileSampleCapacity)
    , _total(0)
    , _success(0)
    , _error(0)
    , _timeout(0)
    , _slow(0)
    , _dropped(0)
    , _minimum(0)
    , _maximum(0)
    , _latencyTotal(0)
    , _latencyCount(0)
{
}

void MetricRecord::Complete(const Outcome outcome, const uint64_t durationMs, const bool slow)
{
    std::lock_guard<std::mutex> guard(_lock);

    ++_total;
    if (outcome == Outcome::SUCCESS) {
        ++_success;
    } else {
        ++_error;
    }
    if (outcome == Outcome::TIMEOUT) {
        ++_timeout;
    }
    if (slow == true) {
        ++_slow;
    }

    if ((_latencyCount == 0) || (durationMs < _minimum)) {
        _minimum = durationMs;
    }
    if (durationMs > _maximum) {
        _maximum = durationMs;
    }
    _latencyTotal += durationMs;
    ++_latencyCount;
    _latencies.Add(durationMs);
}

void MetricRecord::Reset()
{
    std::lock_guard<std::mutex> guard(_lock);

    _total = 0;
    _success = 0;
    _error = 0;
    _timeout = 0;
    _slow = 0;
    _dropped = 0;
    _minimum = 0;
    _maximum = 0;
    _latencyTotal = 0;
    _latencyCount = 0;
    _latencies.Reset();
}

MetricSnapshot MetricRecord::Snapshot() const
{
    std::lock_guard<std::mutex> guard(_lock);

    MetricSnapshot snapshot;
    snapshot.total = _total;
    snapshot.success = _success;
    snapshot.error = _error;
    snapshot.timeout = _timeout;
    snapshot.slow = _slow;
    snapshot.dropped = _dropped;
    snapshot.hasLatency = (_latencyCount != 0);
    snapshot.minimumLatencyMs = _minimum;
    snapshot.maximumLatencyMs = _maximum;
    snapshot.averageLatencyMs = (_latencyCount == 0 ? 0 : _latencyTotal / _latencyCount);
    snapshot.percentile95LatencyMs = _latencies.Percentile(95);
    snapshot.percentile99LatencyMs = _latencies.Percentile(99);

    return snapshot;
}

MetricsStore::MetricsStore(const uint32_t maxMethodsPerPlugin, const uint32_t percentileSampleCapacity)
    : _maxMethodsPerPlugin(std::max<uint32_t>(maxMethodsPerPlugin, 1))
    , _percentileSampleCapacity(std::max<uint32_t>(percentileSampleCapacity, 1))
    , _lock()
    , _runtime(_percentileSampleCapacity)
    , _plugins()
{
}

MetricRecord& MetricsStore::MethodRecord(PluginRecords& plugin, const std::string& method)
{
    const auto existing = plugin.methods.find(method);
    if (existing != plugin.methods.end()) {
        return existing->second;
    }

    if (plugin.methods.size() >= _maxMethodsPerPlugin) {
        ++plugin.cardinalityDrops;
        return plugin.other;
    }

    return plugin.methods.emplace(std::piecewise_construct,
        std::forward_as_tuple(method),
        std::forward_as_tuple(_percentileSampleCapacity)).first->second;
}

void MetricsStore::Complete(const std::string& callsign, const std::string& method, const Outcome outcome,
    const uint64_t durationMs, const bool slow)
{
    _runtime.Complete(outcome, durationMs, slow);

    std::lock_guard<std::mutex> guard(_lock);
    PluginRecords& plugin = _plugins.emplace(std::piecewise_construct,
        std::forward_as_tuple(callsign),
        std::forward_as_tuple(_percentileSampleCapacity)).first->second;
    plugin.aggregate.Complete(outcome, durationMs, slow);
    MethodRecord(plugin, method).Complete(outcome, durationMs, slow);
}

MetricSnapshot MetricsStore::Runtime() const
{
    return _runtime.Snapshot();
}

MetricSnapshot MetricsStore::Plugin(const std::string& callsign) const
{
    std::lock_guard<std::mutex> guard(_lock);
    const auto plugin = _plugins.find(callsign);

    return (plugin == _plugins.end() ? MetricSnapshot() : plugin->second.aggregate.Snapshot());
}

MetricSnapshot MetricsStore::Method(const std::string& callsign, const std::string& method) const
{
    std::lock_guard<std::mutex> guard(_lock);
    const auto plugin = _plugins.find(callsign);
    if (plugin == _plugins.end()) {
        return MetricSnapshot();
    }

    const auto metric = plugin->second.methods.find(method);
    return (metric == plugin->second.methods.end() ? plugin->second.other.Snapshot() : metric->second.Snapshot());
}

void MetricsStore::Reset()
{
    _runtime.Reset();

    std::lock_guard<std::mutex> guard(_lock);
    for (auto& plugin : _plugins) {
        plugin.second.aggregate.Reset();
        plugin.second.other.Reset();
        plugin.second.cardinalityDrops = 0;
        for (auto& method : plugin.second.methods) {
            method.second.Reset();
        }
    }
}

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder
