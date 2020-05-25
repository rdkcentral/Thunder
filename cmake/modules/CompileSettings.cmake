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

#[[    CompileSettings 
This is a non-buildable target used to set the overall mendatory settings for the 
compiler for all sources of the Framework and its Plugins
#]]
option(POSITION_INDEPENDENT_CODE "Create position independent code on all targets including static libs" ON)

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

if(NOT BUILD_TYPE)
    set(BUILD_TYPE Production)
    message(AUTHOR_WARNING "BUILD_TYPE not set, assuming '${BUILD_TYPE}'")
endif()

target_compile_definitions(CompileSettings INTERFACE PLATFORM_${PLATFORM}=1)
message(STATUS "Selected platform ${PLATFORM}")

target_compile_options(CompileSettings INTERFACE -std=c++11 -Wno-psabi)

#
# Build type specific options
#
if("${BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_DEBUG)
    set(CONFIG_DIR "Debug" CACHE STRING "Build config directory" FORCE)

elseif("${BUILD_TYPE}" STREQUAL "DebugOptimized")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_DEBUG)
    set(CONFIG_DIR "DebugOptimized" CACHE STRING "Build config directory" FORCE)

elseif("${BUILD_TYPE}" STREQUAL "ReleaseSymbols")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG)
    set(CONFIG_DIR "ReleaseSymbols" CACHE STRING "Build config directory" FORCE)

elseif("${BUILD_TYPE}" STREQUAL "Release")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG)
    set(CONFIG_DIR "Release" CACHE STRING "Build config directory" FORCE)

elseif("${BUILD_TYPE}" STREQUAL "Production")
    target_compile_definitions(CompileSettings INTERFACE _THUNDER_NDEBUG _THUNDER_PRODUCTION)
    set(CONFIG_DIR "Production" CACHE STRING "Build config directory" FORCE)

else()
    message(FATAL_ERROR "Invalid BUILD_TYPE: '${BUILD_TYPE}'")
endif()


#
# Compiler specific options
#
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Compiling with Clang")
    target_compile_options(CompileSettings INTERFACE -Weverything)
elseif(${CMAKE_COMPILER_IS_GNUCXX})
    message(STATUS "Compiling with GCC")
    target_compile_options(CompileSettings INTERFACE -Wall)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiling with MS Visual Studio")
    target_compile_options(CompileSettings INTERFACE /W4)
else()
    message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# END CompileSettings
