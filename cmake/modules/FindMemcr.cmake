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

cmake_minimum_required(VERSION 3.7)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

if(MEMCR_FIND_QUIETLY)
    set(_MEMCR_MODE QUIET)
elseif(MEMCR_FIND_REQUIRED)
    set(_MEMCR_MODE REQUIRED)
endif()

find_library( MEMCR_LIBRARIES NAMES "checkpoint" )

include(FindPackageHandleStandardArgs)
# Sets the FOUND variable to TRUE if all required variables are present and set
find_package_handle_standard_args(
    MEMCR
    REQUIRED_VARS
    MEMCR_LIBRARIES
)
mark_as_advanced(MEMCR_LIBRARIES)

if(MEMCR_FOUND AND NOT TARGET MEMCR::MEMCR)
    add_library(MEMCR::MEMCR UNKNOWN IMPORTED)
    set_target_properties(MEMCR::MEMCR PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
        IMPORTED_LOCATION "${MEMCR_LIBRARIES}"
        INTERFACE_LINK_LIBRARIES "checkpoint"
    )
else()
    message(FATAL_ERROR "Some required variable(s) is (are) not found / set! Does Memcr exist?")
endif()