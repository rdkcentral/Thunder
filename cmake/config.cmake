## This is an stripped version of cmakepp
##
## https://github.com/toeb/cmakepp
##
cmake_minimum_required(VERSION 2.8.7)

get_property(is_included GLOBAL PROPERTY INCLUDE_GUARD)
if(is_included)
  _return()
endif()
set_property(GLOBAL PROPERTY INCLUDE_GUARD true)

cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0012 NEW)
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif()
# installation dir of cmakepp
set(CMAKE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# get temp dir which is needed by a couple of functions in cmakepp
# first uses env variable TMP if it does not exists TMPDIR is used
# if both do not exists current_list_dir/tmp is used
if(UNIX)
  set(TMP_DIR $ENV{TMPDIR} /var/tmp)
else()
  set(TMP_DIR $ENV{TMP}  ${CMAKE_CURRENT_LIST_DIR}/tmp)
endif()
list(GET TMP_DIR 0 TMP_DIR)
file(TO_CMAKE_PATH "${TMP_DIR}" TMP_DIR)

# dummy function which is overwritten and in this form just returns the temp_dir
function(config_dir key)
	return("${TMP_DIR}")
endfunction()

## includes all cmake files of cmakepp 
include("${CMAKE_BASE_DIR}/config/core/require.cmake")

require("${CMAKE_BASE_DIR}/config/*.cmake")
