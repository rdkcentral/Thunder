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

cmake_minimum_required(VERSION 3.15)

project(Thunder
        VERSION 5.0.0
        DESCRIPTION "Thunder framework"
        HOMEPAGE_URL "https://rdkcentral.github.io/Thunder/")

set(VERSION       ${${PROJECT_NAME}_VERSION})
set(VERSION_MAJOR ${${PROJECT_NAME}_VERSION_MAJOR})
set(VERSION_MINOR ${${PROJECT_NAME}_VERSION_MINOR})
set(VERSION_PATCH ${${PROJECT_NAME}_VERSION_PATCH})

string(TIMESTAMP BUILD_TIMESTAMP UTC)

message(STATUS "Setting up ${PROJECT_NAME} v${Thunder_VERSION}")

option(HIDE_NON_EXTERNAL_SYMBOLS
    "Hide all non EXTERNAL tagged symbols" ON)
option(ENABLE_STRICT_COMPILER_SETTINGS
    "Enable compiler flags to get the warnings/errors due to improper condition in the code" OFF) 
option(LEGACY_CONFIG_GENERATOR
    "Use the legacy config generator and all its needed CMake sub-components. 
     If switched OFF, plugins need to use the new python-based templates. (*.conf.in)" OFF)
option(HUMAN_VERSIONED_BINARIES
    "Create binaries with a human readeable version suffix" ON)
option(VERSIONED_LIBRARY_LOADING
    "Uses the major version (*.so.<major>) to load libraries rather then *.so" OFF)
    

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

if(VERSIONED_LIBRARY_LOADING)
    add_definitions(-DVERSIONED_LIBRARY_LOADING)
endif()

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
    "${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/Modules")

if (TOOLS_SYSROOT)
    list(APPEND CMAKE_MODULE_PATH
        "${TOOLS_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}/Modules")
endif()

include(platform)
include(CmakeHelperFunctions)
include(GNUInstallDirs)

set(PLATFORM "PC_UNIX" CACHE STRING
        "Defines on what platform the application will run")

set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

find_package(ProxyStubGenerator REQUIRED)
find_package(JsonGenerator REQUIRED)
find_package(ConfigGenerator REQUIRED)

if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Enable shared objects by default") 
    message(STATUS "BUILD_SHARED_LIBS was not set, assuming ${BUILD_SHARED_LIBS}")
endif()

#
# Build type specific options
#
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
    message(AUTHOR_WARNING "CMAKE_BUILD_TYPE not set, assuming '${CMAKE_BUILD_TYPE}'")
endif()

message(STATUS "CMake build type: '${CMAKE_BUILD_TYPE}'")

# Remove optimization flags added by the build system
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
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/project.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
                @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
        RENAME "${NAMESPACE}Config.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${NAMESPACE})

if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${NAMESPACE})
endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)

install(DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${NAMESPACE})

if(APPLE)
    # Mac needed variables
    # http://www.cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}")
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
