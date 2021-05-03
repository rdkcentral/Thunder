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

find_package(PkgConfig)
pkg_check_modules(PC_AWC libslauncher_client)

if(${PC_AWC_FOUND})
    find_library(AWC_LIBRARY slauncher_client
        HINTS ${PC_AWC_LIBDIR} ${PC_AWC_LIBRARY_DIRS} REQUIRED
        )
    find_path(AWC_INCLUDE AWCClient.h
        PATH_SUFFIXES awc
        )

    if(NOT TARGET Slauncher::Slauncher)
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
