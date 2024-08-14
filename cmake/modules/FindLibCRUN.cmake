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

cmake_minimum_required(VERSION 3.15)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

if(LibCRUN_FIND_QUIETLY)
    set(_LIBCRUN_MODE QUIET)
elseif(LibCRUN_FIND_REQUIRED)
    set(_LIBCRUN_MODE REQUIRED)
endif()

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})

    # Just check if the libcrun.pc exist, and create the PkgConfig::libcrun target
    # No version requirement (yet)
    pkg_check_modules(LIBCRUN ${_LIBCRUN_MODE} IMPORTED_TARGET libcrun)

    find_library(LIBCRUN_ACTUAL_LIBRARY NAMES crun
        HINTS ${LIBCRUN_LIBDIR})

    include(FindPackageHandleStandardArgs)
    # Sets the FOUND variable to TRUE if all required variables are present and set
    find_package_handle_standard_args(
        LibCRUN
        REQUIRED_VARS
            LIBCRUN_INCLUDE_DIRS
            LIBCRUN_CFLAGS
            LIBCRUN_LDFLAGS
            LIBCRUN_LIBRARIES
            LIBCRUN_LIBDIR
	    LIBCRUN_ACTUAL_LIBRARY
        VERSION_VAR
            LIBCRUN_VERSION
    )
    mark_as_advanced(LIBCRUN_INCLUDE_DIRS LIBCRUN_LIBRARIES LIBCRUN_ACTUAL_LIBRARY)

    if(LibCRUN_FOUND AND NOT TARGET LibCRUN::LibCRUN)
        add_library(LibCRUN::LibCRUN UNKNOWN IMPORTED)

        set_target_properties(LibCRUN::LibCRUN PROPERTIES
            IMPORTED_LOCATION "${LIBCRUN_ACTUAL_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${LIBCRUN_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${LIBCRUN_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBCRUN_INCLUDE_DIRS}"
            )
    else()
        message(FATAL_ERROR "Some required variable(s) is (are) not found / set! Does libcrun.pc exist?")
    endif()
else()

    message(FATAL_ERROR "Unable to locate PkgConfig")

endif()
