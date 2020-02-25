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

if(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()
if(WIN32 AND MSVC)
    set(WIN_MSVC TRUE)
    message(STATUS "Building on Windows with MSVC")
elseif(MINGW)
    message(STATUS "Building on Windows with MinGW")
elseif(LINUX)
    message(STATUS "Building on Linux")
elseif(APPLE)
    message(STATUS "Building on OS X")
else()
    message(STATUS "Unsupported platform " ${CMAKE_HOST_SYSTEM} "?")
endif()
