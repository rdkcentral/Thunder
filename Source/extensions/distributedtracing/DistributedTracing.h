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

#include <string>
#include <atomic>
#include <cstdint>

namespace WPEFramework {

namespace PluginHost {

    /**
     * Distributed tracing manager for Thunder.
     *
     * Built as a separate shared library (libWPEFrameworkDistributedTracing.so)
     * under Source/extensions/distributedtracing/ so that both WPEFramework
     * and WPEProcess can link and initialize it.
     *
     * Delegates all OTEL operations to the shared rdk_otlp_instrumentation
     * library. This ensures Thunder participates in the same distributed traces
     * as Rbus, tr181, RFC, and every other RDK component that uses the wrapper.
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
        // Traceparent access (for COM-RPC proxy side)
        // =========================================================================

        // Returns the current traceparent from rdk_otlp TLS, or nullptr if
        // tracing is not enabled or no traceparent is active.
        const char* GetCurrentTraceparent() const;

        // =========================================================================
        // JSON-RPC Invoke Tracing (Plugin-to-Plugin Communication)
        // =========================================================================

        bool OnInvokeBegin(const std::string& callsign, const std::string& method,
                       uint32_t channelId);
        void OnInvokeEnd(bool spanActive, uint32_t result);

        // =========================================================================
        // COM-RPC Interface Tracing (Cross-Process Plugin Communication)
        // =========================================================================

        bool OnCOMRPCAcquireBegin(const std::string& callsign, uint32_t interfaceId);
        void OnCOMRPCAcquireEnd(bool spanActive, bool success);

        // =========================================================================
        // COM-RPC Per-Method Stub Tracing (via traceparent in message header)
        // =========================================================================

        static void OnCOMRPCStubBegin(uint32_t interfaceId, uint8_t methodId, const char* traceparent);
        static void OnCOMRPCStubEnd();

    private:
        DistributedTracing();
        ~DistributedTracing();

        std::atomic<bool> _enabled;
    };

} // namespace PluginHost

} // namespace WPEFramework
