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

option(BUILD_NETFLIX_VAULT_GENERATOR "Build Netflix Vault generator" OFF)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source)

set(CMAKE_CXX_STANDARD 11)

add_executable(cgimptests
        ImplementationTests.c
        Helpers.cpp
        Test.c
    )

find_package(OpenSSL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography/implementation)
include_directories($<INSTALL_INTERFACE:include/${NAMESPACE}>)

target_link_libraries(cgimptests
        PRIVATE
        ${NAMESPACE}Cryptography
        ssl
        crypto
        -Wl,--warn-unresolved-symbols
    )


add_executable(cgfacetests
        InterfaceTests.cpp
        Helpers.cpp
        Test.c
    )

find_package(OpenSSL)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography/implementation)
include_directories($<INSTALL_INTERFACE:include/${NAMESPACE}>)

set_target_properties(cgfacetests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_link_libraries(cgfacetests
        PRIVATE
        ${NAMESPACE}Cryptography
        ssl
        crypto
        -Wl,--warn-unresolved-symbols
    )

add_executable(cgnfsecuritytests
        NetflixSecurityTests.cpp
        Helpers.cpp
        Test.c
    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../Source/cryptography/implementation)
include_directories($<INSTALL_INTERFACE:include/${NAMESPACE}>)

set_target_properties(cgnfsecuritytests PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
    )

target_link_libraries(cgnfsecuritytests
        PRIVATE
        ${NAMESPACE}Cryptography
        ssl
        crypto
         -Wl,--warn-unresolved-symbols
    )

install(TARGETS cgimptests DESTINATION bin)
install(TARGETS cgfacetests DESTINATION bin)
install(TARGETS cgnfsecuritytests DESTINATION bin)

if (BUILD_NETFLIX_VAULT_GENERATOR)
   add_subdirectory(NetflixVaultGenerator)
endif (BUILD_NETFLIX_VAULT_GENERATOR)
