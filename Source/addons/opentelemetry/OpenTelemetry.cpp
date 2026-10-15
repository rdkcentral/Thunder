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

#include "OpenTelemetry.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/exporters/ostream/metric_exporter_factory.h>
#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/logs/logger.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include "opentelemetry/sdk/trace/samplers/always_off.h"
#include "opentelemetry/sdk/trace/samplers/always_on.h"

namespace logs             = opentelemetry::logs;
namespace trace            = opentelemetry::trace;
namespace metrics          = opentelemetry::metrics;
namespace sdklogs          = opentelemetry::sdk::logs;
namespace sdktrace         = opentelemetry::sdk::trace;
namespace sdkmetrics       = opentelemetry::sdk::metrics;
namespace propagation      = opentelemetry::context::propagation;
namespace ostream_exporter = opentelemetry::exporter::trace;
namespace nostd            = opentelemetry::nostd;

namespace Thunder {
namespace Telemetry {

OpenTelemetry& OpenTelemetry::Instance()
{
    static OpenTelemetry instance;
    return (instance);
}

void OpenTelemetry::Initialize(const Config& config)
{
    // it is not an issue if this causes a race condition with Deinitialize() 
    if (_references++ == 0) {
        _shutdownTimeoutMicroseconds = config.Timeout.Value();
        auto exporter  = ostream_exporter::OStreamSpanExporterFactory::Create();
/*    auto processor = sdktrace::BatchSpanProcessorFactory::Create(
        std::move(exporter), sdktrace::BatchSpanProcessorOptions{}); */
       auto sampler = std::make_unique<sdktrace::AlwaysOnSampler>();
       auto processor = sdktrace::SimpleSpanProcessorFactory::Create(
                std::move(exporter)); 
        auto sdk_provider = sdktrace::TracerProviderFactory::Create(std::move(processor), opentelemetry::sdk::resource::Resource::Create({}), std::move(sampler));
    nostd::shared_ptr<trace::TracerProvider> provider{
        std::shared_ptr<trace::TracerProvider>(std::move(sdk_provider))};
    trace::Provider::SetTracerProvider(provider);
    _tracer = provider->GetTracer("ThunderOpenTelemetry", "1.0.0"); // toto name and get version from Thunder
    }
}

void OpenTelemetry::Deinitialize()
{
    // nothing to do
}

OpenTelemetry::Span::~Span()
{
    if (_active == true) {
        if (_span != nullptr) {
            _span->End();
        }
        _scope.reset();
        _jobContext.ClearTraceContext();
        if (_hadTraceParent == true) {
            _jobContext.TraceParent(std::move(_traceParent));
        }
        if (_hadTraceState == true) {
            _jobContext.TraceState(std::move(_traceState));
        }
    }
}

trace::SpanContext OpenTelemetry::ToSpanContext(
    nostd::string_view traceparent,
    nostd::string_view tracestate)
{
    ASSERT(traceparent.size() == 55);

    auto trace_id = trace::propagation::HttpTraceContext::TraceIdFromHex(
        nostd::string_view{traceparent.data() + 3, 2 * trace::TraceId::kSize});
    auto span_id = trace::propagation::HttpTraceContext::SpanIdFromHex(
        nostd::string_view{traceparent.data() + 36, 2 * trace::SpanId::kSize});
    auto trace_flags = trace::propagation::HttpTraceContext::TraceFlagsFromHex(
        nostd::string_view{traceparent.data() + 53, 2});
    auto trace_state = trace::TraceState::FromHeader(tracestate);

    auto x = trace::SpanContext{ trace_id, span_id, trace_flags, true, std::move(trace_state) };

    return x;
}

std::string OpenTelemetry::ToTraceParent(const trace::SpanContext &span_context)
{
    char traceparent[55];
    traceparent[0] = '0';
    traceparent[1] = '0';
    traceparent[2] = '-';
    span_context.trace_id().ToLowerBase16(
        nostd::span<char, 2 * trace::TraceId::kSize>{&traceparent[3], 2 * trace::TraceId::kSize});
    traceparent[35] = '-';
    span_context.span_id().ToLowerBase16(
        nostd::span<char, 2 * trace::SpanId::kSize>{&traceparent[36], 2 * trace::SpanId::kSize});
    traceparent[52] = '-';
    span_context.trace_flags().ToLowerBase16(nostd::span<char, 2>{&traceparent[53], 2});

    return {traceparent, sizeof(traceparent)};
}


}
}
