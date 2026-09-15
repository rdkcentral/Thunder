#pragma once

#include "Module.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

/**
 * @brief Runtime collection levels. OFF must avoid diagnostics record allocation.
 */
enum class ObservabilityLevel : uint8_t {
    OFF,
    BASIC,
    STANDARD,
    DETAILED,
    DEBUG
};

/**
 * @brief Categories of bounded diagnostic events.
 */
enum class EventType : uint8_t {
    LIFECYCLE,
    RPC_ERROR,
    RPC_TIMEOUT,
    RPC_SLOW,
    HEALTH_TRANSITION
};

/**
 * @brief Stable diagnostic result categories independent of raw Thunder results.
 */
enum class Outcome : uint8_t {
    SUCCESS,
    INVALID_REQUEST,
    METHOD_NOT_FOUND,
    PLUGIN_UNAVAILABLE,
    INTERNAL_ERROR,
    TRANSPORT_ERROR,
    AUTHORIZATION_FAILURE,
    TIMEOUT
};

/**
 * @brief Immutable aggregate returned by future management adapters.
 */
struct MetricSnapshot {
    uint64_t total { 0 };
    uint64_t success { 0 };
    uint64_t error { 0 };
    uint64_t timeout { 0 };
    uint64_t slow { 0 };
    uint64_t active { 0 };
    uint64_t dropped { 0 };
    uint64_t minimumLatencyMs { 0 };
    uint64_t maximumLatencyMs { 0 };
    uint64_t averageLatencyMs { 0 };
    uint64_t percentile95LatencyMs { 0 };
    uint64_t percentile99LatencyMs { 0 };
    bool hasLatency { false };
};

/**
 * @brief Payload-free event retained by the bounded event ring.
 */
struct EventSnapshot {
    uint64_t sequence { 0 };
    uint64_t timestampMs { 0 };
    EventType type { EventType::LIFECYCLE };
    Outcome outcome { Outcome::SUCCESS };
    uint32_t rawResult { Core::ERROR_NONE };
    uint64_t durationMs { 0 };
    std::string callsign;
    std::string method;
    std::string correlationId;
};

/**
 * @brief Cursor-oriented result from a bounded event buffer.
 */
struct EventPage {
    std::vector<EventSnapshot> events;
    uint64_t nextSequence { 0 };
    uint64_t overwritten { 0 };
    bool truncated { false };
};

/**
 * @brief Internal configuration copied from the host configuration model.
 */
struct Configuration {
    ObservabilityLevel level { ObservabilityLevel::OFF };
    uint32_t slowRpcThresholdMs { 0 };
    uint32_t eventBufferCapacity { 256 };
    uint32_t transactionBufferCapacity { 64 };
    uint32_t maxMethodsPerPlugin { 64 };
    uint32_t percentileSampleCapacity { 128 };
    bool enableConnectionMetrics { false };
    bool enableResourceMetrics { false };
    uint32_t resourceSampleIntervalMs { 0 };
    bool enableCorrelationRecords { false };
};

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder
