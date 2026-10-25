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
#include <opentelemetry/logs/logger.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/metrics/provider.h>


namespace Thunder {
namespace Telemetry {

//***************************************************** */
//   OpenTelemetry class 
//***************************************************** */

class EXTERNAL OpenTelemetry {
private:
    static opentelemetry::trace::SpanContext ToSpanContext(opentelemetry::nostd::string_view traceparent, opentelemetry::nostd::string_view tracestate);
    static std::string ToTraceParent(const opentelemetry::trace::SpanContext& spanContext);

public:

//***************************************************** */
//   Configuration
//***************************************************** */

    enum class SamplerType : uint8_t {
        ALWAY_ON,
        ALWAY_OFF,
        PARENT_ON, // if no parent the always on sampler is used, otherwise the parent sampler is used (if this needs more combinations must be solved differently)
        PARENT_OFF,
    };

    enum class ProcessorType  : uint8_t {
        SYMCHRONOUS,
        BATCH
    };

    enum class ExporterType : uint8_t {
        CONSOLE,
        HTTP
    };

    class Config : public Core::JSON::Container {
    public:

        class Resources : public Core::JSON::Container {
        public:
            class Attribute : public Core::JSON::Container {
            public:
                Attribute()
                : Key()
                , Value()
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                }

                Attribute(const Attribute& copy)
                    : Key(copy.Key)
                    , Value(copy.Value)
                {
                    Add(_T("key"), &Key);
                    Add(_T("value"), &Value);
                }

                Attribute& operator=(const Attribute& rhs)
                {
                    if(this != &rhs) {
                        Key = rhs.Key;
                        Value = rhs.Value;
                    }
                    return (*this);
                }

                Core::JSON::String Key;
                Core::JSON::String Value;
            };

            Resources() 
            : SchemaURL()
            , Attributes()
            {
                Add(_T("schemaURL"), &SchemaURL);
                Add(_T("attributes"), &Attributes);
            }

            Core::JSON::String SchemaURL;
            Core::JSON::ArrayType<Attribute> Attributes;
        };

        class Logger : public Core::JSON::Container {
        public:
            Logger()
                : Enabled(true)
                , Endpoint()
            {
                Add(_T("enabled"), &Enabled);
                Add(_T("endpoint"), &Endpoint);
            }

            Core::JSON::Boolean Enabled;
            Core::JSON::String Endpoint;
        };

        class Metrics : public Core::JSON::Container {
        public:
            Metrics()
                : Enabled(true)
                , Endpoint()
                , IntervalMs(10000)
                , TimeoutMs(5000)
            {
                Add(_T("enabled"), &Enabled);
                Add(_T("endpoint"), &Endpoint);
                Add(_T("intervalMs"), &IntervalMs);
                Add(_T("timeoutMs"), &TimeoutMs);
            }

            Core::JSON::Boolean Enabled;
            Core::JSON::String Endpoint;
            Core::JSON::DecUInt16 IntervalMs;
            Core::JSON::DecUInt16 TimeoutMs;
        };

        Config()
            : Core::JSON::Container()
            , Sampler(SamplerType::PARENT_ON)
            , Processor(ProcessorType::SYMCHRONOUS)
            , Exporter(ExporterType::CONSOLE)
            , Resources()
            , TracerEndpoint()
            , ComponentName()
            , ComponentVersion()
            , Logger()
            , Metrics()
        {
            Add(_T("sampler"), &Sampler);
            Add(_T("processor"), &Processor);
            Add(_T("exporter"), &Exporter);
            Add(_T("resources"), &Resources);
            Add(_T("tracerendpoint"), &TracerEndpoint);
            Add(_T("componentname"), &ComponentName);
            Add(_T("componentversion"), &ComponentVersion);
            Add(_T("logger"), &Logger);
            Add(_T("metrics"), &Metrics);
        }

        Core::JSON::EnumType<SamplerType> Sampler;
        Core::JSON::EnumType<ProcessorType> Processor;
        Core::JSON::EnumType<ExporterType> Exporter;
        Resources Resources;
        Core::JSON::String TracerEndpoint;
        Core::JSON::String ComponentName;
        Core::JSON::String ComponentVersion;
        Logger Logger;
        Metrics Metrics;
    };

//***************************************************** */
//   Configuration End
//***************************************************** */

//***************************************************** */
//   Span class
//***************************************************** */

    class EXTERNAL Span {

    friend class OpenTelemetry;

    public:
        Span(const Span&) = delete;
        Span& operator=(const Span&) = delete;
        Span(Span&&) = delete;
        Span& operator=(Span&&) = delete;

        ~Span();

    public:

        //***************************************************** */
        //   Span class public interface
        //***************************************************** */


        void End();

        void Activate();
        void Deactivate();

        // note no const versions for a reason (would not make sense and there is no suitable conversion available for a opentelemetry::nostd::shared_ptr)
        operator opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>()
        {
            return _span;
        }

        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> operator->()
        {
            return _span;
        }

        //***************************************************** */
        //   Span class public interface End
        //***************************************************** */

    private:
        template <typename... Args>
        Span(Args&&... args)
            : _span()
            , _hadTraceParent(Core::JobContext::Instance().TraceContextW3C().HasTraceParent())
            , _hadTraceState(Core::JobContext::Instance().TraceContextW3C().HasTraceState())
            , _traceParent(_hadTraceParent ? Core::JobContext::Instance().TraceContextW3C().TraceParent() : Core::string())
            , _traceState(_hadTraceState ? Core::JobContext::Instance().TraceContextW3C().TraceState() : Core::string())
            , _token()
        {
            auto tracecontext = opentelemetry::trace::SpanContext::GetInvalid();
            if (_hadTraceParent == true) {
                tracecontext = OpenTelemetry::ToSpanContext(
                    opentelemetry::nostd::string_view(_traceParent.data(), _traceParent.length()),
                    opentelemetry::nostd::string_view(_traceState.data(), _traceState.length()));
            } else if (_hadTraceState == true) {
                tracecontext.trace_state()->FromHeader(_traceState);
            }
            opentelemetry::trace::StartSpanOptions options;
            options.parent = tracecontext;
            _span = OpenTelemetry::Instance()._tracer->StartSpan(std::forward<Args>(args)..., options);
            Core::JobContext::Instance().TraceContextW3C().TraceParent(OpenTelemetry::ToTraceParent(_span->GetContext()));
            // no need to get the tracestate, if will be same as the one we passed in, if any, or empty if none.
        }

    private:
        opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> _span;
        bool _hadTraceParent;
        bool _hadTraceState;
        Core::string _traceParent;
        Core::string _traceState;
        opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> _token;
    };

//***************************************************** */
//   Scope class 
//***************************************************** */

    class EXTERNAL Scope {

    friend class OpenTelemetry;

    public:
        Scope(const Scope&) = default;
        Scope& operator=(const Scope&) = default;
        Scope(Scope&&) = default;
        Scope& operator=(Scope&&) = default;

        ~Scope();

    private: 
        Scope();

    private:
        opentelemetry::nostd::unique_ptr<opentelemetry::context::Token> _token;
    };

//***************************************************** */
//   Scope class End
//***************************************************** */

//***************************************************** */
//   OpenTelemetry class public interface
//***************************************************** */


public:
    static OpenTelemetry& Instance();
    static void Configure(const Config& config);

 template <typename... Args>
    std::shared_ptr<OpenTelemetry::Span> StartSpan(Args&&... args)
    {
        return std::shared_ptr<OpenTelemetry::Span>(new Span(std::forward<Args>(args)...));
    }

    Scope StartScopeFromJobContext()
    {
        return Scope();
    }

    void Initialize();
    void Deinitialize();

    // note it is not needed to check this for any logger usage, when the logger is disabled it is connected to a NOOP logger so all calls to it will work and not fail / crash
    bool LoggerEnabled() const
    {
        return _loggerenabled;
    }

    opentelemetry::nostd::unique_ptr<opentelemetry::logs::LogRecord> CreateTraceFilledLogRecord();

    opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> Logger() 
    {
        return _logger;
    }

    // note it is not needed to check this for any metrics usage, when the logger is disabled it is connected to a NOOP logger so all calls to it will work and not fail / crash
    bool MeterEnabled() const
    {
        return _metricsenabled;
    }

    opentelemetry::nostd::shared_ptr<opentelemetry::metrics::Meter> Meter() 
    {
        return _meter;
    }


//***************************************************** */
//   OpenTelemetry class public Interface End
//***************************************************** */

private:
    OpenTelemetry()
        : _references(0)
        , _samplertype(SamplerType::PARENT_ON)
        , _processortype(ProcessorType::SYMCHRONOUS)
        , _exportertype(ExporterType::CONSOLE)
        , _tracerendpoint("http://127.0.0.1:4318/v1/traces")
        , _componentname("Thunder")
        , _componentversion("1.0.0")
        , _loggerenabled(true)
        , _loggerendpoint("http://127.0.0.1:4318/v1/logs")
        , _metricsenabled(true)
        , _metricsendpoint("http://127.0.0.1:4318/v1/metrics")
        , _metricsIntervalMs(10000)
        , _metricsTimeoutMs(5000)
        , _tracer()
        , _logger()
        , _meter()
        , _resource()
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
    SamplerType _samplertype;
    ProcessorType _processortype;
    ExporterType _exportertype;  
    string _tracerendpoint;
    string _componentname;
    string _componentversion;   
    bool _loggerenabled;
    string _loggerendpoint;
    bool _metricsenabled;
    string _metricsendpoint;
    uint16_t _metricsIntervalMs;
    uint16_t _metricsTimeoutMs;
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> _tracer;
    opentelemetry::nostd::shared_ptr<opentelemetry::logs::Logger> _logger;
    opentelemetry::nostd::shared_ptr<opentelemetry::metrics::Meter> _meter;
    opentelemetry::sdk::resource::Resource _resource;

};

//***************************************************** */
//   OpenTelemetry class End
//***************************************************** */

}
}
