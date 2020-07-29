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

set(TARGET implementation)

find_package(NEXUS REQUIRED)
find_package(NXCLIENT REQUIRED)

add_library(${TARGET} STATIC
    Nexus.cpp
    TEECommand.cpp
    Implementation.cpp
    NexusSecurity/SHA.cpp
    SageNetflix/TEE.cpp
    SageNetflix/Vault.cpp
    SageNetflix/HMAC.cpp
    SageNetflix/AES.cpp
    SageNetflix/DiffieHellman.cpp
    SageNetflix/NetflixSecurity.cpp
)

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
    PRIVATE
        NXCLIENT::NXCLIENT
        -lsrai
        -lcmndrm
        -lcmndrm_tl
        -ldrmrootfs
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>)
