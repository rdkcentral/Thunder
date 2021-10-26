# - Try to find Bluez
# Once done this will define
#  BLUEZ_FOUND - System has bluez
#  BLUEZ_INCLUDE_DIRS - The bluez include directories
#  BLUEZ_LIBRARIES - The libraries needed to use bluez
#
# Copyright (C) 2019 Metrological.
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
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE

if(Bluez_FIND_QUIETLY)
    set(_BLUEZ_MODE QUIET)
elseif(Bluez_FIND_REQUIRED)
    set(_BLUEZ_MODE REQUIRED)
endif()

find_package(PkgConfig)

pkg_check_modules(PC_BLUEZ ${_BLUEZ_MODE} bluez)

if(PC_BLUEZ_FOUND)
    set(BLUEZ_CFLAGS ${PC_BLUEZ_CFLAGS})
    set(BLUEZ_FOUND ${PC_BLUEZ_FOUND})
    set(BLUEZ_DEFINITIONS ${PC_BLUEZ_CFLAGS_OTHER})
    set(BLUEZ_NAMES ${PC_BLUEZ_LIBRARIES})
    foreach (_library ${BLUEZ_NAMES})
        find_library(BLUEZ_LIBRARIES_${_library} ${_library}
        HINTS ${PC_BLUEZ_LIBDIR} ${PC_BLUEZ_LIBRARY_DIRS}
        )
        set(BLUEZ_LIBRARIES ${BLUEZ_LIBRARIES} ${BLUEZ_LIBRARIES_${_library}})
    endforeach ()
else ()
    set(BLUEZ_NAMES ${BLUEZ_NAMES} bluez BLUEZ)
    find_library(BLUEZ_LIBRARIES NAMES ${BLUEZ_NAMES}
        HINTS ${PC_BLUEZ_LIBDIR} ${PC_BLUEZ_LIBRARY_DIRS}
    )
endif ()

find_path(BLUEZ_INCLUDE_DIR_BLUEZ NAMES bluetooth/bluetooth.h
    HINTS ${PC_BLUEZ_INCLUDEDIR} ${PC_BLUEZ_INCLUDE_DIRS}
)

set(BLUEZ_INCLUDE_DIRS ${BLUEZ_INCLUDE_DIR_BLUEZ} ${BLUEZ_INCLUDE_DIR_BLUEZ_ARCH} CACHE FILEPATH "FIXME")
set(BLUEZ_LIBRARIES ${BLUEZ_LIBRARIES} CACHE FILEPATH "FIXME")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bluez DEFAULT_MSG BLUEZ_INCLUDE_DIRS BLUEZ_LIBRARIES)

mark_as_advanced(BLUEZ_INCLUDE_DIRS BLUEZ_LIBRARIES)

