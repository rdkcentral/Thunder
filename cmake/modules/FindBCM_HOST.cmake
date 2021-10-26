# - Try to find bcm_host.
# Once done, this will define
#
#  BCM_HOST_FOUND - the bcm_host is available
#  BCM_HOST::BCM_HOST - The bcm_host library and all its dependecies
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

if(BCM_HOST_FIND_QUIETLY)
    set(_BCM_HOST_MODE QUIET)
elseif(BCM_HOST_FIND_REQUIRED)
    set(_BCM_HOST_MODE REQUIRED)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_BCM_HOST ${_BCM_HOST_MODE} bcm_host)

if(${PC_BCM_HOST_FOUND})
    find_library(BCM_HOST_LIBRARY bcm_host
        HINTS ${PC_BCM_LIBDIR} ${PC_BCM_LIBRARY_DIRS}
    )
    set(BCM_LIBRARIES ${PC_BCM_HOST_LIBRARIES})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(BCM_HOST DEFAULT_MSG PC_BCM_HOST_FOUND PC_BCM_HOST_INCLUDE_DIRS BCM_HOST_LIBRARY PC_BCM_HOST_LIBRARIES)
    mark_as_advanced(PC_BCM_HOST_INCLUDE_DIRS PC_BCM_HOST_LIBRARIES)

    if(BCM_HOST_FOUND AND NOT TARGET BCM_HOST::BCM_HOST)
        add_library(BCM_HOST::BCM_HOST UNKNOWN IMPORTED)

        set_target_properties(BCM_HOST::BCM_HOST
            PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${BCM_HOST_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS "PLATFORM_RPI"
                INTERFACE_COMPILE_OPTIONS "${PC_BCM_HOST_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_BCM_HOST_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${PC_BCM_HOST_LIBRARIES}"
                )
    endif()
endif()
