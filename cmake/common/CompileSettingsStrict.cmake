# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2026 Metrological
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

#[[
    CompileSettingsStrict.cmake

    Configure global compiler warning and error flags used by the Framework and
    its plugins. This is not a buildable target — it is a small configuration
    module intended to be included from the top-level CMakeLists and to set
    project-wide compile options.

    Usage:
        include(common/CompileSettingsStrict.cmake)

    Behavior:
        - Controlled by the ENABLE_STRICT_COMPILER_SETTINGS option (defined below).
            ON  -> enable stricter, per-compiler warning+error flags.
            OFF -> apply minimal compatibility tweaks (preserve previous behaviour).
        - Applies appropriate flags for Clang, GCC and MSVC.

    Guidelines:
        - Keep only compiler-wide, project-relevant flags here. Avoid target-specific
            or platform-specific logic that belongs in the corresponding target or
            platform layer.
]]

option(ENABLE_STRICT_COMPILER_SETTINGS "Enable strict compiler warnings and treat some warnings as errors" OFF)

if(ENABLE_STRICT_COMPILER_SETTINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Enabling strict compiler settings for Clang")
        set(_STRICT_OPTS -Wall -Wextra -Werror=return-type -Werror=array-bounds -Wnon-virtual-dtor -Wmisleading-indentation)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 12)
            list(APPEND _STRICT_OPTS -Wrange-loop-construct)
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        message(STATUS "Enabling strict compiler settings for GCC")
        set(_STRICT_OPTS -Wall -Wextra -Wpedantic -Werror -Wnon-virtual-dtor -Wmisleading-indentation)
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 12)
            list(APPEND _STRICT_OPTS -Wrange-loop-construct)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        message(STATUS "Enabling strict compiler settings for MSVC")
        set(_STRICT_OPTS /W4)
    else()
        message(STATUS "ENABLE_STRICT_COMPILER_SETTINGS enabled, but compiler ${CMAKE_CXX_COMPILER_ID} is unrecognised")
    endif()

    if(_STRICT_OPTS)
        message(VERBOSE "Adding strict compiler options: ${_STRICT_OPTS}")
        add_compile_options(${_STRICT_OPTS})
    endif()
else()
    # Default: for Clang disable a noisy deprecated-declarations warning, keep behaviour
    # compatible with the previous location of this code.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-deprecated-declarations)
    endif()
endif()
