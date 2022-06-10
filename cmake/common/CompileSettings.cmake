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

#[[    CompileSettings 
This is a non-buildable target used to set the overall mendatory settings for the 
compiler for all sources of the Framework and its Plugins
#]]
option(POSITION_INDEPENDENT_CODE "Create position independent code on all targets including static libs" ON)
option(EXCEPTIONS_ENABLE "Enable exception handling" OFF)
option(PERFORMANCE_MONITOR "Include Performance Monitoring" OFF)

add_library(CompileSettings INTERFACE)
add_library(CompileSettings::CompileSettings ALIAS CompileSettings)

#
# Global options
#
target_include_directories(CompileSettings INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>)

if (POSITION_INDEPENDENT_CODE)
    set_target_properties(CompileSettings PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Enabled position independent code")
endif()

if (SYSTEM_PREFIX)
    target_compile_definitions(CompileSettings INTERFACE SYSTEM_PREFIX=${SYSTEM_PREFIX})
    message(STATUS "System prefix is set to: ${SYSTEM_PREFIX}")
endif()

if(EXCEPTIONS_ENABLE)
    target_compile_options(CompileSettings INTERFACE -fexceptions)
    message(STATUS "Exception handling is enabled")
else()
    target_compile_options(CompileSettings INTERFACE -fno-exceptions)
    message(STATUS "Exception handling is disabled")
endif()

if(PERFORMANCE_MONITOR)
    target_compile_definitions(CompileSettings INTERFACE "THUNDER_PERFORMANCE=1")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
    message(AUTHOR_WARNING "CMAKE_BUILD_TYPE not set, assuming '${CMAKE_BUILD_TYPE}'")
endif()

# target_compile_definitions(CompileSettings INTERFACE "THUNDER_PLATFORM=${THUNDER_PLATFORM}")
# message(STATUS "Selected platform ${THUNDER_PLATFORM}")

target_compile_options(CompileSettings INTERFACE -std=c++11 -Wno-psabi)

#
# Build type specific options
#
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_DEBUG)
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_CALLSTACK_INFO)
    target_compile_options(CompileSettings INTERFACE -funwind-tables)
    set(CONFIG_DIR "Debug" CACHE STRING "Build config directory" FORCE)

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "DebugOptimized")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_DEBUG)
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_CALLSTACK_INFO)
    target_compile_options(CompileSettings INTERFACE -funwind-tables)
    set(CONFIG_DIR "DebugOptimized" CACHE STRING "Build config directory" FORCE)

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG)
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_CALLSTACK_INFO)
    target_compile_options(CompileSettings INTERFACE -funwind-tables)
    set(CONFIG_DIR "RelWithDebInfo" CACHE STRING "Build config directory" FORCE)

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG)
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_CALLSTACK_INFO)
    target_compile_options(CompileSettings INTERFACE -funwind-tables)
    set(CONFIG_DIR "Release" CACHE STRING "Build config directory" FORCE)

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG _THUNDER_PRODUCTION)
    set(CONFIG_DIR "MinSizeRel" CACHE STRING "Build config directory" FORCE)

else()
    message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")
endif()

# END CompileSettings
