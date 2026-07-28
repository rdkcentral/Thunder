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
 *     http://www.apache.org/licenses/LICENSE-2.0
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

// Direct include — WPEFramework binary links librdk_otlp at build time.
// rdk_otlp_init / rdk_otlp_shutdown are called ONCE at startup/shutdown,
// NOT on every incoming request.  Per-request work only calls
// start_child_from_traceparent / finish_child_span.
#include <rdk_otlp_instrumentation.h>

#include <cstring>
#include <cstdio>

namespace WPEFramework {

namespace PluginHost {

// ─────────────────────────────────────────────────────────────────────────────
// Thread-local storage for the current COM-RPC stub span.
// Each COM-RPC worker thread independently tracks whether it has an active
// stub span so that onEnd can match onBegin on the same thread.
// ─────────────────────────────────────────────────────────────────────────────
namespace {
    thread_local bool     tl_stubSpanActive = false;
    thread_local uint32_t tl_stubIfId       = 0;
    thread_local uint8_t  tl_stubMethodId   = 0;
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
/* static */
DistributedTracing& DistributedTracing::Instance()
{
    static DistributedTracing instance;
    return instance;
}

DistributedTracing::DistributedTracing()
{
}

DistributedTracing::~DistributedTracing()
{
    Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle — called ONCE each at startup / shutdown
// ─────────────────────────────────────────────────────────────────────────────
void DistributedTracing::Initialize()
{
    if (_enabled.load(std::memory_order_acquire)) return;

    // Initialise the WPEFramework-side tracer so it can export spans to the
    // OTLP collector.  Called once at process startup.
    rdk_otlp_init("Thunder", "R4.4");

    // Register stub-side callbacks so Administrator::Invoke() can notify us
    // when an incoming COM-RPC message carries a traceparent.
    static RPC::ICOMRPCStubTraceCallbacks stubCbs = {
        &DistributedTracing::OnCOMRPCStubBegin,
        &DistributedTracing::OnCOMRPCStubEnd
    };
    RPC::SetCOMRPCStubTraceCallbacks(&stubCbs);

    _enabled.store(true, std::memory_order_release);

    SYSLOG(Logging::Startup,
        (_T("DistributedTracing: Enabled (Thunder R4.4, librdk_otlp)")));
}

void DistributedTracing::Shutdown()
{
    if (!_enabled.exchange(false)) return;

    // Unregister stub callbacks first — no new spans will be started.
    RPC::SetCOMRPCStubTraceCallbacks(nullptr);

    {
        std::lock_guard<std::mutex> lock(_spanLock);
        _activeSpans.clear();
    }

    // Flush and shut down the tracer.  Called once at process shutdown.
    rdk_otlp_force_flush();
    rdk_otlp_shutdown();

    SYSLOG(Logging::Shutdown, (_T("DistributedTracing: Shutdown complete")));
}

uint64_t DistributedTracing::NextSpanId()
{
    return ++_spanCounter;
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON-RPC path
//
// traceparent is extracted from the incoming JSON-RPC params by PluginServer.h
// (_tp field injected by JSONRPCLink on the caller side).
// We must NOT read from TLS here — this runs on Thunder's thread pool thread,
// not the caller's thread.  TLS does not survive thread transitions.
// ─────────────────────────────────────────────────────────────────────────────
uint64_t DistributedTracing::OnInvokeBegin(
    const std::string& callsign,
    const std::string& method,
    uint32_t channelId,
    const char* traceparent)
{
    if (!_enabled.load(std::memory_order_acquire)) return 0;
    if ((traceparent == nullptr) || (traceparent[0] == '\0')) return 0;

    char spanName[128];
    ::snprintf(spanName, sizeof(spanName), "Thunder.jsonrpc.%s.%s",
               callsign.c_str(), method.c_str());

    rdk_otlp_start_child_from_traceparent(traceparent, spanName);

    rdk_otlp_set_span_attribute_string("plugin.callsign", callsign.c_str());
    rdk_otlp_set_span_attribute_string("rpc.method",      method.c_str());
    rdk_otlp_set_span_attribute_string("rpc.system",      "jsonrpc");
    rdk_otlp_set_span_attribute_string("rdk.component",   "Thunder");

    char chStr[16];
    ::snprintf(chStr, sizeof(chStr), "%u", channelId);
    rdk_otlp_set_span_attribute_string("rpc.channel_id", chStr);

    uint64_t spanId = NextSpanId();

    SpanMetadata meta;
    meta.callsign  = callsign;
    meta.operation = method;

    std::lock_guard<std::mutex> lock(_spanLock);
    _activeSpans[spanId] = std::move(meta);

    return spanId;
}

void DistributedTracing::OnInvokeEnd(uint64_t spanId, uint32_t result)
{
    if (!_enabled.load(std::memory_order_acquire) || (spanId == 0)) return;

    SpanMetadata meta;
    {
        std::lock_guard<std::mutex> lock(_spanLock);
        auto it = _activeSpans.find(spanId);
        if (it == _activeSpans.end()) return;
        meta = std::move(it->second);
        _activeSpans.erase(it);
    }

    char resultStr[16];
    ::snprintf(resultStr, sizeof(resultStr), "%u", result);
    rdk_otlp_set_span_attribute_string("rpc.result_code", resultStr);

    char durStr[32];
    ::snprintf(durStr, sizeof(durStr), "%.3f", meta.ElapsedMs());
    rdk_otlp_set_span_attribute_string("rpc.duration_ms", durStr);

    if (result != 0) {
        rdk_otlp_set_span_attribute_string("otel.status_code", "ERROR");
    }

    rdk_otlp_finish_child_span();
}

// ─────────────────────────────────────────────────────────────────────────────
// COM-RPC stub path
//
// Static callbacks registered with Administrator via ICOMRPCStubTraceCallbacks.
// traceparent was stamped into the binary message header by the proxy side
// (IUnknown.cpp) and extracted by Administrator::Invoke().
// These run on COM-RPC worker threads.
// ─────────────────────────────────────────────────────────────────────────────
/* static */
void DistributedTracing::OnCOMRPCStubBegin(
    uint32_t interfaceId, uint8_t methodId, const char* traceparent)
{
    auto& self = Instance();
    if (!self._enabled.load(std::memory_order_acquire)) return;
    if ((traceparent == nullptr) || (traceparent[0] == '\0')) return;

    char spanName[80];
    ::snprintf(spanName, sizeof(spanName), "Thunder.comrpc.if0x%X.method%u",
               interfaceId, static_cast<unsigned>(methodId));

    rdk_otlp_start_child_from_traceparent(traceparent, spanName);

    rdk_otlp_set_span_attribute_string("rpc.system",      "comrpc");
    rdk_otlp_set_span_attribute_string("rdk.component",   "Thunder");
    rdk_otlp_set_span_attribute_string("comrpc.side",     "stub");

    char ifStr[16], methStr[8];
    ::snprintf(ifStr,   sizeof(ifStr),   "0x%X", interfaceId);
    ::snprintf(methStr, sizeof(methStr), "%u",   static_cast<unsigned>(methodId));
    rdk_otlp_set_span_attribute_string("comrpc.interface_id", ifStr);
    rdk_otlp_set_span_attribute_string("comrpc.method_id",    methStr);

    tl_stubSpanActive = true;
    tl_stubIfId       = interfaceId;
    tl_stubMethodId   = methodId;
}

/* static */
void DistributedTracing::OnCOMRPCStubEnd(uint32_t interfaceId, uint8_t methodId)
{
    auto& self = Instance();
    if (!self._enabled.load(std::memory_order_acquire)) return;
    if (!tl_stubSpanActive) return;
    if ((tl_stubIfId != interfaceId) || (tl_stubMethodId != methodId)) return;

    rdk_otlp_finish_child_span();
    tl_stubSpanActive = false;
}

} // namespace PluginHost

} // namespace WPEFramework

#endif // THUNDER_DISTRIBUTED_TRACING

 *
 * Copyright 2024 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

#include <dlfcn.h>
#include <cstring>
#include <cstdio>

namespace WPEFramework {

namespace PluginHost {

// ─────────────────────────────────────────────────────────────────────────────
// librdk_otlp symbols resolved lazily via dlsym(RTLD_DEFAULT).
//
// Using RTLD_DEFAULT means we search the global symbol table of every library
// already loaded — this works regardless of whether the soname is versioned
// (e.g. librdk_otlp.so.1) because we look up by symbol name, not by library
// name.  WPEFramework does NOT hard-link librdk_otlp.
// ─────────────────────────────────────────────────────────────────────────────
namespace {
    using FnGetTraceparent          = const char* (*)();
    using FnStartChildTraceparent   = void (*)(const char* traceparent, const char* spanName);
    using FnFinishChild             = void (*)();
    using FnSetSpanAttrStr          = void (*)(const char* key, const char* value);
    using FnForceFlush              = void (*)();
    using FnShutdown                = void (*)();

    static std::atomic<bool>        s_resolved{false};
    static std::mutex               s_resolvedMutex;

    static FnGetTraceparent         s_getTraceparent       = nullptr;
    static FnStartChildTraceparent  s_startChildFromTp     = nullptr;
    static FnFinishChild            s_finishChild          = nullptr;
    static FnSetSpanAttrStr         s_setAttrStr           = nullptr;
    static FnForceFlush             s_forceFlush           = nullptr;
    static FnShutdown               s_shutdown             = nullptr;

    static void resolveSymbols()
    {
        if (s_resolved.load(std::memory_order_acquire)) return;
        std::lock_guard<std::mutex> lk(s_resolvedMutex);
        if (s_resolved.load(std::memory_order_relaxed)) return;

        void* fn = ::dlsym(RTLD_DEFAULT, "rdk_otlp_get_current_traceparent");
        if (fn == nullptr) return; // librdk_otlp not yet loaded — retry next time

        s_getTraceparent    = reinterpret_cast<FnGetTraceparent>(fn);
        s_startChildFromTp  = reinterpret_cast<FnStartChildTraceparent>(
                                  ::dlsym(RTLD_DEFAULT, "rdk_otlp_start_child_from_traceparent"));
        s_finishChild       = reinterpret_cast<FnFinishChild>(
                                  ::dlsym(RTLD_DEFAULT, "rdk_otlp_finish_child_span"));
        s_setAttrStr        = reinterpret_cast<FnSetSpanAttrStr>(
                                  ::dlsym(RTLD_DEFAULT, "rdk_otlp_set_span_attribute_string"));
        s_forceFlush        = reinterpret_cast<FnForceFlush>(
                                  ::dlsym(RTLD_DEFAULT, "rdk_otlp_force_flush"));
        s_shutdown          = reinterpret_cast<FnShutdown>(
                                  ::dlsym(RTLD_DEFAULT, "rdk_otlp_shutdown"));

        s_resolved.store(true, std::memory_order_release);
    }

    // Thread-local storage for the current COM-RPC stub span context.
    // Keyed as (interfaceId << 8 | methodId) so nested calls on the same thread
    // can be tracked independently (COM-RPC worker pool assigns one thread per
    // incoming message, so a single value is sufficient in practice).
    thread_local bool   tl_stubSpanActive = false;
    thread_local uint32_t tl_stubIfId     = 0;
    thread_local uint8_t  tl_stubMethodId = 0;
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
/* static */
DistributedTracing& DistributedTracing::Instance()
{
    static DistributedTracing instance;
    return instance;
}

DistributedTracing::DistributedTracing()
{
}

DistributedTracing::~DistributedTracing()
{
    Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────
void DistributedTracing::Initialize()
{
    if (_enabled.load(std::memory_order_acquire)) return;

    resolveSymbols();
    if (s_getTraceparent == nullptr) {
        // librdk_otlp not loaded — tracing unavailable.
        SYSLOG(Logging::Startup,
            (_T("DistributedTracing: librdk_otlp not found — distributed tracing disabled")));
        return;
    }

    // Register stub-side callbacks so Administrator::Invoke() can call us back
    // when an incoming COM-RPC message carries a traceparent.
    static RPC::ICOMRPCStubTraceCallbacks stubCbs = {
        &DistributedTracing::OnCOMRPCStubBegin,
        &DistributedTracing::OnCOMRPCStubEnd
    };
    RPC::SetCOMRPCStubTraceCallbacks(&stubCbs);

    _enabled.store(true, std::memory_order_release);

    SYSLOG(Logging::Startup,
        (_T("DistributedTracing: Enabled (librdk_otlp symbols resolved via RTLD_DEFAULT)")));
}

void DistributedTracing::Shutdown()
{
    if (!_enabled.exchange(false)) return;

    // Unregister stub callbacks first so no new spans are started.
    RPC::SetCOMRPCStubTraceCallbacks(nullptr);

    // Discard any bookkeeping for spans that were never closed
    // (should be empty in a clean shutdown).
    {
        std::lock_guard<std::mutex> lock(_spanLock);
        _activeSpans.clear();
    }

    if (s_forceFlush != nullptr) s_forceFlush();
    if (s_shutdown   != nullptr) s_shutdown();

    SYSLOG(Logging::Shutdown, (_T("DistributedTracing: Shutdown complete")));
}

uint64_t DistributedTracing::NextSpanId()
{
    return ++_spanCounter;
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON-RPC path
// ─────────────────────────────────────────────────────────────────────────────
uint64_t DistributedTracing::OnInvokeBegin(
    const std::string& callsign,
    const std::string& method,
    uint32_t channelId)
{
    if (!_enabled.load(std::memory_order_acquire)) return 0;

    resolveSymbols();
    if ((s_getTraceparent == nullptr) || (s_startChildFromTp == nullptr)) return 0;

    // Read the W3C traceparent from TLS — set by the caller via
    // rdk_otlp_start_distributed_trace().  We do NOT look in params.
    const char* tp = s_getTraceparent();
    if ((tp == nullptr) || (tp[0] == '\0')) return 0;

    // Build a human-readable span name.
    char spanName[128];
    ::snprintf(spanName, sizeof(spanName), "Thunder.jsonrpc.%s.%s",
               callsign.c_str(), method.c_str());

    s_startChildFromTp(tp, spanName);

    if (s_setAttrStr != nullptr) {
        s_setAttrStr("plugin.callsign", callsign.c_str());
        s_setAttrStr("rpc.method",      method.c_str());
        s_setAttrStr("rpc.system",      "jsonrpc");
        s_setAttrStr("rdk.component",   "Thunder");
        char chStr[16];
        ::snprintf(chStr, sizeof(chStr), "%u", channelId);
        s_setAttrStr("rpc.channel_id",  chStr);
    }

    uint64_t spanId = NextSpanId();

    SpanMetadata meta;
    meta.callsign  = callsign;
    meta.operation = method;

    std::lock_guard<std::mutex> lock(_spanLock);
    _activeSpans[spanId] = std::move(meta);

    return spanId;
}

void DistributedTracing::OnInvokeEnd(uint64_t spanId, uint32_t result)
{
    if (!_enabled.load(std::memory_order_acquire) || (spanId == 0)) return;
    if (s_finishChild == nullptr) return;

    SpanMetadata meta;
    {
        std::lock_guard<std::mutex> lock(_spanLock);
        auto it = _activeSpans.find(spanId);
        if (it == _activeSpans.end()) return;
        meta = std::move(it->second);
        _activeSpans.erase(it);
    }

    if (s_setAttrStr != nullptr) {
        char resultStr[16];
        ::snprintf(resultStr, sizeof(resultStr), "%u", result);
        s_setAttrStr("rpc.result_code", resultStr);

        char durStr[32];
        ::snprintf(durStr, sizeof(durStr), "%.3f", meta.ElapsedMs());
        s_setAttrStr("rpc.duration_ms", durStr);

        if (result != 0) {
            s_setAttrStr("otel.status_code", "ERROR");
        }
    }

    s_finishChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// COM-RPC stub path
//
// These are static so they can be stored as plain function pointers in
// ICOMRPCStubTraceCallbacks.  They delegate through Instance() to access
// the singleton's enabled flag.
// ─────────────────────────────────────────────────────────────────────────────
/* static */
void DistributedTracing::OnCOMRPCStubBegin(
    uint32_t interfaceId, uint8_t methodId, const char* traceparent)
{
    auto& self = Instance();
    if (!self._enabled.load(std::memory_order_acquire)) return;
    if ((s_startChildFromTp == nullptr) || (traceparent == nullptr) || (traceparent[0] == '\0')) return;

    char spanName[80];
    ::snprintf(spanName, sizeof(spanName), "Thunder.comrpc.if0x%X.method%u",
               interfaceId, static_cast<unsigned>(methodId));

    s_startChildFromTp(traceparent, spanName);

    if (s_setAttrStr != nullptr) {
        s_setAttrStr("rpc.system",      "comrpc");
        s_setAttrStr("rdk.component",   "Thunder");
        s_setAttrStr("comrpc.side",     "stub");
        char ifStr[16], methStr[8];
        ::snprintf(ifStr,   sizeof(ifStr),   "0x%X", interfaceId);
        ::snprintf(methStr, sizeof(methStr), "%u",   static_cast<unsigned>(methodId));
        s_setAttrStr("comrpc.interface_id", ifStr);
        s_setAttrStr("comrpc.method_id",    methStr);
    }

    tl_stubSpanActive = true;
    tl_stubIfId       = interfaceId;
    tl_stubMethodId   = methodId;
}

/* static */
void DistributedTracing::OnCOMRPCStubEnd(uint32_t interfaceId, uint8_t methodId)
{
    auto& self = Instance();
    if (!self._enabled.load(std::memory_order_acquire)) return;
    if (!tl_stubSpanActive) return;
    if ((tl_stubIfId != interfaceId) || (tl_stubMethodId != methodId)) return;
    if (s_finishChild == nullptr) return;

    s_finishChild();
    tl_stubSpanActive = false;
}

} // namespace PluginHost

} // namespace WPEFramework

#endif // THUNDER_DISTRIBUTED_TRACING
