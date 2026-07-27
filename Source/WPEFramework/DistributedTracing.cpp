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

#ifdef THUNDER_DISTRIBUTED_TRACING

#include "DistributedTracing.h"
#include "Module.h"
#include <com/Administrator.h>

#include <rdk_otlp_instrumentation.h>

namespace WPEFramework {

namespace PluginHost {

    // =========================================================================
    // Helpers
    // =========================================================================

    std::string DistributedTracing::MakeContextKey(
        const std::string& prefix, const std::string& callsign)
    {
        return prefix + "." + callsign;
    }

    std::string DistributedTracing::MakeSpanContextKey(const uint64_t spanId)
    {
        return std::string("Thunder.comrpc.span.") + std::to_string(spanId);
    }

    // =========================================================================
    // Singleton
    // =========================================================================
    DistributedTracing& DistributedTracing::Instance() {
        static DistributedTracing instance;
        return instance;
    }

    DistributedTracing::DistributedTracing()
        : _enabled(false)
        , _spanCounter(0) {
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

        {
            std::lock_guard<std::mutex> lock(_spanLock);
            for (auto& entry : _activeSpans) {
                rdk_otlp_finish_child_span();
            }
            _activeSpans.clear();
        }

        rdk_otlp_force_flush();
        rdk_otlp_shutdown();

        SYSLOG(Logging::Shutdown, (_T("DistributedTracing: Shutdown complete")));
    }

    uint64_t DistributedTracing::NextSpanId() {
        return ++_spanCounter;
    }

    // =========================================================================
    // JSON-RPC Invoke Tracing
    // =========================================================================

    uint64_t DistributedTracing::OnInvokeBegin(
        const std::string& callsign, const std::string& method,
        uint32_t channelId)
    {
        if (!_enabled.load()) return 0;

        std::string ctxKey = MakeContextKey("Thunder.jsonrpc", callsign);

        // Thunder must NOT start a parent trace on its own.
        // Only create a child span if an upstream component already started a trace.
        char traceId[33] = {0}, parentSpanId[17] = {0}, flags[3] = {0};
        bool hasParent = rdk_otlp_get_trace_context(ctxKey.c_str(),
                                                     traceId, parentSpanId, flags);

        if (!hasParent) {
            return 0;
        }

        uint64_t spanId = NextSpanId();
        rdk_otlp_start_child_span(ctxKey.c_str(), "jsonrpc.invoke");

        rdk_otlp_set_span_attribute_string("plugin.callsign", callsign.c_str());
        rdk_otlp_set_span_attribute_string("rpc.method", method.c_str());
        rdk_otlp_set_span_attribute_string("rpc.system", "jsonrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");

        std::string channelStr = std::to_string(channelId);
        rdk_otlp_set_span_attribute_string("rpc.channel_id", channelStr.c_str());

        // Set the span ID in the thread-local so that any COM-RPC proxy calls
        // made during this JSON-RPC invoke carry the span ID in the message.
        RPC::SetCurrentTraceSpanId(spanId);

        SpanMetadata meta;
        meta.contextKey = ctxKey;
        meta.callsign = callsign;
        meta.operation = method;

        std::lock_guard<std::mutex> lock(_spanLock);
        _activeSpans[spanId] = std::move(meta);

        return spanId;
    }

    void DistributedTracing::OnInvokeEnd(uint64_t spanId, uint32_t result) {
        if (!_enabled.load() || spanId == 0) return;

        RPC::SetCurrentTraceSpanId(0);

        SpanMetadata meta;
        {
            std::lock_guard<std::mutex> lock(_spanLock);
            auto it = _activeSpans.find(spanId);
            if (it == _activeSpans.end()) return;
            meta = std::move(it->second);
            _activeSpans.erase(it);
        }

        std::string resultStr = std::to_string(result);
        rdk_otlp_set_span_attribute_string("rpc.result_code", resultStr.c_str());

        std::string durationStr = std::to_string(meta.ElapsedMs());
        rdk_otlp_set_span_attribute_string("rpc.duration_ms", durationStr.c_str());

        if (result != 0) {
            rdk_otlp_set_span_attribute_string("otel.status_code", "ERROR");
        }

        rdk_otlp_finish_child_span();
    }

    // =========================================================================
    // COM-RPC Interface Tracing
    // =========================================================================

    uint64_t DistributedTracing::OnCOMRPCAcquireBegin(
        const std::string& callsign, uint32_t interfaceId)
    {
        if (!_enabled.load()) return 0;

        std::string ctxKey = MakeContextKey("Thunder.comrpc", callsign);

        // Thunder must NOT start a parent trace on its own.
        // Only create a child span if an upstream component already started a trace.
        char traceId[33] = {0}, parentSpanId[17] = {0}, flags[3] = {0};
        bool hasParent = rdk_otlp_get_trace_context(ctxKey.c_str(),
                                                     traceId, parentSpanId, flags);

        if (!hasParent) {
            return 0;
        }

        uint64_t spanId = NextSpanId();
        rdk_otlp_start_child_span(ctxKey.c_str(), "comrpc.acquire");

        rdk_otlp_set_span_attribute_string("plugin.callsign", callsign.c_str());
        rdk_otlp_set_span_attribute_string("rpc.system", "comrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");

        std::string ifIdStr = std::to_string(interfaceId);
        rdk_otlp_set_span_attribute_string("comrpc.interface_id", ifIdStr.c_str());

        RPC::SetCurrentTraceSpanId(spanId);

        SpanMetadata meta;
        meta.contextKey = ctxKey;
        meta.callsign = callsign;
        meta.operation = "comrpc.acquire";

        std::lock_guard<std::mutex> lock(_spanLock);
        _activeSpans[spanId] = std::move(meta);

        return spanId;
    }

    void DistributedTracing::OnCOMRPCAcquireEnd(uint64_t spanId, bool success) {
        if (!_enabled.load() || spanId == 0) return;

        RPC::SetCurrentTraceSpanId(0);

        SpanMetadata meta;
        {
            std::lock_guard<std::mutex> lock(_spanLock);
            auto it = _activeSpans.find(spanId);
            if (it == _activeSpans.end()) return;
            meta = std::move(it->second);
            _activeSpans.erase(it);
        }

        rdk_otlp_set_span_attribute_string("comrpc.acquire.success",
                                            success ? "true" : "false");

        std::string durationStr = std::to_string(meta.ElapsedMs());
        rdk_otlp_set_span_attribute_string("comrpc.acquire.duration_ms",
                                            durationStr.c_str());

        if (!success) {
            rdk_otlp_set_span_attribute_string("otel.status_code", "ERROR");
        }

        rdk_otlp_finish_child_span();
    }

    // =========================================================================
    // COM-RPC Per-Method Stub Tracing
    // =========================================================================

    /* static */ void DistributedTracing::OnCOMRPCStubBegin(
        uint64_t parentSpanId, uint32_t interfaceId, uint8_t methodId)
    {
        auto& self = Instance();
        if (!self._enabled.load()) return;

        std::string ctxKey = MakeSpanContextKey(parentSpanId);

        char traceId[33] = {0}, parentId[17] = {0}, flags[3] = {0};
        bool hasParent = rdk_otlp_get_trace_context(ctxKey.c_str(),
                                                    traceId, parentId, flags);

        if (!hasParent) {
            return;
        }

        rdk_otlp_start_child_span(ctxKey.c_str(), "comrpc.stub.handle");

        rdk_otlp_set_span_attribute_string("rpc.system", "comrpc");
        rdk_otlp_set_span_attribute_string("rdk.component", "Thunder");
        rdk_otlp_set_span_attribute_string("comrpc.side", "stub");

        std::string ifStr = std::to_string(interfaceId);
        std::string methStr = std::to_string(methodId);
        std::string parentStr = std::to_string(parentSpanId);
        rdk_otlp_set_span_attribute_string("comrpc.interface_id", ifStr.c_str());
        rdk_otlp_set_span_attribute_string("comrpc.method_id", methStr.c_str());
        rdk_otlp_set_span_attribute_string("comrpc.parent_span_id", parentStr.c_str());
    }

    /* static */ void DistributedTracing::OnCOMRPCStubEnd(uint64_t parentSpanId)
    {
        auto& self = Instance();
        if (!self._enabled.load()) return;

        rdk_otlp_finish_child_span();
    }

} // namespace PluginHost

} // namespace WPEFramework

#endif // THUNDER_DISTRIBUTED_TRACING
