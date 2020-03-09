# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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
option(BLUETOOTH
        "Include the bluetooth library." OFF)
option(BROADCAST
        "Include the broadcasting library." OFF)
option(WEBSOCKET
        "Include the websocket library." OFF)
option(PROTOCOLS
        "Include the protocols library." ON)
option(TRACING
        "Include the tracing library." ON)
option(PROFILER
        "Include the profiler library." OFF)
option(PROVISIONPROXY
        "Include the provisionproxy library." OFF)
option(SECURITYAGENT
        "Include the securityagent library." OFF)
option(COM
        "Include the COM library." OFF)
option(PROTOCOLS
        "Include Protocols lib." ON)
option(CDMI
        "Include OpenCDM interface." OFF)
option(PROCESS
        "Include the COM hosting process executable." ON)
option(PROXY_STUB
        "Include proxy/stubs." ON)
option(PLUGINS
        "Include plugins library." ON)
option(INTERFACES
        "Include interfaces library." ON)
option(EXECUTABLE
        "Include the hosting executable." ON)
option(VIRTUALINPUT
        "Include VirtualInput for external applications." OFF)
option(COMPOSITORCLIENT
        "Include a graphics backend abstraction for external applications." OFF)
option(GSTREAMERCLIENT
        "Include a gstreamer sink abstraction for external applications." OFF)
option(PROCESSCONTAINERS
        "Include containerization in the build." OFF)
option(TEST_APPS
        "Include test applications." OFF)
option(UNIT_TESTS
        "Include unit tests (new gtest based)." OFF)
option(TEST_MEASUREMENTS
        "Include code coverage measurments." OFF)
option(STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)
option(BUILD_TYPE
        "How do you want to build it, Production, Release or Debug" Production)

#
# Build type specific options
#
if(NOT BUILD_TYPE)
    set(BUILD_TYPE Production)
    message(AUTHOR_WARNING "BUILD_TYPE not set, assuming '${BUILD_TYPE}'")
endif()
if("${BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
elseif("${BUILD_TYPE}" STREQUAL "DebugOptimized")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
elseif("${BUILD_TYPE}" STREQUAL "ReleaseSymbols")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
elseif("${BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
elseif("${BUILD_TYPE}" STREQUAL "Production")
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build." FORCE)
else()
    message(FATAL_ERROR "Invalid BUILD_TYPE: '${BUILD_TYPE}'")
endif()

set(EXTERN_EVENTS ${EXTERN_EVENTS} CACHE INTERNAL
        "List of events assumed to be provided by other plugins.")
string(REPLACE " " ";" EXTERN_EVENT_LIST "${EXTERN_EVENTS}")
message("-- Assuming external events: ${EXTERN_EVENT_LIST}")

#FIXME: We shouldn't touch cflags...
if(TEST_MEASUREMENTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS -lgcov)
endif ()

if(CORE)
    add_subdirectory(core)
endif()

if(CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(PROCESSCONTAINERS)
    add_subdirectory(processcontainers)
endif()

if(BROADCAST)
    add_subdirectory(broadcast)
endif()

if(PROVISIONPROXY)
    add_subdirectory(provisionproxy)
endif()

if(SECURITYAGENT)
    add_subdirectory(securityagent)
endif()

if(CDMI)
    add_subdirectory(ocdm)
endif()

if(WEBSOCKET)
    add_subdirectory(websocket)
endif()

if(TRACING)
    add_subdirectory(tracing)
endif()

if(BLUETOOTH)
    add_subdirectory(bluetooth)
endif()

if(PROFILER)
    message(FATAL_ERROR "PROFILER option does not exist?")
    set(PROFILER_INCLUDE_DIR "${ROOT}/Source/profiler")
    add_subdirectory(profiler)
endif()

if(COM)
    add_subdirectory(com)
endif()

if(PROTOCOLS)
    add_subdirectory(protocols)
endif()

if(PROXY_STUB)
    add_subdirectory(proxystubs)
endif()

if(PLUGINS)
    add_subdirectory(plugins)
endif()

if(INTERFACES)
    add_subdirectory(interfaces)
endif()

if(VIRTUALINPUT)
    add_subdirectory(virtualinput)
endif()

if(COMPOSITORCLIENT)
    add_subdirectory(compositorclient)
endif()

if(GSTREAMERCLIENT)
    add_subdirectory(gstreamerclient)
endif()

if(PROCESS)
    add_subdirectory(WPEProcess)
endif()

if(EXECUTABLE)
    add_subdirectory(WPEFramework)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
        DESTINATION include/${NAMESPACE})
