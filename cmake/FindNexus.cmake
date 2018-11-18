# - Try to find BroadCom RefSW.
# Once done this will define
#  BCMREFSW_FOUND - System has Nexus
#  BCMREFSW_INCLUDE_DIRS - The Nexus include directories
#
#  All variable from platform_app.inc are available except:
#  - NEXUS_PLATFORM_VERSION_NUMBER.
#
# Copyright (C) 2015 Metrological.
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

find_path(LIBNEXUS_INCLUDE nexus_config.h
        PATH_SUFFIXES refsw)

find_library(LIBNEXUS_LIBRARY nexus)

find_library(LIBB_OS_LIBRARY b_os)

find_library(LIBNXCLIENT_LOCAL_LIBRARY nxclient_local)

find_library(LIBNXCLIENT_LIBRARY nxclient)

find_library(LIBNEXUS_CLIENT_LIBRARY nexus_client)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBNEXUS DEFAULT_MSG LIBNEXUS_INCLUDE LIBNEXUS_LIBRARY)

mark_as_advanced(LIBNEXUS_INCLUDE LIBNEXUS_LIBRARY)

if(EXISTS "${LIBNXCLIENT_LIBRARY}")
  find_package_handle_standard_args(LIBNXCLIENT DEFAULT_MSG LIBNEXUS_INCLUDE LIBNXCLIENT_LIBRARY)
  mark_as_advanced(LIBNXCLIENT_LIBRARY)
endif()


if(NOT TARGET NEXUS::NEXUS)
    add_library(NEXUS::NEXUS UNKNOWN IMPORTED)
    if(EXISTS "${LIBNEXUS_LIBRARY}")
        set_target_properties(NEXUS::NEXUS PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBNEXUS_INCLUDE}"
                    )

        if(NOT EXISTS "${LIBNEXUS_CLIENT_LIBRARY}")
            message(STATUS "Nexus in Proxy mode")
            set_target_properties(NEXUS::NEXUS PROPERTIES
                    IMPORTED_LOCATION "${LIBNEXUS_LIBRARY}"
                    )
        else()

            message(STATUS "Nexus in Client mode")
            set_target_properties(NEXUS::NEXUS PROPERTIES
                    IMPORTED_LOCATION "${LIBNEXUS_CLIENT_LIBRARY}"
                    )
        endif()

        if(NOT EXISTS "${LIBNXCLIENT_LIBRARY}")
            set_target_properties(NEXUS::NEXUS PROPERTIES
                 INTERFACE_COMPILE_DEFINITIONS NO_NXCLIENT
                 )
        endif()

        if(EXISTS "${LIBB_OS_LIBRARY}")
            set_target_properties(NEXUS::NEXUS PROPERTIES
                    IMPORTED_LINK_INTERFACE_LIBRARIES "${LIBB_OS_LIBRARY}"
                    )
        endif()
    endif()
endif()

if(NOT TARGET NEXUS::NXCLIENT)
    add_library(NEXUS::NXCLIENT UNKNOWN IMPORTED)
    if(EXISTS "${LIBNXCLIENT_LIBRARY}")
        set_target_properties(NEXUS::NXCLIENT PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBNXCLIENT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBNEXUS_INCLUDE}"
                )
    endif()
endif()

if(NOT TARGET NEXUS::NXCLIENT_LOCAL)
    add_library(NEXUS::NXCLIENT_LOCAL UNKNOWN IMPORTED)
    if(EXISTS "${LIBNXCLIENT_LOCAL_LIBRARY}")
        set_target_properties(NEXUS::NXCLIENT_LOCAL PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LIBNXCLIENT_LOCAL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LIBNEXUS_INCLUDE}"
                )
    endif()
endif()