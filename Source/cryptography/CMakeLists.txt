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

set(TARGET ${NAMESPACE}Cryptography)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# remove the -as-needed flag because it can lead to losing dependencies when we're mixing static and shared libs 
string(REPLACE "-Wl,--as-needed" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed")

option(INCLUDE_SOFTWARE_CRYPTOGRAPHY_LIBRARY "Include explicitly a software based cryptography library" OFF)

add_subdirectory(implementation)

add_library(${TARGET} SHARED
    Module.cpp
    Cryptography.cpp
    NetflixSecurity.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Tracing::${NAMESPACE}Tracing
    PRIVATE
        -Wl,--whole-archive implementation -Wl,--no-whole-archive
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/implementation>
        $<INSTALL_INTERFACE:include/${NAMESPACE}/cryptography>
)

set(PUBLIC_HEADERS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ICryptography.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/INetflixSecurity.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cryptography.h>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/implementation/cryptography_vault_ids.h>
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    LINK_WHAT_YOU_USE TRUE
    VERSION ${VERSION}
    SOVERSION ${VERSION_MAJOR}
)

install(
    TARGETS ${TARGET}  EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION lib COMPONENT libs
    LIBRARY DESTINATION lib COMPONENT libs
    FRAMEWORK DESTINATION bin COMPONENT libs
    PUBLIC_HEADER DESTINATION include/${NAMESPACE}/cryptography
    INCLUDES DESTINATION include/${NAMESPACE}/cryptography
)

InstallPackageConfig(
    TARGETS ${TARGET}
    DESCRIPTION "A crypto wrapper library."
)

InstallCMakeConfig(
    TARGETS ${TARGET}
)

if(INCLUDE_SOFTWARE_CRYPTOGRAPHY_LIBRARY)
    find_package(OpenSSL REQUIRED)

    add_library(${TARGET}Software SHARED
        Module.cpp
        Cryptography.cpp
        NetflixSecurity.cpp
        implementation/OpenSSL/Vault.cpp
        implementation/OpenSSL/Hash.cpp
        implementation/OpenSSL/Cipher.cpp
        implementation/OpenSSL/DiffieHellman.cpp
        implementation/OpenSSL/Derive.cpp
    )

    target_link_libraries(${TARGET}Software 
        PRIVATE
            ${NAMESPACE}Core::${NAMESPACE}Core
            ${NAMESPACE}Tracing::${NAMESPACE}Tracing
            OpenSSL::SSL
            OpenSSL::Crypto
    )

    target_include_directories(${TARGET}Software 
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/implementation>
        PUBLIC
            $<INSTALL_INTERFACE:include/${NAMESPACE}/cryptography>
    )

    set_target_properties(${TARGET}Software PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        LINK_WHAT_YOU_USE TRUE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
    )

    install(
        TARGETS ${TARGET}Software EXPORT ${TARGET}SoftwareTargets
        ARCHIVE DESTINATION lib COMPONENT libs
        LIBRARY DESTINATION lib COMPONENT libs
        FRAMEWORK DESTINATION bin COMPONENT libs
        INCLUDES DESTINATION include/${NAMESPACE}/cryptography
    )

    InstallPackageConfig(
        TARGETS ${TARGET}Software
        DESCRIPTION "A software crypto wrapper library."
    )

    InstallCMakeConfig(
        TARGETS ${TARGET}Software 
    )
endif(INCLUDE_SOFTWARE_LIB)
