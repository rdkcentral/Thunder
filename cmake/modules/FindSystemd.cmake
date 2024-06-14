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

# - Try to find Systemd
# Once done this will define

#  SYSTEMD_FOUND - System has libsystemd
#  SYSTEMD_INCLUDE_DIRS - The libsystemd include directories
#  SYSTEMD_LIBRARIES - The libraries needed to use libsystemd
#  SYSTEMD_DEFINITIONS - The libraries needed to use libsystemd

if(Systemd_FIND_QUIETLY)
    set(_SYSTEMD_MODE QUIET)
    message("Systemd_FIND_QUIETLY = " ${Systemd_FIND_QUIETLY})
elseif(Systemd_FIND_REQUIRED)
    set(_SYSTEMD_MODE REQUIRED)
    message("Systemd_FIND_REQUIRED = " ${Systemd_FIND_REQUIRED})
endif()

find_package(PkgConfig)
pkg_check_modules(PC_SYSTEMD ${_SYSTEMD_MODE} libsystemd)

if(${PC_SYSTEMD_FOUND})
    find_library(SYSTEMD_LIBRARY systemd
        HINTS ${PC_SYSTEMD_LIBDIR} ${PC_SYSTEMD_LIBRARY_DIRS} REQUIRED
        )
    find_path(SYSTEMD_INCLUDE systemd/sd-daemon.h
        PATHS ${PC_SYSTEMD_INCLUDE_DIRS}
        )

    set(SYSTEMD_DEFINITIONS ${PC_SYSTEMD_CFLAGS_OTHER})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Systemd DEFAULT_MSG SYSTEMD_INCLUDE SYSTEMD_LIBRARY)
    mark_as_advanced(Systemd_FOUND SYSTEMD_INCLUDE SYSTEMD_LIBRARY SYSTEMD_DEFINITIONS)

    set(SYSTEMD_FOUND ${Systemd_FOUND})
    if(Systemd_FOUND AND NOT TARGET Systemd::Systemd)
        add_library(Systemd::Systemd UNKNOWN IMPORTED)

        set_target_properties(Systemd::Systemd
            PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${SYSTEMD_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMD_INCLUDE}"
            INTERFACE_LINK_LIBRARIES "${PC_SYSTEMD_LIBRARIES}"
        )
    endif()
endif()
