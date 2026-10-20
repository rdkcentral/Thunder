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

namespace Thunder {
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
        const string& TraceParent() const
        {
            static const string empty;
            return (traceparent ? *traceparent : empty);
        }
        void TraceParent(const string& value)
        {
            traceparent = std::make_unique<string>(value);
        }
        void TraceParent(string&& value)
        {
            traceparent = std::make_unique<string>(std::move(value));
        }
        bool HasTraceParent() const
        {
            return (traceparent != nullptr);
        }
        const string& TraceState() const
        {
            static const string empty;
            return (tracestate ? *tracestate : empty);
        }
        void TraceState(const string& value)
        {
            tracestate = std::make_unique<string>(value);
        }
        void TraceState(string&& value)
        {
            tracestate = std::make_unique<string>(std::move(value));
        }
        bool HasTraceState() const
        {
            return (tracestate != nullptr);
        }
        void ClearTraceContext()
        {
            traceparent.reset();
            tracestate.reset();
        }
#endif

    private:
        JobContext() = default;
        ~JobContext() = default;

#ifdef __W3C_TRACE_CONTEXT__
        std::unique_ptr<string> traceparent;
        std::unique_ptr<string> tracestate;
#endif
    };

}
}
