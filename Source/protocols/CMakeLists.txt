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

set(TARGET ${NAMESPACE}Protocols)

find_package(ZLIB REQUIRED)

ProxyStubGenerator(NAMESPACE "WPEFramework::RPC" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/../com/ICOM.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs")

# Take care the sources are listed !!! TWICE !!! in this setup. raised a JIRA ticket to solve this for now, also scroll down and list the 
# new source file again. This is in case CMAKE forgets :-)

set(SOURCES_COM
        com/Administrator.h
        com/com.h
        com/Communicator.h
        com/Ids.h
        com/ICOM.h
        com/IUnknown.h
        com/Messages.h
        com/Administrator.cpp
        com/Communicator.cpp
        com/ITracing.cpp
        com/ProxyStubs.cpp.in
        com/IValueIterator.h
        com/IUnknown.cpp
        com/IStringIterator.h
        com/IRPCIterator.h
        )

set(SOURCES_WEBSOCKETS
        websocket/URL.cpp
        websocket/JSONWebToken.cpp
        websocket/WebSerializer.cpp
        websocket/WebSocketLink.cpp
        websocket/JSONRPCLink.cpp
        websocket/URL.h
        websocket/JSONWebToken.h
        websocket/JSONRPCLink.h
        websocket/WebLink.h
        websocket/WebRequest.h
        websocket/WebResponse.h
        websocket/WebSerializer.h
        websocket/websocket.h
        websocket/WebSocketLink.h
        websocket/WebTransfer.h
        websocket/WebTransform.h
        )

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/com")
foreach(file IN LISTS SOURCES_COM)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../${file}" ${CMAKE_CURRENT_BINARY_DIR}/com COPYONLY)
endforeach()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/websocket")
foreach(file IN LISTS SOURCES_WEBSOCKETS)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../${file}" ${CMAKE_CURRENT_BINARY_DIR}/websocket COPYONLY)
endforeach()

set(COM_INCLUDES
        "${CMAKE_CURRENT_BINARY_DIR}/com/Administrator.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/com.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/Communicator.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/Ids.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/ICOM.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/IUnknown.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/Messages.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/IRPCIterator.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/IStringIterator.h"
        "${CMAKE_CURRENT_BINARY_DIR}/com/IValueIterator.h"
        )

set(PROTOCOLS_INCLUDES
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/URL.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/JSONWebToken.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/JSONRPCLink.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebLink.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebRequest.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebResponse.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebSerializer.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/websocket.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebSocketLink.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebTransfer.h"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebTransform.h"
        )

set(PUBLIC_HEADERS
        ${COM_INCLUDES}
        ${PROTOCOLS_INCLUDES}
        Module.h
        )

set(PROXYSTUB_COM_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs/ProxyStubs_COM.cpp")

configure_file( "${CMAKE_CURRENT_BINARY_DIR}/com/ProxyStubs.cpp.in"
                "${CMAKE_CURRENT_BINARY_DIR}/com/ProxyStubs.cpp"
                @ONLY)

# Construct a library object
add_library(${TARGET} SHARED
        Module.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/com/Administrator.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/com/Communicator.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/com/ITracing.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/com/ProxyStubs.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/com/IUnknown.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/URL.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/JSONWebToken.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebSerializer.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/WebSocketLink.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/websocket/JSONRPCLink.cpp"
        )

target_include_directories(${TARGET} 
        PRIVATE 
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/com>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/websocket>
)

target_compile_definitions(${TARGET} PRIVATE COM_EXPORTS)
target_compile_definitions(${TARGET} PRIVATE WEBSOCKET_EXPORTS)

target_link_libraries(${TARGET}
        PRIVATE
          ZLIB::ZLIB
          CompileSettingsDebug::CompileSettingsDebug
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

if(PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif(PROCESSCONTAINERS)

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/protocols COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

InstallCMakeConfig(TARGETS ${TARGET})

#FIXME: This is messy, but it's the consequence of combining more independent libraries in one.
install(
        FILES ${COM_INCLUDES} ../com/Module.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/com
)

install(
        FILES ${PROTOCOLS_INCLUDES} ../websocket/Module.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/websocket
)

#FIXME: If every one moved to the JSONRPCLink in websocket, the mimic of the jsonrpc lib can be removed !!!
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink websocket ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/jsonrpc)")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink websocket.h ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/websocket/jsonrpc.h)")

