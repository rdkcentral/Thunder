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

    # Just check if the gbm.pc exist, and create the PkgConfig::gbm target
    # No version requirement (yet)
    pkg_check_modules(GBM REQUIRED IMPORTED_TARGET gbm)

    include(FindPackageHandleStandardArgs)

    # Sets the FOUND variable to TRUE if all required variables are present and set
    find_package_handle_standard_args(
        gbm
        REQUIRED_VARS
            GBM_INCLUDE_DIRS
            GBM_CFLAGS
            GBM_LDFLAGS
            GBM_LIBRARIES
            GBM_LIBRARY_DIRS
        VERSION_VAR
            GBM_VERSION
    )

    find_library(GBM_ACTUAL_LIBRARY NAMES gbm 
        HINTS ${GBM_LIBRARY_DIRS} )

    if(GBM_FOUND AND NOT TARGET libgbm::libgbm)
        add_library(libgbm::libgbm UNKNOWN IMPORTED)
        set_target_properties(libgbm::libgbm PROPERTIES
            IMPORTED_LOCATION "${GBM_ACTUAL_LIBRARY}"
            INTERFACE_LINK_LIBRARIES "${GBM_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${GBM_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${GBM_INCLUDE_DIRS}"
            )
    else()
        message(FATAL_ERROR "Some required variable(s) is (are) not found / set! Does gbm.pc exist?")
    endif()

    mark_as_advanced(LIBDRM_INCLUDE_DIRS LIBDRM_LIBRARIES)

else()

    message(FATAL_ERROR "Unable to locate PkgConfig")

endif()
