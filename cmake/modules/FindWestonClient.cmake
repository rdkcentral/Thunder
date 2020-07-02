# - Try to find Weston library.
# Once done this will define
#  WESTON_FOUND - System has weston
#  WESTON_INCLUDE_DIRS - The weston include directories
#  WESTON_LIBRARIES    - The libraries needed to use weston
#
#  WESTON::WESTON, the weston compositor
#
# Copyright (C) 2015 Metrological.
# Copyright (C) 2019 Linaro.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_WESTON libweston-desktop-8)

find_library(WESTON_CLIENT_LIB NAMES weston-desktop-8
        HINTS ${PC_WESTON_LIBDIR} ${PC_WESTON_LIBRARY_DIRS}
)

if(PC_WESTON_FOUND AND NOT TARGET WestonClient::WestonClient)
    add_library(WestonClient::WestonClient UNKNOWN IMPORTED)
    set_target_properties(WestonClient::WestonClient PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${WESTON_CLIENT_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES ""
            INTERFACE_COMPILE_OPTIONS "${PC_WESTON_DEFINITIONS}"
            INTERFACE_LINK_LIBRARIES "${PC_WESTON_LIBRARIES}"
            )
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_WESTON DEFAULT_MSG PC_WESTON_FOUND)

mark_as_advanced(PC_WESTON_LIBRARIES)
