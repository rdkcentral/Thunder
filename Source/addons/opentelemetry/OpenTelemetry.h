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

    friend class OpenTelemetry;

    public:
        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;
        Span(Span&&) = delete;
        Span& operator=(Span&&) = delete;

        ~Span();

    public:
    void End();

    public: 
        // note no const versions for a reason (would not make sense and there is no suitable conversion available for a opentelemetry::nostd::shared_ptr)
        operator opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>()
        {
            return _span;
        }

        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> operator->()
        {
            return _span;
        }

    private:
        template <typename... Args>
        OpenTelemetry::Span(Args&&... args)
            : _span()
            , _hadTraceParent(Core::JobContext::Instance().HasTraceParent())
            , _hadTraceState(Core::JobContext::Instance().HasTraceState())
            , _traceParent(_hadTraceParent ? Core::JobContext::Instance().TraceParent() : Core::string())
            , _traceState(_hadTraceState ? Core::JobContext::Instance().TraceState() : Core::string())
        {
            if (_hadTraceParent == true) {
                const auto parentSpanContext = OpenTelemetry::ToSpanContext(
                    opentelemetry::nostd::string_view(_traceParent.data(), _traceParent.length()),
                    opentelemetry::nostd::string_view(_traceState.data(), _traceState.length()));
                opentelemetry::trace::StartSpanOptions options;
                options.parent = parentSpanContext;
                _span = OpenTelemetry::Instance()._tracer->StartSpan(std::forward<Args>(args)..., options);
            } else {
                _span = OpenTelemetry::Instance()._tracer->StartSpan(std::forward<Args>(args)...);
            }
            Core::JobContext::Instance().Core::JobContext::Instance().TraceParent(OpenTelemetry::ToTraceParent(_span->GetContext()));
        }

    private:
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> _span;
        bool _hadTraceParent;
        bool _hadTraceState;
        Core::string _traceParent;
        Core::string _traceState;
    };

public:
    static OpenTelemetry& Instance();
    static void Configure(const Config& config);

 template <typename... Args>
    std::shared_ptr<OpenTelemetry::Span> StartSpan(Args&&... args)
    {
        return std::shared_ptr<OpenTelemetry::Span>(new Span(std::forward<Args>(args)...));
    }

    void Initialize();
    void Deinitialize();

private:
    OpenTelemetry()
        : _references(0)
        , _tracer()
    {
    }
    OpenTelemetry(const OpenTelemetry&) = delete;
    OpenTelemetry& operator=(const OpenTelemetry&) = delete;
    OpenTelemetry(OpenTelemetry&&) = delete;
    OpenTelemetry& operator=(OpenTelemetry&&) = delete;

private: 
    void InternalConfigure(const Config& config);

private:
    std::atomic<uint16_t> _references;
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> _tracer;
};

}
}
