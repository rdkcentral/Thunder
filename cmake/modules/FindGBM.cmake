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

cmake_minimum_required(VERSION 3.15)

# Be compatible even if a newer CMake version is available
cmake_policy(VERSION 3.7...3.12)

if(GBM_FIND_QUIETLY)
    set(_GBM_MODE QUIET)
elseif(GBM_FIND_REQUIRED)
    set(_GBM_MODE REQUIRED)
endif()

find_package(PkgConfig)
if(${PKG_CONFIG_FOUND})

    # Just check if the gbm.pc exist, and create the PkgConfig::gbm target
    # No version requirement (yet)
    pkg_check_modules(PC_GBM ${_GBM_MODE} IMPORTED_TARGET gbm)
    find_library(GBM_ACTUAL_LIBRARY NAMES gbm
        HINTS ${PC_GBM_LIBRARY_DIRS})

    find_path(GBM_INCLUDE_DIR NAMES gbm.h
        HINTS ${PC_GBM_INCLUDEDIR} ${PC_GBM_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Unable to locate PkgConfig")
endif()

include(FindPackageHandleStandardArgs)
# Sets the FOUND variable to TRUE if all required variables are present and set
find_package_handle_standard_args(
    GBM
    REQUIRED_VARS
        GBM_ACTUAL_LIBRARY
        PC_GBM_LIBRARIES
        GBM_INCLUDE_DIR
    VERSION_VAR
        GBM_VERSION
)
mark_as_advanced(GBM_INCLUDE_DIR PC_GBM_LIBRARIES GBM_ACTUAL_LIBRARY)

if(GBM_FOUND AND NOT TARGET GBM::GBM)
    add_library(GBM::GBM UNKNOWN IMPORTED)
    set_target_properties(GBM::GBM PROPERTIES
        IMPORTED_LOCATION "${GBM_ACTUAL_LIBRARY}"
        INTERFACE_LINK_LIBRARIES "${PC_GBM_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${PC_GBM_CFLAGS}"
        INTERFACE_INCLUDE_DIRECTORIES "${GBM_INCLUDE_DIR}"
    )
endif()
