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

set(TARGET ${NAMESPACE}WebSocket)

add_library(${TARGET}
        Module.cpp
        URL.cpp
        JSONWebToken.cpp
        WebSerializer.cpp
        WebSocketLink.cpp
        JSONRPCLink.cpp
        )

find_package(ZLIB REQUIRED)

set(PUBLIC_HEADERS
        URL.h
        JSONWebToken.h
        JSONRPCLink.h
        WebLink.h
        WebRequest.h
        WebResponse.h
        WebSerializer.h
        websocket.h
        WebSocketLink.h
        WebTransfer.h
        WebTransform.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ZLIB::ZLIB
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        )

target_compile_definitions(${TARGET} PRIVATE WEBSOCKET_EXPORTS)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/websocket COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink websocket ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/jsonrpc)")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink websocket.h ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/websocket/jsonrpc.h)")


InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "library containing HTTP protocol (RFC2616), WebSocket (RFC6455) and JSONRPC functionality.")


InstallCMakeConfig(TARGETS ${TARGET})
