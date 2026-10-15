/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "Module.h"

#include <atomic>
#include <string>
#include <utility>

#include <opentelemetry/context/context.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/nostd/string_view.h>
#include <opentelemetry/nostd/unique_ptr.h>
#include <opentelemetry/trace/context.h>
#include <opentelemetry/trace/default_span.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/trace_state.h>

namespace Thunder {
namespace Telemetry {

class EXTERNAL OpenTelemetry {
private:
    static opentelemetry::trace::SpanContext ToSpanContext(
        opentelemetry::nostd::string_view traceparent,
        opentelemetry::nostd::string_view tracestate);
    static std::string ToTraceParent(const opentelemetry::trace::SpanContext& spanContext);

public:
    class Config : public Core::JSON::Container {
    public:
        Config()
            : Core::JSON::Container()
            , Timeout(100)
        {
            Add(_T("timeout"), &Timeout);
        }

        Core::JSON::DecUInt32 Timeout;
    };

    class EXTERNAL Span {
    public:
        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;
        Span& operator=(Span&&) = delete;

        template <typename... Args>
        Span(OpenTelemetry& owner, Args&&... args)
            : _jobContext(Core::JobContext::Instance())
            , _span()
            , _scope()
            , _hadTraceParent(_jobContext.HasTraceParent())
            , _hadTraceState(_jobContext.HasTraceState())
            , _traceParent(_hadTraceParent ? _jobContext.TraceParent() : Core::string())
            , _traceState(_hadTraceState ? _jobContext.TraceState() : Core::string())
            , _active(true)
        {
            auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("ThunderOpenTelemetry");
            if ((_hadTraceParent == true) && (_traceParent.empty() == false)) {
                const auto parentSpanContext = OpenTelemetry::ToSpanContext(
                    opentelemetry::nostd::string_view(_traceParent.data(), _traceParent.length()),
                    opentelemetry::nostd::string_view(_traceState.data(), _traceState.length()));
                opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> parentSpan(
                    new opentelemetry::trace::DefaultSpan(parentSpanContext));
                auto parentContext = opentelemetry::trace::SetSpan(
                    opentelemetry::context::Context(), std::move(parentSpan));
                opentelemetry::trace::StartSpanOptions options;
                options.parent = &parentContext;
                _span = tracer->StartSpan(std::forward<Args>(args)..., options);
            } else {
                _span = tracer->StartSpan(std::forward<Args>(args)...);
            }

            _scope = tracer->WithActiveSpan(_span);
            _jobContext.TraceParent(OpenTelemetry::ToTraceParent(_span->GetContext()));
            if (_hadTraceState == true) {
                _jobContext.TraceState(_traceState);
            }
        }

        Span(Span&& move) noexcept
            : _jobContext(move._jobContext)
            , _span(std::move(move._span))
            , _scope(std::move(move._scope))
            , _hadTraceParent(move._hadTraceParent)
            , _hadTraceState(move._hadTraceState)
            , _traceParent(std::move(move._traceParent))
            , _traceState(std::move(move._traceState))
            , _active(move._active)
        {
            move._active = false;
        }
        ~Span();

        opentelemetry::trace::Span* operator->() const
        {
            return (_span.get());
        }
        opentelemetry::trace::Span& Get() const
        {
            return (*_span);
        }

    private:
        Core::JobContext& _jobContext;
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> _span;
        opentelemetry::nostd::unique_ptr<opentelemetry::trace::Scope> _scope;
        bool _hadTraceParent;
        bool _hadTraceState;
        Core::string _traceParent;
        Core::string _traceState;
        bool _active;
    };

public:
    static OpenTelemetry& Instance();

    template <typename... Args>
    Span StartSpan(Args&&... args)
    {
        return Span(*this, std::forward<Args>(args)...);
    }

    void Initialize(const Config& config);
    void Deinitialize();

private:
    OpenTelemetry()
        : _references(0)
        , _shutdownTimeoutMicroseconds(100)
        , _tracer()
    {
    }
    OpenTelemetry(const OpenTelemetry&) = delete;
    OpenTelemetry& operator=(const OpenTelemetry&) = delete;
    OpenTelemetry(OpenTelemetry&&) = delete;
    OpenTelemetry& operator=(OpenTelemetry&&) = delete;

private:
    std::atomic<uint16_t> _references;
    uint32_t _shutdownTimeoutMicroseconds;
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> _tracer;
};

}
}
