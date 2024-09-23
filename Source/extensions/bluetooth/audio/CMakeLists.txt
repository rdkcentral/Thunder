# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2023 Metrological
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

cmake_minimum_required(VERSION 3.15)

project(${NAMESPACE}BluetoothAudio
        VERSION 1.0.0
        DESCRIPTION "Bluetooth audio library"
        LANGUAGES CXX)

set(TARGET ${PROJECT_NAME})
message("Setup ${TARGET} v${PROJECT_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(SBC REQUIRED)

file(GLOB CODEC_HEADERS codecs/*.h)

set(PUBLIC_HEADERS
    IAudioCodec.h
    IAudioContentProtection.h
    SDPSocket.h
    SDPProfile.h
    AVDTPSocket.h
    AVDTPProfile.h
    RTPSocket.h
    DataRecord.h
    Module.h
    bluetooth_audio.h
)

add_library(${TARGET}
    SDPSocket.cpp
    SDPProfile.cpp
    AVDTPSocket.cpp
    AVDTPProfile.cpp
    codecs/SBC.cpp
    Module.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        SBC::SBC
)

set_target_properties(${TARGET}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/drivers"
)

target_compile_options(${TARGET}
    PRIVATE
        -Wno-psabi
        -fdiagnostics-color=always
)

install(
    TARGETS ${TARGET}  EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
    FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/bluetooth/audio COMPONENT ${NAMESPACE}_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

install(
    FILES ${CODEC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/bluetooth/audio/codecs
)

InstallPackageConfig(
    TARGETS ${TARGET}
    DESCRIPTION "${PROJECT_DESCRIPTION}"
)

InstallCMakeConfig(
    TARGETS ${TARGET}
)
