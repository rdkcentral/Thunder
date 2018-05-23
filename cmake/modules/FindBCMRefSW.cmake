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

find_library(LIBNXCLIENT_LIBRARY nxclient)

if (LIBNXCLIENT_LIBRARY STREQUAL "LIBNXCLIENT_LIBRARY-NOTFOUND")
add_definitions(-DNO_NXCLIENT)
set(BCMREFSW_LIBRARIES ${LIBNEXUS_LIBRARY})
else ()
set(BCMREFSW_LIBRARIES ${LIBNEXUS_LIBRARY} ${LIBNXCLIENT_LIBRARY})
endif ()
set(BCMREFSW_INCLUDE_DIRS ${LIBNEXUS_INCLUDE})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(BCMREFSW DEFAULT_MSG BCMREFSW_INCLUDE_DIRS BCMREFSW_LIBRARIES)

mark_as_advanced(BCMREFSW_INCLUDE_DIRS BCMREFSW_LIBRARIES)

