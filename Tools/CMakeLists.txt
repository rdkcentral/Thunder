# If not stated otherwise in this file or this component's LICENSE file the
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

cmake_minimum_required(VERSION 3.3)

project(Generators)

set(GENERATOR_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/sbin")

if(NOT DEFINED GENERIC_CMAKE_MODULE_PATH)
    find_path(GENERIC_CMAKE_MODULE_PATH CMake.cmake
    PATHS
        "/usr/share/cmake/Modules/"
        "/usr/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules/"
    )
endif()

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindProxyStubGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindProxyStubGenerator.cmake"
                @ONLY
)

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindJsonGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindJsonGenerator.cmake"
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

install(DIRECTORY 
        "${CMAKE_SOURCE_DIR}/ProxyStubGenerator"
        "${CMAKE_SOURCE_DIR}/JsonGenerator"
    DESTINATION ${GENERATOR_INSTALL_PATH}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
