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

if(Slauncher_FIND_QUIETLY)
    set(_SLAUNCHER_MODE QUIET)
elseif(Slauncher_FIND_REQUIRED)
    set(_SLAUNCHER_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_AWC ${_SLAUNCHER_MODE} libslauncher_client)

if(${PC_AWC_FOUND})
    find_library(AWC_LIBRARY slauncher_client
        HINTS ${PC_AWC_LIBDIR} ${PC_AWC_LIBRARY_DIRS} REQUIRED
        )
    find_path(AWC_INCLUDE AWCClient.h
        PATH_SUFFIXES awc
        )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Slauncher DEFAULT_MSG AWC_LIBRARY AWC_INCLUDE)

    mark_as_advanced(AWC_LIBRARY AWC_INCLUDE)

    if(Slauncher_FOUND AND NOT TARGET Slauncher::Slauncher)
        add_library(Slauncher::Slauncher UNKNOWN IMPORTED)

        set_target_properties(Slauncher::Slauncher
            PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${AWC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${AWC_INCLUDE}"
            INTERFACE_LINK_LIBRARIES "${PC_AWC_LIBRARIES}"
        )
    endif()
endif()
