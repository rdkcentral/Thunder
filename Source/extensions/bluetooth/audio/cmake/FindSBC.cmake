# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2021 Metrological
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

# - Try to find sbc
# Once done this will define
#  SBC_FOUND - System has libsbc
#  SBC_INCLUDE_DIRS - The libsbc include directories
#  SBC_LIBRARIES - The libraries needed to use libsbc

find_package(PkgConfig)
pkg_check_modules(SBC REQUIRED sbc IMPORTED_TARGET)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SBC DEFAULT_MSG SBC_LIBRARIES SBC_INCLUDE_DIRS)

mark_as_advanced(SBC_FOUND SBC_LIBRARIES SBC_INCLUDE_DIRS)

if(SBC_FOUND)
   add_library(SBC::SBC ALIAS PkgConfig::SBC)
endif()