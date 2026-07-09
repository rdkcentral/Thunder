 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "JSONRPCLink.h"

// RDK OpenTelemetry trace propagation wrapper
// These functions live in the websocket .so, so downstream consumers
// resolve them via the shared library without needing to link librdk_otlp directly.
#if __has_include("rdk_otlp_instrumentation.h")
#include "rdk_otlp_instrumentation.h"
#define RDK_OTEL_JSONRPC_IMPL 1
#else
#define RDK_OTEL_JSONRPC_IMPL 0
#endif

namespace WPEFramework {

    EXTERNAL const char* OtelGetCurrentTraceparent() {
#if RDK_OTEL_JSONRPC_IMPL
        return rdk_otlp_get_current_traceparent();
#else
        return nullptr;
#endif
    }
} // namespace WPEFramework

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(WPEFramework::JSONRPC::JSONPluginState)

    { WPEFramework::JSONRPC::DEACTIVATED, _TXT("Deactivated") },
    { WPEFramework::JSONRPC::ACTIVATED, _TXT("Activated") },

ENUM_CONVERSION_END(WPEFramework::JSONRPC::JSONPluginState)

}
