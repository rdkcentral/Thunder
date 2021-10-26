# - Try to find Broadcom Nexus client.
# Once done this will define
#  NXCLIENT_FOUND     - System has a Nexus client
#  NXCLIENT::NXCLIENT - The Nexus client library
#
# Copyright (C) 2019 Metrological B.V
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

find_path(LIBNXCLIENT_INCLUDE nexus_config.h
        PATH_SUFFIXES refsw)

find_library(LIBNXCLIENT_LIBRARY nxclient)

if(EXISTS "${LIBNXCLIENT_LIBRARY}")
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(NXCLIENT DEFAULT_MSG LIBNXCLIENT_INCLUDE LIBNXCLIENT_LIBRARY)
    mark_as_advanced(LIBNXCLIENT_LIBRARY)

    if(NXCLIENT_FOUND AND NOT TARGET NXCLIENT::NXCLIENT)
        add_library(NXCLIENT::NXCLIENT UNKNOWN IMPORTED)
        set_target_properties(NXCLIENT::NXCLIENT PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBNXCLIENT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBNXCLIENT_INCLUDE}"
                )
    endif()
else()
    if(NXCLIENT_FIND_REQUIRED)
        message(FATAL_ERROR "LIBNXCLIENT_LIBRARY not available")
    elseif(NOT NXCLIENT_FIND_QUIETLY)
        message(STATUS "LIBNXCLIENT_LIBRARY not available")
    endif()
endif()
