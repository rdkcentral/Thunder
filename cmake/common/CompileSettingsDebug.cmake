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

#[[    CompileSettingsDebug
This is a non-buildable target used to set the optional settings for the
compiler for all sources of the Framework and its Plugins based on the build type
#]]

add_library(CompileSettingsDebug INTERFACE)
add_library(CompileSettingsDebug::CompileSettingsDebug ALIAS CompileSettingsDebug)

include(CMakePackageConfigHelpers)

#
# Build type specific options
#
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettingsDebug INTERFACE -O0 -g)
    endif()

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "DebugOptimized")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettingsDebug INTERFACE -O2 -g)
    endif()

elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    target_compile_options(CompileSettingsDebug INTERFACE "${CMAKE_C_FLAGS_DEBUG}")
endif()

install(TARGETS CompileSettingsDebug EXPORT CompileSettingsDebugTargets)
