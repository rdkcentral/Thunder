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
#ifdef __W3C_TRACE_CONTEXT__
#include <memory>
#include <utility>
#endif

namespace WPEFramework {
namespace Core {

    class EXTERNAL JobContext {
    public:

        JobContext(const JobContext&) = delete;
        JobContext(JobContext&&) = delete;
        JobContext& operator=(const JobContext&) = delete;
        JobContext& operator=(JobContext&&) = delete;

        static JobContext& Instance()
        {
            static thread_local JobContext instance;
            return (instance);
        }
#ifdef __W3C_TRACE_CONTEXT__

        class W3CTraceContext
        {
        public:
            W3CTraceContext(const W3CTraceContext& other) 
            : _traceparent()
            , _tracestate()
            {
                if(other._traceparent) {
                    _traceparent = std::make_unique<string>(*(other._traceparent));
                }
                if(other._tracestate) {
                    _tracestate = std::make_unique<string>(*(other._tracestate));
                }
            }
            W3CTraceContext(W3CTraceContext&&) = default;
            W3CTraceContext& operator=(const W3CTraceContext& other)
            {
                if(&other != this) {
                    if(other._traceparent) {
                        _traceparent = std::make_unique<string>(*(other._traceparent));
                    } else {
                        _traceparent.reset();
                    }
                    if(other._tracestate) {
                        _tracestate = std::make_unique<string>(*(other._tracestate));
                    } else {
                        _tracestate.reset();
                    }
                }
                return *this;
            }
            W3CTraceContext& operator=(W3CTraceContext&&) = default;

            W3CTraceContext() = default;
            W3CTraceContext(const string& traceparent, const string& tracestate) 
            : _traceparent( traceparent.empty() == true ? nullptr : std::make_unique<string>(traceparent) )
            , _tracestate( tracestate.empty() == true ? nullptr : std::make_unique<string>(tracestate) )
            {
            }
            W3CTraceContext(string&& traceparent, string&& tracestate) 
            : _traceparent( traceparent.empty() == true ? nullptr : std::make_unique<string>(std::move(traceparent)) )
            , _tracestate( tracestate.empty() == true ? nullptr : std::make_unique<string>(std::move(tracestate)) )
            {
            }

            const string& TraceParent() const
            {
                static const string empty;
                return (_traceparent ? *_traceparent : empty);
            }
            void TraceParent(const string& value)
            {
                _traceparent = std::make_unique<string>(value);
            }
            void TraceParent(string&& value)
            {
                _traceparent = std::make_unique<string>(std::move(value));
            }
            bool HasTraceParent() const
            {
                return (_traceparent != nullptr);
            }
            const string& TraceState() const
            {
                static const string empty;
                return (_tracestate ? *_tracestate : empty);
            }
            void TraceState(const string& value)
            {
                _tracestate = std::make_unique<string>(value);
            }
            void TraceState(string&& value)
            {
                _tracestate = std::make_unique<string>(std::move(value));
            }
            bool HasTraceState() const
            {
                return (_tracestate != nullptr);
            }
            void ClearTraceContext()
            {
                _traceparent.reset();
                _tracestate.reset();
            }

        private:
            std::unique_ptr<string> _traceparent;
            std::unique_ptr<string> _tracestate;
        };

        const W3CTraceContext& TraceContextW3C() const
        {
            return _tracecontext;
        }
        W3CTraceContext& TraceContextW3C()
        {
            return _tracecontext;
        }

#endif

    private:
        JobContext() = default;
        ~JobContext() = default;

#ifdef __W3C_TRACE_CONTEXT__
        W3CTraceContext _tracecontext;
#endif
    };

}
}
