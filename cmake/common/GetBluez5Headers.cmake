option(DOWNLOAD_BLUEZ_UTIL_HEADERS "Download bluez5 headers" OFF)
option(BLUEZ_REMOVE_UINT24T "Removes the uint24_t form the bluez5 headers" ON)

set(DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION "5.78" CACHE STRING "version of the bluez5 headers to download...")
set(DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO "https://github.com/bluez/bluez.git" CACHE STRING "Repo where to get the bluez5 headers...")

set(BLUEZ_LOCAL_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/core/bluez5")
include(CreateLink)

if(DOWNLOAD_BLUEZ_UTIL_HEADERS)
    message(STATUS "Downloaded bluez5 headers are used, assuming your \"the expert\"!")

    include(GetExternalCode)

    set(BLUEZ_SRC "${CMAKE_BINARY_DIR}/bluez5-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}")

    GetExternalCode(
      GIT_REPOSITORY "${DOWNLOAD_BLUEZ_UTIL_HEADERS_REPO}"
      GIT_VERSION "${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}"
      SOURCE_DIR "${BLUEZ_SRC}"
    )

    if(BLUEZ_REMOVE_UINT24T)
        include(ApplyPatch)

        set(BLUEZ_REMOVE_UINT24T_PATCH_CONTENT "
diff --git a/lib/bluetooth.h b/lib/bluetooth.h
index 1286aa7..d403b53 100644
--- a/lib/bluetooth.h
+++ b/lib/bluetooth.h
@@ -443,10 +443,6 @@ void bt_free(void *ptr);
 int bt_error(uint16_t code);
 const char *bt_compidtostr(int id);
 
-typedef struct {
-	uint8_t data[3];
-} uint24_t;
-
 typedef struct {
 	uint8_t data[16];
 } uint128_t;
--
2.43.0")

        set(BLUEZ_REMOVE_UINT24T_PATCH_FILE "${CMAKE_BINARY_DIR}/0001-remove-int24.patch")

        file(WRITE "${BLUEZ_REMOVE_UINT24T_PATCH_FILE}" "${BLUEZ_REMOVE_UINT24T_PATCH_CONTENT}")
        
        if(NOT EXISTS "${BLUEZ_REMOVE_UINT24T_PATCH_FILE}")
            message(FATAL_ERROR "Failed to create patch file: ${BLUEZ_REMOVE_UINT24T_PATCH_FILE}")
        endif()

        ApplyPatch("${BLUEZ_SRC}" "${BLUEZ_REMOVE_UINT24T_PATCH_FILE}")
    endif()

    CreateLink(
      LINK "${BLUEZ_LOCAL_INCLUDE_DIR}"
      TARGET "${CMAKE_BINARY_DIR}/bluez5-${DOWNLOAD_BLUEZ_UTIL_HEADERS_VERSION}/lib"
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

function(GetBluez5IncludeDirs var)
  set(${var} "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/core" PARENT_SCOPE)
endfunction(GetBluez5IncludeDirs)

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
    message(FATAL_ERROR "Could not determine the usage of inclusive language, probably due to a compilation error. Please check the cmake error log for details.")
  elseif(NO_INCLUSIVE_LANGUAGE)
    set(${var} FALSE PARENT_SCOPE)
  elseif(INCLUSIVE_LANGUAGE)
    set(${var} TRUE PARENT_SCOPE)
  endif()  
endfunction(CheckBluez5InclusiveLanguage)
