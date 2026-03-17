# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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

# ============================================================================
# thunder_test_support - Static library for Thunder plugin integration testing
#
# This library embeds the Thunder PluginHost server (PluginServer, Controller,
# SystemInfo, etc.) into a static archive so that GTest-based test binaries
# can spin up a real Thunder runtime in-process without launching the
# standalone Thunder daemon.
#
# Usage:
#   1. Link your test executable against thunder_test_support.
#   2. Use ThunderTestRuntime::Initialize() to start the embedded server.
#   3. Call JSON-RPC or COM-RPC methods directly against loaded plugins.
#   4. Call ThunderTestRuntime::Shutdown() when done.
#
# NOTE: PluginHost.cpp is deliberately excluded — it contains main().
#       The test binary provides its own main() via GTest.
# ============================================================================

find_package(Threads REQUIRED)

set(THUNDER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Thunder")

set(THREADPOOL_COUNT "4" CACHE STRING "The number of threads in the thread pool for test runtime")

add_library(thunder_test_support STATIC
    ThunderTestRuntime.cpp
    Module.cpp
    ${THUNDER_SOURCE_DIR}/PluginServer.cpp
    ${THUNDER_SOURCE_DIR}/Controller.cpp
    ${THUNDER_SOURCE_DIR}/SystemInfo.cpp
    ${THUNDER_SOURCE_DIR}/PostMortem.cpp
    ${THUNDER_SOURCE_DIR}/Probe.cpp
)

# PluginHost.cpp is deliberately excluded — it contains main().
# The test binary supplies its own main via GTest/GMock.

target_compile_definitions(thunder_test_support
    PRIVATE
        NAMESPACE=${NAMESPACE}
        APPLICATION_NAME=ThunderTestRuntime
        MODULE_NAME=ThunderTestRuntime
        THREADPOOL_COUNT=${THREADPOOL_COUNT}
)

target_compile_options(thunder_test_support PRIVATE -Wno-psabi)

target_link_libraries(thunder_test_support
    PRIVATE
        CompileSettings::CompileSettings
)

if(EXCEPTION_CATCHING)
    set_source_files_properties(${THUNDER_SOURCE_DIR}/PluginServer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")
endif()

target_include_directories(thunder_test_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${THUNDER_SOURCE_DIR}
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/plugins/generated/jsonrpc>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/Thunder/generated>
)

target_link_libraries(thunder_test_support
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess
        Threads::Threads
)

if(WARNING_REPORTING)
    target_sources(thunder_test_support PRIVATE ${THUNDER_SOURCE_DIR}/WarningReportingCategories.cpp)
endif()

if(PROCESSCONTAINERS)
    target_link_libraries(thunder_test_support
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(thunder_test_support
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif()

if(HIBERNATESUPPORT)
    target_link_libraries(thunder_test_support PUBLIC
        ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate)
    target_compile_definitions(thunder_test_support PUBLIC
        HIBERNATE_SUPPORT_ENABLED=1)
endif()

# --- Install rules ---
install(TARGETS thunder_test_support
    ARCHIVE DESTINATION lib
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ThunderTestRuntime.h
    DESTINATION include/thunder_test_support
)
