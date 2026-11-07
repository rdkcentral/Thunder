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
#include <limits>

#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/exporters/ostream/metric_exporter_factory.h>
#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include "opentelemetry/sdk/trace/samplers/always_off.h"
#include "opentelemetry/sdk/trace/samplers/always_off_factory.h"
#include "opentelemetry/sdk/trace/samplers/always_on.h"
#include "opentelemetry/sdk/trace/samplers/always_on_factory.h"
#include "opentelemetry/sdk/trace/samplers/parent.h"
#include "opentelemetry/sdk/trace/samplers/parent_factory.h"
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/trace/provider.h"

#include "opentelemetry/sdk/metrics/aggregation/default_aggregation.h"
#include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"

#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_log_record_exporter_options.h"
#include "opentelemetry/sdk/logs/processor.h"
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/sdk/logs/batch_log_record_processor.h>
#include <opentelemetry/sdk/logs/batch_log_record_processor_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_log_record_exporter.h>
#include <opentelemetry/exporters/ostream/log_record_exporter.h>
#include <opentelemetry/exporters/ostream/log_record_exporter_factory.h>
#include <opentelemetry/logs/noop.h>

#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter_options.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h>
#include <opentelemetry/sdk/metrics/meter_context_factory.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include "opentelemetry/sdk/metrics/provider.h"


namespace logs                          = opentelemetry::logs;
namespace trace                         = opentelemetry::trace;
namespace sdklogs                       = opentelemetry::sdk::logs;
namespace sdktrace                      = opentelemetry::sdk::trace;
namespace sdkmetrics                    = opentelemetry::sdk::metrics;
namespace propagation                   = opentelemetry::context::propagation;
namespace ostream_exporter              = opentelemetry::exporter::trace;
namespace ostream_logs_exporter         = opentelemetry::exporter::logs;
namespace ostream_metrics_exporter      = opentelemetry::exporter::metrics;
namespace nostd                         = opentelemetry::nostd;
namespace otlp_exporter                 = opentelemetry::exporter::otlp;
namespace otlp_context                  = opentelemetry::context;
namespace resource                      = opentelemetry::sdk::resource;
namespace metrics                       = opentelemetry::metrics;
namespace metrics_api                   = opentelemetry::metrics;
namespace metrics_sdk                   = opentelemetry::sdk::metrics;


namespace Thunder {

ENUM_CONVERSION_BEGIN(Telemetry::OpenTelemetry::SamplerType)

    { Telemetry::OpenTelemetry::SamplerType::ALWAYS_ON, _TXT("AlwaysOn") },
    { Telemetry::OpenTelemetry::SamplerType::ALWAYS_OFF, _TXT("AlwaysOff") },
    { Telemetry::OpenTelemetry::SamplerType::PARENT_ON, _TXT("ParentOn") },
    { Telemetry::OpenTelemetry::SamplerType::PARENT_OFF, _TXT("ParentOff") },

    ENUM_CONVERSION_END(Telemetry::OpenTelemetry::SamplerType)

ENUM_CONVERSION_BEGIN(Telemetry::OpenTelemetry::ProcessorType)

    { Telemetry::OpenTelemetry::ProcessorType::SYNCHRONOUS, _TXT("Synchronous") },
    { Telemetry::OpenTelemetry::ProcessorType::BATCH, _TXT("Batch") },

ENUM_CONVERSION_END(Telemetry::OpenTelemetry::ProcessorType )

ENUM_CONVERSION_BEGIN(Telemetry::OpenTelemetry::ExporterType)

    { Telemetry::OpenTelemetry::ExporterType::CONSOLE, _TXT("Console") },
    { Telemetry::OpenTelemetry::ExporterType::HTTP, _TXT("HTTP") },

ENUM_CONVERSION_END(Telemetry::OpenTelemetry::ExporterType) 
    
namespace Telemetry
{

OpenTelemetry& OpenTelemetry::Instance()
{
    static OpenTelemetry instance;
    return (instance);
}

void OpenTelemetry::Configure(const Config& config)
{
    OpenTelemetry::Instance().InternalConfigure(config);
}

void OpenTelemetry::InternalConfigure(const OpenTelemetry::Config& config)
{
    _adminLock.Lock();

    if (config.Sampler.IsSet()) {
        _samplertype = config.Sampler.Value();
    }
    if (config.Processor.IsSet()) {
        _processortype = config.Processor.Value();
    }
    if (config.Exporter.IsSet()) {
        _exportertype = config.Exporter.Value();
    }
    if (config.TracerEndpoint.IsSet()) {
        _tracerendpoint = config.TracerEndpoint.Value();
    }
    if (config.ComponentName.IsSet() == true) {
        _componentname = config.ComponentName.Value();
    }
    if (config.ComponentVersion.IsSet() == true) {
        _componentversion = config.ComponentVersion.Value();
    }   
    if (config.Logger.Enabled.IsSet() == true) {
        _loggerenabled = config.Logger.Enabled.Value();
    }
    if (config.Logger.Endpoint.IsSet()) {
        _loggerendpoint = config.Logger.Endpoint.Value();
    }
    if (config.Metrics.Enabled.IsSet() == true) {
        _metricsenabled = config.Metrics.Enabled.Value();
    }
    if (config.Metrics.Endpoint.IsSet()) {
        _metricsendpoint = config.Metrics.Endpoint.Value();
    }
    if (config.Metrics.IntervalMs.IsSet() == true) {
        _metricsIntervalMs = config.Metrics.IntervalMs.Value();
    }
    if (config.Metrics.TimeoutMs.IsSet()) {
        _metricsTimeoutMs = config.Metrics.TimeoutMs.Value();
    }
    if(config.Resources.IsSet()) {
        resource::ResourceAttributes attributes;
        Core::JSON::ArrayType<Config::Resources::Attribute>::ConstIterator index(config.Resources.Attributes.Elements());
        while (index.Next() == true) {
            const Config::Resources::Attribute& element(index.Current());
            if((element.Key.IsSet() == true) && (element.Value.IsSet()) ) {
                attributes.SetAttribute(static_cast<string>(element.Key.Value()), element.Value.Value());
            } 
        }
        if(config.Resources.SchemaURL.IsSet() == true) {
            _resource = resource::Resource::Create(attributes, config.Resources.SchemaURL.Value());
        } else {
            _resource = resource::Resource::Create(attributes);
        }
    }

    _adminLock.Unlock();

}

void OpenTelemetry::Initialize()
{
    _adminLock.Lock();

    ASSERT(_references < std::numeric_limits<decltype(_references)>::max());

    if (++_references == 1) {

        std::unique_ptr<sdktrace::SpanExporter> exporter;
        std::unique_ptr<sdktrace::SpanProcessor> processor;
        std::unique_ptr<sdktrace::Sampler> sampler;

        switch (_exportertype)
        {
            case ExporterType::CONSOLE:
                exporter = ostream_exporter::OStreamSpanExporterFactory::Create();
                break;
            case ExporterType::HTTP: 
                {
                    otlp_exporter::OtlpHttpExporterOptions exporter_opts;
                    exporter_opts.url = _tracerendpoint;
                    exporter_opts.content_type = otlp_exporter::HttpRequestContentType::kJson;
                    exporter = otlp_exporter::OtlpHttpExporterFactory::Create(exporter_opts);
                }
                break;  
            default:
                ASSERT(true);
        }

        switch (_processortype)
        {
            case ProcessorType::SYNCHRONOUS:
                processor = sdktrace::SimpleSpanProcessorFactory::Create(std::move(exporter)); 
                break;
            case ProcessorType::BATCH:
                {
                    opentelemetry::sdk::trace::BatchSpanProcessorOptions batch_opts;
                    batch_opts.max_queue_size = 2048; 
                    batch_opts.max_export_batch_size = 512; 
                    batch_opts.schedule_delay_millis = std::chrono::milliseconds(1000); 
                    processor = sdktrace::BatchSpanProcessorFactory::Create(std::move(exporter), batch_opts);
                }
                break;
            default:
                ASSERT(true);
        }

        switch (_samplertype)
        {
            case SamplerType::ALWAYS_ON:
                sampler = sdktrace::AlwaysOnSamplerFactory::Create();
                break;
            case SamplerType::ALWAYS_OFF:
                sampler = sdktrace::AlwaysOffSamplerFactory::Create();
                break;
            case SamplerType::PARENT_ON: 
                {
                    auto delegatedsampler = sdktrace::AlwaysOnSamplerFactory::Create();
                    sampler = sdktrace::ParentBasedSamplerFactory::Create(std::shared_ptr<sdktrace::Sampler>(delegatedsampler.release()));
                }
                break;
            case SamplerType::PARENT_OFF: 
                {
                    auto delegatedsampler = sdktrace::AlwaysOffSamplerFactory::Create();
                    sampler = sdktrace::ParentBasedSamplerFactory::Create(std::shared_ptr<sdktrace::Sampler>(delegatedsampler.release()));
                } 
                break;
            default:
                ASSERT(true);
        }

        nostd::shared_ptr<trace::TracerProvider> provider(sdktrace::TracerProviderFactory::Create(std::move(processor), _resource, std::move(sampler)).release());
        trace::Provider::SetTracerProvider(provider);
        _tracer = provider->GetTracer(_componentname, _componentversion); 

        if (_loggerenabled == true) {
            std::unique_ptr<sdklogs::LogRecordExporter> logexporter;
            std::unique_ptr<sdklogs::LogRecordProcessor> logprocessor;

            switch (_exportertype) {
            case ExporterType::CONSOLE:
                logexporter = ostream_logs_exporter::OStreamLogRecordExporterFactory::Create();
                break;
            case ExporterType::HTTP: {
                otlp_exporter::OtlpHttpLogRecordExporterOptions logexporter_opts;
                logexporter_opts.url = _loggerendpoint;
                logexporter_opts.content_type = otlp_exporter::HttpRequestContentType::kJson;
                logexporter = otlp_exporter::OtlpHttpLogRecordExporterFactory::Create(logexporter_opts);
            } break;
            default:
                ASSERT(true);
            }

            switch (_processortype) {
            case ProcessorType::SYNCHRONOUS:
                logprocessor = sdklogs::SimpleLogRecordProcessorFactory::Create(std::move(logexporter));
                break;
            case ProcessorType::BATCH:
                {
                    opentelemetry::sdk::logs::BatchLogRecordProcessorOptions batch_opts;
                    batch_opts.max_queue_size = 2048;
                    batch_opts.max_export_batch_size = 512;
                    batch_opts.schedule_delay_millis = std::chrono::milliseconds(1000);
                    logprocessor = sdklogs::BatchLogRecordProcessorFactory::Create(std::move(logexporter), batch_opts);
                }
                break;

            default:
                ASSERT(true);
            }

            nostd::shared_ptr<logs::LoggerProvider> loggerprovider(sdklogs::LoggerProviderFactory::Create(std::move(logprocessor), _resource).release());
            logs::Provider::SetLoggerProvider(loggerprovider);
            _logger = loggerprovider->GetLogger("Thunder Logger", _componentname, _componentversion); 
        } else {
            _logger = nostd::shared_ptr < logs::Logger>( new logs::NoopLogger());
        }

        if (_metricsenabled == true) {
            std::unique_ptr<metrics_sdk::PushMetricExporter> metricsexporter;

            switch (_exportertype) {
            case ExporterType::CONSOLE:
                metricsexporter = ostream_metrics_exporter::OStreamMetricExporterFactory::Create();
                break;
            case ExporterType::HTTP: {
                otlp_exporter::OtlpHttpMetricExporterOptions metricsexporter_opts;
                metricsexporter_opts.url = _metricsendpoint;
                metricsexporter_opts.content_type = otlp_exporter::HttpRequestContentType::kJson;
                metricsexporter = otlp_exporter::OtlpHttpMetricExporterFactory::Create(metricsexporter_opts);
            } break;
            default:
                ASSERT(true);
            }

            // for now only one exporter supported (are there more?)
            metrics_sdk::PeriodicExportingMetricReaderOptions reader_opts;
            reader_opts.export_interval_millis = std::chrono::milliseconds(_metricsIntervalMs);
            reader_opts.export_timeout_millis = std::chrono::milliseconds(_metricsTimeoutMs);
            auto reader = metrics_sdk::PeriodicExportingMetricReaderFactory::Create(std::move(metricsexporter), reader_opts);

            // The default MeterContext resource merges OTEL_SERVICE_NAME and
            // OTEL_RESOURCE_ATTRIBUTES from the environment.
            auto context = metrics_sdk::MeterContextFactory::Create();
            context->AddMetricReader(std::move(reader));

            auto provider = metrics_sdk::MeterProviderFactory::Create(std::move(context));
            std::shared_ptr<metrics_api::MeterProvider> api_provider(std::move(provider));
            metrics_sdk::Provider::SetMeterProvider(api_provider);

            _meter = metrics_api::Provider::GetMeterProvider()->GetMeter(_componentname, _componentversion);

        } else {
            _meter = nostd::shared_ptr<metrics::Meter>(new metrics::NoopMeter());
        }
    }

    _adminLock.Unlock();

}

void OpenTelemetry::Deinitialize()
{
    _adminLock.Lock();

    ASSERT(_references > 0);
    --_references;

    _adminLock.Unlock();
}

opentelemetry::nostd::unique_ptr<opentelemetry::logs::LogRecord> OpenTelemetry::CreateTraceFilledLogRecord()
{
    auto logrecord = _logger->CreateLogRecord();
    if (LoggerEnabled() && Core::JobContext::Instance().TraceContextW3C().HasTraceParent() == true) {
        auto spancontext = OpenTelemetry::ToSpanContext(
            opentelemetry::nostd::string_view(Core::JobContext::Instance().TraceContextW3C().TraceParent().data(), Core::JobContext::Instance().TraceContextW3C().TraceParent().length()),
            opentelemetry::nostd::string_view(Core::JobContext::Instance().TraceContextW3C().TraceState().data(), Core::JobContext::Instance().TraceContextW3C().TraceState().length()));
        logrecord->SetTraceId(spancontext.trace_id());
        logrecord->SetSpanId(spancontext.span_id());
        logrecord->SetTraceFlags(spancontext.trace_flags());
    }
    return logrecord;
}

OpenTelemetry::Span::~Span()
{
    End();
}

void OpenTelemetry::Span::End()
{
    Deactivate();
    _span->End();
    Core::JobContext::Instance().TraceContextW3C().ClearTraceContext();
    if (_hadTraceParent == true) {
        Core::JobContext::Instance().TraceContextW3C().TraceParent(std::move(_traceParent));
        _hadTraceParent = false;
    }
    if (_hadTraceState == true) {
        Core::JobContext::Instance().TraceContextW3C().TraceState(std::move(_traceState));
        _hadTraceState = false;
    }
}

void OpenTelemetry::Span::Activate()
{
    _token = otlp_context::RuntimeContext::Attach(otlp_context::RuntimeContext::GetCurrent().SetValue(trace::kSpanKey, _span));
}

void OpenTelemetry::Span::Deactivate()
{
    
    _token.reset();
}     


OpenTelemetry::Scope::Scope()
    : _token()
{
    auto tracecontext = opentelemetry::trace::SpanContext::GetInvalid();
    if (Core::JobContext::Instance().TraceContextW3C().HasTraceParent() == true) {
        tracecontext = OpenTelemetry::ToSpanContext(
            opentelemetry::nostd::string_view(Core::JobContext::Instance().TraceContextW3C().TraceParent().data(), Core::JobContext::Instance().TraceContextW3C().TraceParent().length()),
            opentelemetry::nostd::string_view(Core::JobContext::Instance().TraceContextW3C().TraceState().data(), Core::JobContext::Instance().TraceContextW3C().TraceState().length()));
    } else if (Core::JobContext::Instance().TraceContextW3C().HasTraceState() == true) {
        tracecontext.trace_state()->FromHeader(Core::JobContext::Instance().TraceContextW3C().TraceState());
    }

    opentelemetry::nostd::shared_ptr<trace::SpanContext> spancontext(new trace::SpanContext(tracecontext));
    _token = otlp_context::RuntimeContext::Attach(otlp_context::RuntimeContext::GetCurrent().SetValue(trace::kSpanKey, spancontext));
}

OpenTelemetry::Scope::~Scope()
{
    _token.reset();
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
