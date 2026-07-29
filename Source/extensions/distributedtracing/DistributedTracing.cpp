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

#include "DistributedTracing.h"
#include "Module.h"
#include <com/Administrator.h>

#include <rdk_otlp_instrumentation.h>

namespace WPEFramework {

namespace PluginHost {

    // =========================================================================
    // Singleton
    // =========================================================================
    DistributedTracing& DistributedTracing::Instance() {
        static DistributedTracing instance;
        return instance;
    }

    DistributedTracing::DistributedTracing()
        : _enabled(false) {
    }

    DistributedTracing::~DistributedTracing() {
        Shutdown();
    }

    void DistributedTracing::Initialize() {
        if (_enabled.load()) {
            return;
        }

        rdk_otlp_init("Thunder", "R4.4");

        _enabled.store(true);

        // Register stub-side tracing callbacks with the com library.
        static RPC::ICOMRPCStubTraceCallbacks stubCallbacks = {
            &DistributedTracing::OnCOMRPCStubBegin,
            &DistributedTracing::OnCOMRPCStubEnd
        };
        RPC::SetCOMRPCStubTraceCallbacks(&stubCallbacks);

        const char* endpoint = rdk_otlp_get_endpoint();
        SYSLOG(Logging::Startup,
            (_T("DistributedTracing: Enabled via rdk_otlp_instrumentation, collector=%s"),
             endpoint ? endpoint : "default"));
    }

    void DistributedTracing::Shutdown() {
        if (!_enabled.exchange(false)) {
            return;
        }

        RPC::SetCOMRPCStubTraceCallbacks(nullptr);

        rdk_otlp_force_flush();
        rdk_otlp_shutdown();

        SYSLOG(Logging::Shutdown, (_T("DistributedTracing: Shutdown complete")));
    }

    // =========================================================================
    // Traceparent access
    // =========================================================================

    const char* DistributedTracing::GetCurrentTraceparent() const {
        if (!_enabled.load()) {
            return nullptr;
        }
        return rdk_otlp_get_current_traceparent();
    }

    // =========================================================================
    // JSON-RPC Invoke Tracing
    // =========================================================================

    bool DistributedTracing::OnInvokeBegin(
        const std::string& callsign, const std::string& method,
        uint32_t channelId)
    {
        if (!_enabled.load()) return false;

        const char* traceparent = rdk_otlp_get_current_traceparent();

        // Start a child span only when an active traceparent exists.
        if ((traceparent == nullptr) || (traceparent[0] == '\0')) {
            return false;
        }

        rdk_otlp_start_child_from_traceparent(traceparent, "jsonrpc.invoke");

        rdk_otlp_set_span_attribute_string("plugin.callsign", callsign.c_str());
        rdk_otlp_set_span_attribute_string("rpc.method", method.c_str());
        rdk_otlp_set_span_attribute_string("rpc.system", "jsonrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");

        std::string channelStr = std::to_string(channelId);
        rdk_otlp_set_span_attribute_string("rpc.channel_id", channelStr.c_str());

        return true;
    }

    void DistributedTracing::OnInvokeEnd(bool spanActive, uint32_t result) {
        if (!_enabled.load() || !spanActive) return;

        std::string resultStr = std::to_string(result);
        rdk_otlp_set_span_attribute_string("rpc.result_code", resultStr.c_str());

        if (result != 0) {
            rdk_otlp_set_span_attribute_string("otel.status_code", "ERROR");
        }

        rdk_otlp_finish_child_span();
    }

    // =========================================================================
    // COM-RPC Interface Tracing
    // =========================================================================

    bool DistributedTracing::OnCOMRPCAcquireBegin(
        const std::string& callsign, uint32_t interfaceId)
    {
        if (!_enabled.load()) return false;

        const char* traceparent = rdk_otlp_get_current_traceparent();

        if ((traceparent == nullptr) || (traceparent[0] == '\0')) {
            return false;
        }

        rdk_otlp_start_child_from_traceparent(traceparent, "comrpc.acquire");

        rdk_otlp_set_span_attribute_string("plugin.callsign", callsign.c_str());
        rdk_otlp_set_span_attribute_string("rpc.system", "comrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");

        std::string ifIdStr = std::to_string(interfaceId);
        rdk_otlp_set_span_attribute_string("comrpc.interface_id", ifIdStr.c_str());

        return true;
    }

    void DistributedTracing::OnCOMRPCAcquireEnd(bool spanActive, bool success) {
        if (!_enabled.load() || !spanActive) return;

        rdk_otlp_set_span_attribute_string("comrpc.acquire.success",
                                            success ? "true" : "false");

        if (!success) {
            rdk_otlp_set_span_attribute_string("otel.status_code", "ERROR");
        }

        rdk_otlp_finish_child_span();
    }

    // =========================================================================
    // COM-RPC Per-Method Stub Tracing
    // =========================================================================

    /* static */ void DistributedTracing::OnCOMRPCStubBegin(
        uint32_t interfaceId, uint8_t methodId, const char* traceparent)
    {
        auto& self = Instance();
        if (!self._enabled.load()) return;

        if ((traceparent != nullptr) && (traceparent[0] != '\0')) {
            rdk_otlp_start_child_from_traceparent(traceparent, "comrpc.stub.handle");
        } else {
            return;
        }

        rdk_otlp_set_span_attribute_string("rpc.system", "comrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");
        rdk_otlp_set_span_attribute_string("comrpc.side", "stub");

        std::string ifStr = std::to_string(interfaceId);
        std::string methStr = std::to_string(methodId);
        rdk_otlp_set_span_attribute_string("comrpc.interface_id", ifStr.c_str());
        rdk_otlp_set_span_attribute_string("comrpc.method_id", methStr.c_str());
    }

    /* static */ void DistributedTracing::OnCOMRPCStubEnd()
    {
        auto& self = Instance();
        if (!self._enabled.load()) return;

        rdk_otlp_finish_child_span();
    }

} // namespace PluginHost

} // namespace WPEFramework
