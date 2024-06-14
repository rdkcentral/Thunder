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

include(config)
include(CompileSettingsDebug)

option(CORE
        "Include the core library." ON)
option(CRYPTALGO
        "Include the encyption algorithm library." ON)
option(WEBSOCKET
        "Include the websocket library." ON)
option(MESSAGING
        "Include the messaging library." ON)
option(COM
        "Include the COM library." ON)
option(PROCESS
        "Include the COM hosting process executable." ON)
option(PLUGINS
        "Include plugins library." ON)
option(EXECUTABLE
        "Include the hosting executable." ON)
option(STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)
option(WCHAR_SUPPORT
        "Enable support for WCHAR." OFF)
option(DISABLE_TRACING
        "Disable tracing in debug" OFF)
set(ENABLED_TRACING_LEVEL 0 CACHE STRING "If tracing is enabled, this sets its default level.")
option(EXCEPTION_CATCHING
        "Enable unhandled exception handling catching." OFF)
option(DEADLOCK_DETECTION
        "Enable deadlock detection tooling." OFF)

if(HIDE_NON_EXTERNAL_SYMBOLS)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

if(ENABLE_STRICT_COMPILER_SETTINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Compiling with Clang")
        set(CMAKE_STRICT_COMPILER_SETTINGS "-Weverything -Wextra -Wpedantic -Werror")
        set(CMAKE_STRICT_CXX_COMPILER_SETTINGS "${CMAKE_STRICT_COMPILER_SETTINGS} -Wnon-virtual-dtor")
    elseif(${CMAKE_COMPILER_IS_GNUCXX})
        message(STATUS "Compiling with GCC")
        set(CMAKE_STRICT_COMPILER_SETTINGS "-Wall -Wextra -Wpedantic -Werror")
        set(CMAKE_STRICT_CXX_COMPILER_SETTINGS "${CMAKE_STRICT_COMPILER_SETTINGS} -Wnon-virtual-dtor")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        message(STATUS "Compiling with MS Visual Studio")
        set(CMAKE_STRICT_COMPILER_SETTINGS "/W4")
    else()
        message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_STRICT_CXX_COMPILER_SETTINGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_STRICT_COMPILER_SETTINGS}")
endif()

if(CORE)
    add_subdirectory(core)
endif()

if(CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(WEBSOCKET)
    add_subdirectory(websocket)
endif()

if(MESSAGING)
    add_subdirectory(messaging)
endif()

add_subdirectory(extensions)

if(COM)
    add_subdirectory(com)
endif()

if(PLUGINS)
    add_subdirectory(plugins)
endif()

if(PROCESS)
    add_subdirectory(ThunderPlugin)
endif()

if(EXECUTABLE)
    add_subdirectory(Thunder)
endif()
