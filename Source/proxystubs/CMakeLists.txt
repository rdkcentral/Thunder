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

set(TARGET ${NAMESPACE}ProxyStubs)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

set(PLUGIN_INCLUDES
        plugins/IPlugin.h
        plugins/IShell.h
        plugins/IStateControl.h
        plugins/ISubSystem.h
        )

foreach(file IN LISTS PLUGIN_INCLUDES)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../${file}" ${CMAKE_CURRENT_SOURCE_DIR} COPYONLY)
endforeach()

ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT ${CMAKE_CURRENT_SOURCE_DIR}/IPlugin.h)
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT ${CMAKE_CURRENT_SOURCE_DIR}/IShell.h)
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT ${CMAKE_CURRENT_SOURCE_DIR}/IStateControl.h)
ProxyStubGenerator(NAMESPACE "WPEFramework::PluginHost" INPUT ${CMAKE_CURRENT_SOURCE_DIR}/ISubSystem.h)

add_library(${TARGET} SHARED
        ProxyStubs_Shell.cpp
        ProxyStubs_Plugin.cpp
        ProxyStubs_StateControl.cpp
        ProxyStubs_SubSystem.cpp
        Module.cpp
        )

if (PROTOCOLS)
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/${NAMESPACE_LIB}/proxystubs COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/proxystubs COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/proxystubs      # headers
)

InstallCMakeConfig(TARGETS ${TARGET})
