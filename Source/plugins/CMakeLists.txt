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

set(TARGET ${NAMESPACE}Plugins)
set(TARGET_PROXYSTUBS ${NAMESPACE}ProxyStubs)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

ProxyStubGenerator(NAMESPACE "Thunder::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IPlugin.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "Thunder::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IShell.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "Thunder::Exchange" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IController.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated" INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
ProxyStubGenerator(NAMESPACE "Thunder::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IStateControl.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "Thunder::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/ISubSystem.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "Thunder::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IDispatcher.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated" INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")

JsonGenerator(CODE NAMESPACE Thunder::Exchange::Controller INPUT ${CMAKE_CURRENT_SOURCE_DIR}/IController.h OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/json" INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." NO_INCLUDES)

file(GLOB JSON_ENUM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/json/JsonEnum_*.cpp")
file(GLOB JSON_DATA_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/json/JsonData_*.h" "${CMAKE_CURRENT_BINARY_DIR}/json/json_*.h")

add_library(${TARGET} SHARED
        Channel.cpp
        JSONRPC.cpp
        Metadata.cpp
        Module.cpp
        Service.cpp
        Shell.cpp
        StateControl.cpp
        SubSystem.cpp
        VirtualInput.cpp
        System.cpp
        ${JSON_ENUM_SOURCES}
        )

file(GLOB PROXY_STUB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs*.cpp")
add_library(${TARGET_PROXYSTUBS} SHARED
        ${PROXY_STUB_SOURCES}
        Module.cpp
        )

set(PUBLIC_HEADERS
        Channel.h
        Config.h
        Configuration.h
        IPlugin.h
        IShell.h
        IController.h
        Shell.h
        IStateControl.h
        StateControl.h
        ISubSystem.h
        IDispatcher.h
        SubSystem.h
        JSONRPC.h
        Metadata.h
        plugins.h
        Request.h
        Service.h
        System.h
        Types.h
        VirtualInput.h
        IVirtualInput.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}Messaging::${NAMESPACE}Messaging
          ${NAMESPACE}COM::${NAMESPACE}COM
          ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

if (WARNING_REPORTING)
    target_link_libraries(${TARGET}
           PUBLIC 
              ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
            )
endif()

# when compiling proxy/stubs we should ignore deprecated warnings (which are treated as erros nowadays) 
# as usage of those deprecated methods is valid here
target_compile_options(${TARGET_PROXYSTUBS} PRIVATE -Wno-deprecated-declarations)

target_link_libraries(${TARGET_PROXYSTUBS}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}COM::${NAMESPACE}COM
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

target_include_directories( ${TARGET_PROXYSTUBS}
        PRIVATE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

set_target_properties(${TARGET_PROXYSTUBS} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/plugins COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

install(
        FILES ${JSON_DATA_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/plugins/json COMPONENT ${NAMESPACE}_Development
)

install(
        TARGETS ${TARGET_PROXYSTUBS}  EXPORT ${TARGET_PROXYSTUBS}Targets      
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR}/${NAMESPACE_LIB}/proxystubs COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/proxystubs COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/proxystubs
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "Basic library to realize the glue between a proprietary plugin and the framework host.")

InstallCMakeConfig(
        EXTRA_DEPENDENCIES "ConfigGenerator"
        TARGETS ${TARGET})

InstallCMakeConfig(TARGETS ${TARGET_PROXYSTUBS})
