option(DOWNLOAD_BLUEZ_UTIL_HEADERS "Download bluez5 headers" OFF)
set(DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION "5.78" CACHE STRING "version of the bluez5 headers to download...")
set(DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO "https://github.com/bluez/bluez.git" CACHE STRING "Repo where to get the bluez5 headers...")

set(BLUEZ_LOCAL_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/bluez")

include(CreateLink)

if(DOWNLOAD_BLUEZ_UTIL_HEADERS)
    message(STATUS "Downloaded bluez5 headers are used, assuming your \"the expert\"!")

    include(GetExternalCode)


    GetExternalCode(
      GIT_REPOSITORY "${DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO}"
      GIT_VERSION "${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}"
      SOURCE_DIR "${CMAKE_BINARY_DIR}/bluez-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}"
    )

    CreateLink(
      LINK "${BLUEZ_LOCAL_INCLUDE_DIR}"
      TARGET "${CMAKE_BINARY_DIR}/bluez-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}/lib"
    )

else()
    set(BLUEZ_INCLUDE_DIR)
    
    # Find the bluez5 headers in a sysroot/staging location
    find_path(_header_path  bluetooth/bluetooth.h)
    if(_header_path)
      message(VERBOSE "Found bluetooth.h in ${_header_path}")
      set(BLUEZ_INCLUDE_DIR "${_header_path}")
    endif()

    CreateLink(
      LINK "${BLUEZ_LOCAL_INCLUDE_DIR}"
      TARGET "${BLUEZ_INCLUDE_DIR}/bluetooth"
    )
endif()

function(GetBluez5UtilHeadersFiles var)
  file(GLOB Bluez5UtilHeadersFiles "${BLUEZ_LOCAL_INCLUDE_DIR}/*.h")
  set(${var} ${Bluez5UtilHeadersFiles} PARENT_SCOPE)
endfunction(GetBluez5UtilHeadersFiles)

function(GetBluez5IncludeDir var)
  set(${var} ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
endfunction(GetBluez5IncludeDir)

#
# From Bluez >= v5.64 the mgmt_ltk_info struct is changed due to inclusive language changes.
# https://github.com/bluez/bluez/commit/b7d6a7d25628e9b521a29a5c133fcadcedeb2102
# This a function so CMAKE_REQUIRED_FLAGS is only valid in this scope 
# it sets the `var` variable in the calling scope accordingly.
#
function(CheckBluez5InclusiveLanguage var)
  include(CheckStructHasMember)

  # is needed because of pedenic warnings in the bluez headers
  set(CMAKE_REQUIRED_FLAGS "-Wno-error")

  check_struct_has_member("struct mgmt_ltk_info" master 
    "${BLUEZ_LOCAL_INCLUDE_DIR}/bluetooth.h;${BLUEZ_LOCAL_INCLUDE_DIR}/mgmt.h" 
    INCLUSIVE_LANGUAGE 
    LANGUAGE C) 

  check_struct_has_member("struct mgmt_ltk_info" central 
    "${BLUEZ_LOCAL_INCLUDE_DIR}/bluetooth.h;${BLUEZ_LOCAL_INCLUDE_DIR}/mgmt.h" 
    NO_INCLUSIVE_LANGUAGE 
    LANGUAGE C) 

  if(NOT INCLUSIVE_LANGUAGE AND NOT NO_INCLUSIVE_LANGUAGE)
    message(FATAL_ERROR "Could not determen the usage of inclusive launguage, probaly diue to a compilation error. Please check the cmake error log for details.")
  elseif(NO_INCLUSIVE_LANGUAGE)
    set(${var} FALSE PARENT_SCOPE)
  elseif(INCLUSIVE_LANGUAGE)
    set(${var} TRUE PARENT_SCOPE)
  endif()  
endfunction(CheckBluez5InclusiveLanguage)
