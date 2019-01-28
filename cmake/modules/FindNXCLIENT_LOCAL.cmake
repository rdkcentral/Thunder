# - Try to find Broadcom Nexus local client.
# Once done this will define
#  LIBNXCLIENT_LOCAL_LIBRARY_FOUND - System has Nexus local client
#  NXCLIENT_LOCAL::NXCLIENT_LOCAL  - The Nexus local client library
#
# Copyright (C) 2019 Metrological B.V.
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

find_path(LIBNXCLIENT_LOCAL_INCLUDE nexus_config.h
        PATH_SUFFIXES refsw)

find_library(LIBNXCLIENT_LOCAL_LIBRARY nxclient_local)

if(EXISTS "${LIBNXCLIENT_LOCAL_LIBRARY}")
    include(FindPackageHandleStandardArgs)
    
    set(NXCLIENT_LOCAL_FOUND TRUE)
    
    find_package_handle_standard_args(LIBNXCLIENT_LOCAL_LIBRARY DEFAULT_MSG LIBNXCLIENT_LOCAL_INCLUDE LIBNXCLIENT_LOCAL_LIBRARY)
    mark_as_advanced(LIBNXCLIENT_LOCAL_LIBRARY)

    if(NOT TARGET NXCLIENT_LOCAL::NXCLIENT_LOCAL)
        add_library(NXCLIENT_LOCAL::NXCLIENT_LOCAL UNKNOWN IMPORTED)
        set_target_properties(NXCLIENT_LOCAL::NXCLIENT_LOCAL PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBNXCLIENT_LOCAL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBNXCLIENT_LOCAL_INCLUDE}"
    endif()
endif()