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

cmake_minimum_required(VERSION 3.3)

project(WPEFramework)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

message(STATUS "Setting up ${PROJECT_NAME} v${VERSION}")

option(HIDE_NON_EXTERNAL_SYMBOLS
    "Hide all non EXTERNAL tagged symbols" ON)
option(ENABLE_STRICT_COMPILER_SETTINGS
    "Enable compiler flags to get the warnings/errors due to improper condition in the code" OFF) 
option(LEGACY_CONFIG_GENERATOR
    "Use the legacy config generator and all its needed CMake sub-components. 
     If switched OFF, plugins need to use the new python-based templates. (*.conf.in)" ON) 
option(BUILD_PLUGIN_ACTIVATOR
     "Build the standalone plugin activator utility to activate plugins using systemd" OFF) 

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    "${CMAKE_SOURCE_DIR}/cmake/common"
    "${CMAKE_SOURCE_DIR}/cmake/modules"
    "${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/Modules")

if (TOOLS_SYSROOT)
    list(APPEND CMAKE_MODULE_PATH
        "${TOOLS_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/Modules")
endif()

include(platform)
include(CmakeHelperFunctions)

set(PLATFORM "PC_UNIX" CACHE STRING
        "Defines on what platform the application will run")

set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

find_package(ProxyStubGenerator REQUIRED)
find_package(JsonGenerator REQUIRED)
find_package(ConfigGenerator REQUIRED)

#
# Build type specific options
#
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
    message(AUTHOR_WARNING "CMAKE_BUILD_TYPE not set, assuming '${CMAKE_BUILD_TYPE}'")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
   "${CMAKE_BUILD_TYPE}" STREQUAL "DebugOptimized" OR
   "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo" OR
   "${CMAKE_BUILD_TYPE}" STREQUAL "Release" OR
   "${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")

    message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
else()
    message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
endif()

# Remove optimization flags added by the build system
string(REGEX REPLACE "(-O([0123gs]|fast))" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "(-O([0123gs]|fast))" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "(-g[0123])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "(-g[0123])" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "\\-\\g$" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "\\-\\g$" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
# FIX_ME: Disable fortify source.
#         https://jira.rdkcentral.com/jira/browse/METROL-483
#         Enabled in upstream buildroot, generates a lot of warnings, caused by missing optimalisation flags.
#         https://www.redhat.com/en/blog/enhance-application-security-fortifysource
string(REGEX REPLACE "-D_FORTIFY_SOURCE=[0-3]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "-D_FORTIFY_SOURCE=[0-3]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# make sure others can make use of the JSON creation tools as well!!!
configure_file( "${CMAKE_SOURCE_DIR}/cmake/project.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
                @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
        RENAME "${NAMESPACE}Config.cmake"
        DESTINATION lib/cmake/${NAMESPACE})

if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
install(DIRECTORY
        "${CMAKE_SOURCE_DIR}/cmake/config"
    DESTINATION lib/cmake/${NAMESPACE})
endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)

install(DIRECTORY
            "${CMAKE_SOURCE_DIR}/cmake/common"
            "${CMAKE_SOURCE_DIR}/cmake/templates"
        DESTINATION lib/cmake/${NAMESPACE})

if(APPLE)
    # Mac needed variables
    # http://www.cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if (ENABLE_CODE_COVERAGE)
  include (CodeCoverage)
  append_coverage_compiler_flags()
  set(COVERAGE_EXCLUDES
    "${CMAKE_CURRENT_BINARY_DIR}/Tests/unit*"
    "${CMAKE_CURRENT_BINARY_DIR}/Tests/unit/core/*")
  setup_target_for_coverage_gcovr_html(NAME "coverage")
  setup_target_for_coverage_gcovr_xml(NAME "coverage-report")
  add_definitions(-DWITH_CODE_COVERAGE=1)
endif()

if(BUILD_TESTS)
  enable_testing()
endif()

add_subdirectory(Source)

add_subdirectory(Tests)

if (BUILD_PLUGIN_ACTIVATOR)
  add_subdirectory(Utils/PluginActivator)
endif()
