# If not stated otherwise in this file or this component's LICENSE file the
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

cmake_minimum_required(VERSION 3.3)

project(Generators)

set(GENERATOR_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/sbin")

if(NOT DEFINED GENERIC_CMAKE_MODULE_PATH)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../src/bridge")
        set(NAMESPACE bridge)
    else()
        set(NAMESPACE WPEFramework)
    endif()
    if(DEFINED ENV{STAGING_DIR})
        set(GENERIC_CMAKE_MODULE_PATH "$ENV{STAGING_DIR}/usr/include/${NAMESPACE}/Modules")
    else()
        set(GENERIC_CMAKE_MODULE_PATH "${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/Modules")
    endif()
endif()

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindProxyStubGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindProxyStubGenerator.cmake"
                @ONLY
)

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindJsonGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindJsonGenerator.cmake"
                @ONLY
)

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindConfigGenerator.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/FindConfigGenerator.cmake"
                @ONLY
)



install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/FindProxyStubGenerator.cmake"
    DESTINATION ${GENERIC_CMAKE_MODULE_PATH}
)

install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/FindJsonGenerator.cmake"
    DESTINATION ${GENERIC_CMAKE_MODULE_PATH}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/FindConfigGenerator.cmake"
    DESTINATION ${GENERIC_CMAKE_MODULE_PATH}
)


install(DIRECTORY 
        "${CMAKE_SOURCE_DIR}/ProxyStubGenerator"
        "${CMAKE_SOURCE_DIR}/JsonGenerator"
        "${CMAKE_SOURCE_DIR}/ConfigGenerator"
    DESTINATION ${GENERATOR_INSTALL_PATH}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
