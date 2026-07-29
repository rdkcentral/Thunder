# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2024 Metrological
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

set(TARGET ${NAMESPACE}DistributedTracing)

find_library(RDK_OTEL_LIB rdk_otlp)
if(NOT RDK_OTEL_LIB)
    message(FATAL_ERROR "DISTRIBUTED_TRACING enabled but librdk_otlp not found")
endif()

add_library(${TARGET} SHARED
    DistributedTracing.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        THUNDER_DISTRIBUTED_TRACING
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        ${CMAKE_SYSROOT}/usr/include
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${RDK_OTEL_LIB}
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    SOVERSION ${VERSION_MAJOR}
)

install(
    TARGETS ${TARGET} EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION lib COMPONENT libs
    LIBRARY DESTINATION lib COMPONENT libs
)

install(
    FILES DistributedTracing.h
    DESTINATION include/${NAMESPACE}/distributedtracing
)

add_library(${TARGET}::${TARGET} ALIAS ${TARGET})
