# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2026 Metrological
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

find_package(opentelemetry-cpp CONFIG REQUIRED)

set(TARGET ${NAMESPACE}OpenTelemetry)

add_library(${TARGET}
    OpenTelemetry.cpp
)

set(PUBLIC_HEADERS
    Module.h
    OpenTelemetry.h
    open_telemetry.h
)

target_compile_definitions(${TARGET} PRIVATE OPENTELEMETRY_EXPORTS)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${TARGET} PUBLIC OPENTELEMETRY_STATIC)
endif()

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        opentelemetry-cpp::trace
        opentelemetry-cpp::common
        opentelemetry-cpp::metrics
        opentelemetry-cpp::resources
        opentelemetry-cpp::logs
        opentelemetry-cpp::version

    PRIVATE
        CompileSettings::CompileSettings
        CompileSettingsDebug::CompileSettingsDebug
        opentelemetry-cpp::ostream_span_exporter
        opentelemetry-cpp::ostream_span_exporter_builder
        opentelemetry-cpp::ostream_log_record_exporter
        opentelemetry-cpp::otlp_http_exporter
        opentelemetry-cpp::otlp_http_exporter_builder
        opentelemetry-cpp::otlp_http_log_record_exporter
        opentelemetry-cpp::otlp_http_log_record_exporter_builder
        opentelemetry-cpp::otlp_http_metric_exporter
        opentelemetry-cpp::otlp_http_metric_exporter_builder
        opentelemetry-cpp::ostream_metrics_exporter
        opentelemetry-cpp::ostream_metrics_exporter_builder
        opentelemetry-cpp::http_client_curl
        opentelemetry-cpp::http_client
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/open_telemetry>
)

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

install(
    TARGETS ${TARGET}  EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/open_telemetry COMPONENT ${NAMESPACE}_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Enable ease of development for Thunder Open Telemetry support." )
