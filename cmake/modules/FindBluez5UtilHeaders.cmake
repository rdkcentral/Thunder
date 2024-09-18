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

option(DOWNLOAD_BLUEZ_UTIL_HEADERS "Download bluez5 headers, only for testing or development..." OFF)
set(DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION "5.78" CACHE STRING "version of the bluez5 headers to download...")
set(DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO "https://github.com/bluez/bluez.git" CACHE STRING "Repo where to get the bluez5 headers...")

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

function(CheckNoInclusiveLanguage)
  include(CheckStructHasMember)

  set(CMAKE_REQUIRED_FLAGS "-Wno-error")

  check_struct_has_member("struct mgmt_ltk_info" central 
    "${BLUEZ_INCLUDE_DIRS}/bluetooth/bluetooth.h;${BLUEZ_INCLUDE_DIRS}/bluetooth/mgmt.h" 
    _NO_INCLUSIVE_LANGUAGE 
    LANGUAGE C) 

  set(NO_INCLUSIVE_LANGUAGE ${_NO_INCLUSIVE_LANGUAGE} PARENT_SCOPE)
endfunction()

if(NOT TARGET Bluez5UtilHeaders)
  set(BLUEZ_INCLUDE_DIRS)

  if(DOWNLOAD_BLUEZ_UTIL_HEADERS AND NOT TARGET DownloadedBluez5Headers)
      message(STATUS "Downloaded bluez5 headers are used, assuming your \"the expert\"!")

      include(GetExternalCode)
      include(CreateLink)

      GetExternalCode(
        GIT_REPOSITORY "${DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO}"
        GIT_VERSION "${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/bluez-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}"
      )

      add_library(DownloadedBluez5Headers INTERFACE)

      #FIX ME: Hack for weird include paths in the source... 
      CreateLink(
        LINK "${CMAKE_BINARY_DIR}/bluez/include/bluetooth"
        TARGET "${CMAKE_BINARY_DIR}/bluez-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}/lib"
      )

      set(BLUEZ_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/bluez/include")
  else()
    foreach(_header ${NEEDED_BLUEZ_HEADERS})
      find_path(_header_path  bluetooth/${_header})
      if(_header_path)
        message(VERBOSE "Found ${_header} in ${_header_path}")
        list(APPEND BLUEZ_INCLUDE_DIRS ${_header_path})
      endif()
    endforeach()

    list(REMOVE_DUPLICATES BLUEZ_INCLUDE_DIRS)
  endif()

  add_library(Bluez5UtilHeaders INTERFACE)
  add_library(Bluez5UtilHeaders::Bluez5UtilHeaders ALIAS Bluez5UtilHeaders)

  #
  # From Bluez >= v5.64 the mgmt_ltk_info struct is changed due to inclusive language changes.
  # https://github.com/bluez/bluez/commit/b7d6a7d25628e9b521a29a5c133fcadcedeb2102
  #
  include(CheckStructHasMember)

  CheckNoInclusiveLanguage()

  if(${NO_INCLUSIVE_LANGUAGE})
      message(VERBOSE "Your bluez version does not use inclusive language anymore")
      target_compile_definitions(Bluez5UtilHeaders INTERFACE NO_INCLUSIVE_LANGUAGE)
  endif()

  if(DOWNLOAD_BLUEZ_UTIL_HEADERS)
    target_include_directories(Bluez5UtilHeaders
      INTERFACE
        $<BUILD_INTERFACE:${BLUEZ_INCLUDE_DIRS}>  
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/Bluez5UtilHeaders/include>
    )

    file(GLOB Bluez5UtilHeadersFiles "${CMAKE_BINARY_DIR}/bluez-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}/lib/*.h")

    install(FILES ${Bluez5UtilHeadersFiles}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/Bluez5UtilHeaders/include/bluetooth COMPONENT ${NAMESPACE}_Development)
  else()
    target_include_directories(Bluez5UtilHeaders
    INTERFACE
      ${BLUEZ_INCLUDE_DIRS})
  endif()

  install(TARGETS Bluez5UtilHeaders EXPORT Bluez5UtilHeadersTargets)

  include(HeaderOnlyInstall)
  HeaderOnlyInstallCMakeConfig(TARGET Bluez5UtilHeaders TREAT_AS_NORMAL)

  message(TRACE "BLUEZ_INCLUDE_DIRS ${BLUEZ_INCLUDE_DIRS}")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Bluez5UtilHeaders DEFAULT_MSG BLUEZ_INCLUDE_DIRS)
  mark_as_advanced(BLUEZ_INCLUDE_DIRS)
endif()
