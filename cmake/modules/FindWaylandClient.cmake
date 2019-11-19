# - Try to find Wayland.
# Once done, this will define
#
#  WAYLANDCLIENT_FOUND - system has Wayland.
#  WAYLANDCLIENT_INCLUDE_DIRS - the Wayland include directories
#  WAYLANDCLIENT_LIBRARIES - link these to use Wayland.
#
#  WaylandClient::WaylandClient, the wayland client library
#
# Copyright (C) 2014 Igalia S.L.
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
pkg_check_modules(WAYLANDCLIENT wayland-client>=1.2 )

find_library(WAYLANDCLIENT_LIB NAMES wayland-client
        HINTS ${WAYLANDCLIENT_LIBDIR} ${WAYLANDCLIENT_LIBRARY_DIRS})

if(WAYLANDCLIENT_FOUND AND NOT TARGET WaylandClient::WaylandClient)
    add_library(WaylandClient::WaylandClient UNKNOWN IMPORTED)
    set_target_properties(WaylandClient::WaylandClient PROPERTIES
            IMPORTED_LOCATION "${WAYLANDCLIENT_LIB}"
            INTERFACE_LINK_LIBRARIES "${WAYLANDCLIENT_LIBRARIES}"
            INTERFACE_COMPILE_OPTIONS "${WAYLANDCLIENT_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${WAYLANDCLIENT_INCLUDE_DIRS}"
            )
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAYLAND DEFAULT_MSG WAYLANDCLIENT_FOUND)
mark_as_advanced(WAYLANDCLIENT_INCLUDE_DIRS WAYLANDCLIENT_LIBRARIES)
