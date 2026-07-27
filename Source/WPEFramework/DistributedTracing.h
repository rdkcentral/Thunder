/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

#pragma once

#ifdef THUNDER_DISTRIBUTED_TRACING

#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace WPEFramework {

namespace PluginHost {

    // Metadata tracked per active span (timing, callsign, operation)
    struct SpanMetadata {
        std::string contextKey;
        std::string callsign;
        std::string operation;
        std::chrono::steady_clock::time_point startTime;

        SpanMetadata()
            : startTime(std::chrono::steady_clock::now()) {}

        double ElapsedMs() const {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - startTime).count();
        }
    };

    /**
     * Distributed tracing manager for Thunder core.
     *
     * Delegates ALL OTEL operations to the shared rdk_otlp_instrumentation
     * library (librdk_otel_instrumentation.so). This ensures Thunder participates
     * in the same distributed traces as Rbus, tr181, RFC, and every other RDK
     * component that uses the wrapper.
     *
     * Traces plugin-to-plugin and plugin-to-Thunder interactions through
     * JSON-RPC and COM-RPC paths only.
     *
    * JSON-RPC and COM-RPC method handlers consume propagated traceparent
    * values to start child spans.
     */
    class DistributedTracing {
    public:
        DistributedTracing(const DistributedTracing&) = delete;
        DistributedTracing& operator=(const DistributedTracing&) = delete;

        static DistributedTracing& Instance();

        // Lifecycle
        void Initialize();
        void Shutdown();
        bool IsEnabled() const { return _enabled.load(); }

        // =========================================================================
        // JSON-RPC Invoke Tracing (Plugin-to-Plugin Communication)
        // =========================================================================

        uint64_t OnInvokeBegin(const std::string& callsign, const std::string& method,
                       uint32_t channelId, const char* traceparent);
        void OnInvokeEnd(uint64_t spanId, uint32_t result);

        // =========================================================================
        // COM-RPC Interface Tracing (Cross-Process Plugin Communication)
        // =========================================================================

        uint64_t OnCOMRPCAcquireBegin(const std::string& callsign, uint32_t interfaceId,
                          const char* traceparent);
        void OnCOMRPCAcquireEnd(uint64_t spanId, bool success);

        // =========================================================================
        // COM-RPC Per-Method Stub Tracing (via span ID in RPC::Data::Input)
        // =========================================================================

        static void OnCOMRPCStubBegin(uint64_t parentSpanId, uint32_t interfaceId, uint8_t methodId, const char* traceparent);
        static void OnCOMRPCStubEnd(uint64_t parentSpanId);

    private:
        DistributedTracing();
        ~DistributedTracing();

        uint64_t NextSpanId();

        std::atomic<bool> _enabled;
        std::atomic<uint64_t> _spanCounter;

        mutable std::mutex _spanLock;
        std::unordered_map<uint64_t, SpanMetadata> _activeSpans;
    };

} // namespace PluginHost

} // namespace WPEFramework

#endif // THUNDER_DISTRIBUTED_TRACING
