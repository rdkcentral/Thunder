# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(config)
include(CompileSettingsDebug)

option(CORE
        "Include the core library." ON)
option(CRYPTALGO
        "Include the encyption algorithm library." ON)
option(WEBSOCKET
        "Include the websocket library." ON)
option(MESSAGING
        "Include the messaging library." ON)
option(COM
        "Include the COM library." ON)
option(PROCESS
        "Include the COM hosting process executable." ON)
option(PLUGINS
        "Include plugins library." ON)
option(EXECUTABLE
        "Include the hosting executable." ON)
option(STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)
option(WCHAR_SUPPORT
        "Enable support for WCHAR." OFF)
option(DISABLE_TRACING
        "Disable tracing in debug" OFF)
set(ENABLED_TRACING_LEVEL 0 CACHE STRING "If tracing is enabled, this sets its default level.")
option(EXCEPTION_CATCHING
        "Enable unhandled exception handling catching." OFF)
option(DEADLOCK_DETECTION
        "Enable deadlock detection tooling." OFF)
option(DISABLE_USE_COMPLEMENTARY_CODE_SET
        "Disable the complementary code set" OFF)
option(ENABLE_JSONRPC_FORGIVING_METHOD_CASE_HANDLING
        "Enable json rpc forgiving camel and pascal case method handling" OFF)
option(ACCEPT_VERSION_IN_CALLSIGN
        "Accept version in callsign syntax (version is ignored)" OFF)
option(W3C_TRACE_CONTEXT_ENABLED
        "allow propagation of W3C trace context" OFF)
option(CUSTOM_ERROR_CODES_ENABLED
        "enable custom error codes (obsolete)" OFF)
option(THUNDER_CORE_RESOURCE_MONITOR_WATCHDOG
        "Compile in the WatchDog supervision of the ResourceMonitor (runtime enabled via the Thunder configuration)" ON)
option(THUNDER_CORE_WORKERPOOL_WATCHDOG
        "Compile in the WatchDog supervision of the WorkerPool (runtime enabled via the Thunder configuration)" ON)
option(SLIM_WARNING_REPORTING
    "Only report Invoke durations and WorkerPool warnings." OFF)

set(WARNING_REPORTING_INVOKE_RPC_REPORT_BOUND 250 CACHE STRING "COM Invoke reporting threshold in milliseconds")
set(WARNING_REPORTING_INVOKE_RPC_WARNING_BOUND 750 CACHE STRING "COM Invoke warning threshold in milliseconds")
set(WARNING_REPORTING_INVOKE_MESSAGE_REPORT_BOUND 500 CACHE STRING "JSON-RPC Invoke reporting threshold in milliseconds")
set(WARNING_REPORTING_INVOKE_MESSAGE_WARNING_BOUND 500 CACHE STRING "JSON-RPC Invoke warning threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_FINISH_REPORT_BOUND 5000 CACHE STRING "WorkerPool execution reporting threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_FINISH_WARNING_BOUND 7500 CACHE STRING "WorkerPool execution warning threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_QUEUE_REPORT_BOUND 200 CACHE STRING "WorkerPool queue reporting threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_QUEUE_WARNING_BOUND 400 CACHE STRING "WorkerPool queue warning threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_ACTIVE_REPORT_BOUND 15000 CACHE STRING "Active WorkerPool job reporting threshold in milliseconds")
set(WARNING_REPORTING_WORKERPOOL_ACTIVE_WARNING_BOUND 20000 CACHE STRING "Active WorkerPool job warning threshold in milliseconds")
set(WARNING_REPORTING_PLUGIN_STATE_REPORT_BOUND 5000 CACHE STRING "Plugin state change reporting threshold in milliseconds")
set(WARNING_REPORTING_PLUGIN_STATE_WARNING_BOUND 5000 CACHE STRING "Plugin state change warning threshold in milliseconds")

if(SLIM_WARNING_REPORTING)
    set(WARNING_REPORTING ON CACHE BOOL "Include warning reporting in the build." FORCE)
endif()

if(WARNING_REPORTING)
    if(WARNING_REPORTING_INVOKE_RPC_WARNING_BOUND LESS WARNING_REPORTING_INVOKE_RPC_REPORT_BOUND OR
        WARNING_REPORTING_INVOKE_MESSAGE_WARNING_BOUND LESS WARNING_REPORTING_INVOKE_MESSAGE_REPORT_BOUND OR
        WARNING_REPORTING_WORKERPOOL_FINISH_WARNING_BOUND LESS WARNING_REPORTING_WORKERPOOL_FINISH_REPORT_BOUND OR
        WARNING_REPORTING_WORKERPOOL_QUEUE_WARNING_BOUND LESS WARNING_REPORTING_WORKERPOOL_QUEUE_REPORT_BOUND OR
        WARNING_REPORTING_WORKERPOOL_ACTIVE_WARNING_BOUND LESS WARNING_REPORTING_WORKERPOOL_ACTIVE_REPORT_BOUND OR
        WARNING_REPORTING_PLUGIN_STATE_WARNING_BOUND LESS WARNING_REPORTING_PLUGIN_STATE_REPORT_BOUND)
        message(FATAL_ERROR "Warning bounds must be greater than or equal to their report bounds")
    endif()
endif()

if(HIDE_NON_EXTERNAL_SYMBOLS)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

if(DISABLE_USE_COMPLEMENTARY_CODE_SET)
    add_definitions(-D__DISABLE_USE_COMPLEMENTARY_CODE_SET__)
    message(STATUS "complementary code set disabled")
endif()

if(CORE)
    add_subdirectory(core)
endif()

if(CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(WEBSOCKET)
    add_subdirectory(websocket)
endif()

if(MESSAGING)
    add_subdirectory(messaging)
endif()

add_subdirectory(addons)

if(COM)
    add_subdirectory(com)
endif()

if(PLUGINS)
    add_subdirectory(plugins)
    add_subdirectory(common)
endif()

if(PROCESS)
    add_subdirectory(ThunderPlugin)
endif()

if(EXECUTABLE)
    add_subdirectory(Thunder)
endif()
