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

ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IPlugin.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IShell.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::Exchange" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IController.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated" INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IStateControl.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/ISubSystem.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/IDispatcher.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

JsonGenerator(CODE NAMESPACE WPEFramework::Exchange::IController INPUT ${CMAKE_CURRENT_SOURCE_DIR}/IController.h OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/jsonrpc" INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." NO_INCLUDES)

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
        )

add_library(${TARGET_PROXYSTUBS} SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Plugin.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Shell.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Controller.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_StateControl.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_SubSystem.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_Dispatcher.cpp
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

target_link_libraries(${TARGET_PROXYSTUBS}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}COM::${NAMESPACE}COM
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
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
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

set_target_properties(${TARGET_PROXYSTUBS} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
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

install(
        TARGETS ${TARGET_PROXYSTUBS}  EXPORT ${TARGET_PROXYSTUBS}Targets        # for downstream dependencies
        ARCHIVE DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs                                  # binaries
        FRAMEWORK DESTINATION bin/${NAMESPACE_LIB}/proxystubs COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/proxystubs COMPONENT devel # for mac
        INCLUDES DESTINATION include/${NAMESPACE}/proxystubs                    # headers
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
