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

cmake_minimum_required(VERSION 3.7)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

find_package(PkgConfig)

if(${PKG_CONFIG_FOUND})

    # Just check if the libcrun.pc exist, and create the PkgConfig::libcrun target
    # No version requirement (yet)
    pkg_check_modules(LIBCRUN REQUIRED IMPORTED_TARGET libcrun)

    include(FindPackageHandleStandardArgs)

    # Sets the FOUND variable to TRUE if all required variables are present and set
    find_package_handle_standard_args(
        libcrun
        REQUIRED_VARS
            LIBCRUN_INCLUDE_DIRS
            LIBCRUN_CFLAGS
            LIBCRUN_LDFLAGS
            LIBCRUN_LIBRARIES
            LIBCRUN_LIBRARY_DIRS
        VERSION_VAR
            LIBCRUN_VERSION
    )

    find_library(LIBCRUN_ACTUAL_LIBRARY NAMES crun
        HINTS ${LIBCRUN_LIBRARY_DIRS} )

    if(LIBCRUN_FOUND AND NOT TARGET libcrun::libcrun)
        add_library(libcrun::libcrun UNKNOWN IMPORTED)

        set_target_properties(libcrun::libcrun PROPERTIES
            IMPORTED_LOCATION "${LIBCRUN_ACTUAL_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${LIBCRUN_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${LIBCRUN_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBCRUN_INCLUDE_DIRS}"
            )
    else()
        message(FATAL_ERROR "Some required variable(s) is (are) not found / set! Does libcrun.pc exist?")
    endif()

    #mark_as_advanced(LIBCRUN_INCLUDE_DIRS LIBCRUN_LIBRARIES)
else()

    message(FATAL_ERROR "Unable to locate PkgConfig")

endif()
