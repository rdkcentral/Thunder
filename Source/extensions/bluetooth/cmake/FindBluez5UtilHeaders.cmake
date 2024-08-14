# - Try to find Bluez Utils Headers
# Once done this will define
#  Bluez5UtilHeaders::Bluez5UtilHeaders - The bluez include directories
#
# Copyright (C) 2022 Metrological.
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

if(Bluez5UtilHeaders_FIND_QUIETLY)
    set(_FIND_MODE QUIET)
elseif(Bluez5UtilHeaders_FIND_REQUIRED)
    set(_FIND_MODE REQUIRED)
endif()

set(NEEDED_BLUEZ_HEADERS
  bluetooth.h 
  hci.h
  mgmt.h
  l2cap.h
)

set(BLUEZ_INCLUDE_DIRS)

foreach(_header ${NEEDED_BLUEZ_HEADERS})
  find_path(_header_path  bluetooth/${_header})
  if(_header_path)
    message(VERBOSE "Found ${_header} in ${_header_path}")
    list(APPEND BLUEZ_INCLUDE_DIRS ${_header_path})
  endif()
endforeach()

list(REMOVE_DUPLICATES BLUEZ_INCLUDE_DIRS)

add_library(Bluez5UtilHeaders INTERFACE)

target_include_directories(Bluez5UtilHeaders
  INTERFACE
    INTERFACE_INCLUDE_DIRECTORIES ${BLUEZ_INCLUDE_DIRS}
)

add_library(Bluez5UtilHeaders::Bluez5UtilHeaders ALIAS Bluez5UtilHeaders)

message(TRACE "BLUEZ_INCLUDE_DIRS ${BLUEZ_INCLUDE_DIRS}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bluez5UtilHeaders DEFAULT_MSG BLUEZ_INCLUDE_DIRS)
mark_as_advanced(BLUEZ_INCLUDE_DIRS)
