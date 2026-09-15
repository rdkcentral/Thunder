#include "DiagnosticsManager.h"

#include <chrono>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

DiagnosticsManager::DiagnosticsManager()
    : _lock()
    , _enabled(false)
    , _configuration()
    , _metrics()
    , _events()
    , _epoch(0)
{
}

DiagnosticsManager::~DiagnosticsManager()
{
    Shutdown();
}

void DiagnosticsManager::Initialize(const Configuration& configuration)
{
    std::lock_guard<std::mutex> guard(_lock);

    _enabled = false;
    _events.reset();
    _metrics.reset();
    _configuration = configuration;

    if (_configuration.level == ObservabilityLevel::OFF) {
        return;
    }

    try {
        _metrics.reset(new MetricsStore(_configuration.maxMethodsPerPlugin, _configuration.percentileSampleCapacity));
        _events.reset(new EventStore(_configuration.eventBufferCapacity));
        _enabled = true;
    } catch (...) {
        // Diagnostics is observational: a setup failure must leave runtime behavior unchanged.
        _metrics.reset();
        _events.reset();
        _enabled = false;
    }
}

void DiagnosticsManager::Shutdown()
{
    std::lock_guard<std::mutex> guard(_lock);
    _enabled = false;
    _events.reset();
    _metrics.reset();
}

bool DiagnosticsManager::Enabled(const ObservabilityLevel minimum) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return ((_enabled == true) && (static_cast<uint8_t>(_configuration.level) >= static_cast<uint8_t>(minimum)));
}

void DiagnosticsManager::OnRpcCompleted(const std::string& callsign, const std::string& method,
    const std::string& correlationId, const Outcome outcome, const uint32_t rawResult,
    const uint64_t durationMs) noexcept
{
    try {
        std::lock_guard<std::mutex> guard(_lock);
        if ((_enabled == false) || (static_cast<uint8_t>(_configuration.level) < static_cast<uint8_t>(ObservabilityLevel::BASIC))) {
            return;
        }

        const bool slow = ((_configuration.slowRpcThresholdMs != 0)
            && (durationMs >= _configuration.slowRpcThresholdMs));
        _metrics->Complete(callsign, method, outcome, durationMs, slow);

        if ((outcome != Outcome::SUCCESS) || (slow == true)) {
            EventSnapshot event;
            event.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            event.type = (slow == true ? EventType::RPC_SLOW : EventType::RPC_ERROR);
            event.outcome = outcome;
            event.rawResult = rawResult;
            event.durationMs = durationMs;
            event.callsign = callsign;
            event.method = method;
            event.correlationId = correlationId;
            _events->Add(std::move(event));
        }
    } catch (...) {
        // An observer failure must never affect the original dispatcher outcome.
    }
}

MetricSnapshot DiagnosticsManager::SnapshotRuntime() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return (_metrics ? _metrics->Runtime() : MetricSnapshot());
}

MetricSnapshot DiagnosticsManager::SnapshotPlugin(const std::string& callsign) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return (_metrics ? _metrics->Plugin(callsign) : MetricSnapshot());
}

EventPage DiagnosticsManager::SnapshotEvents(const uint64_t afterSequence, const uint32_t limit) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return (_events ? _events->Snapshot(afterSequence, limit) : EventPage());
}

void DiagnosticsManager::ResetMetrics()
{
    std::lock_guard<std::mutex> guard(_lock);
    if (_metrics) {
        _metrics->Reset();
    }
    if (_events) {
        _events->Reset();
    }
    ++_epoch;
}

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder
