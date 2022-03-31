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

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IPlugin.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IShell.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IController.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IStateControl.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/ISubSystem.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

add_library(${TARGET} SHARED
        Channel.cpp
        JSONRPC.cpp
        MetaData.cpp
        Module.cpp
        Service.cpp
        Shell.cpp
        StateControl.cpp
        SubSystem.cpp
        VirtualInput.cpp
        System.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Plugin.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Shell.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Controller.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_StateControl.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_SubSystem.cpp
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
        SubSystem.h
        JSONRPC.h
        MetaData.h
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
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}COM::${NAMESPACE}COM
        )

if (WARNING_REPORTING)
    target_link_libraries(${TARGET}
           PUBLIC 
              ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
            )
endif()

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/plugins COMPONENT devel # for mac
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "Basic library to realize the glue between a proprietary plugin and the framework host.")

InstallCMakeConfig(
        TARGETS ${TARGET})
