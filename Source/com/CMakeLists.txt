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

set(TARGET ${NAMESPACE}COM)
set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
ProxyStubGenerator(NAMESPACE "WPEFramework::RPC" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/ICOM.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated" PROJECT_DIR ${PROJECT_DIR})

add_library(${TARGET} SHARED
    Administrator.cpp
    Communicator.cpp
    IUnknown.cpp
    ConnectorType.cpp
    Module.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_COM.cpp
)
  
set(PUBLIC_HEADERS
    Administrator.h
    com.h
    Communicator.h
    ConnectorType.h
    Ids.h
    IteratorType.h
    IIteratorType.h
    IStringIterator.h
    IValueIterator.h
    ICOM.h
    IUnknown.h
    Messages.h
    ProxyStubs.h
    Module.h
)

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Messaging::${NAMESPACE}Messaging
          CompileSettingsDebug::CompileSettingsDebug
          ${CMAKE_DL_LIBS}
        )

# rdk_otlp is intentionally NOT hard-linked here.
# libWPEFrameworkCOM.so is loaded by many processes (WPEProcess, IARM daemons, etc.)
# that must not have librdk_otlp force-loaded into them.
# IUnknown.cpp uses dlopen(RTLD_NOLOAD) to resolve symbols only if the library is
# already present in the process (i.e. WPEFramework, which links it via PluginHost).
find_library(RDK_OTLP_LIB rdk_otlp)
if(RDK_OTLP_LIB)
    target_include_directories(${TARGET} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_SYSROOT}/usr/include>
    )
    message(STATUS "WPEFrameworkCOM: rdk_otlp header found — COM-RPC OTEL tracing compiled in (runtime-optional via dlopen)")
else()
    message(STATUS "WPEFrameworkCOM: rdk_otlp not found — COM-RPC OTEL tracing disabled")
endif()

if (WARNING_REPORTING)
target_compile_definitions(${TARGET}
PUBLIC
        WARNING_REPORTING_ENABLED
)

target_link_libraries(${TARGET}
PUBLIC
        ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
)
endif()

target_include_directories(${TARGET}
        PRIVATE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

target_compile_definitions(${TARGET} PRIVATE COM_EXPORTS)

if(PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif(PROCESSCONTAINERS)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/com COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

InstallCMakeConfig(TARGETS ${TARGET})
