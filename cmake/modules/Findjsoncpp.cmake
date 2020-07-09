# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# - Try to find the JSONCPP library.
#
# The following are set after configuration is done:
#  JSONCPP_FOUND
#  JSONCPP_INCLUDE_DIRS
#  JSONCPP_LIBRARY_DIRS
#  JSONCPP_LIBRARIES

find_path( JSONCPP_INCLUDE_DIR NAMES json.h PREFIX json )
find_library( JSONCPP_LIBRARY NAMES libjsoncpp.so jsoncpp )

message( "JSONCPP_INCLUDE_DIR include dir = ${JSONCPP_INCLUDE_DIR}" )
message( "JSONCPP_LIBRARY lib = ${JSONCPP_LIBRARY}" )

include( FindPackageHandleStandardArgs )

# Handle the QUIETLY and REQUIRED arguments and set the JSONCPP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( JSONCPP DEFAULT_MSG
        JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR )

mark_as_advanced( JSONCPP_INCLUDE_DIR JSONCPP_LIBRARY )

if( JSONCPP_FOUND )
    set( JSONCPP_LIBRARIES ${JSONCPP_LIBRARY} )
    set( JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR} )
endif()

if( JSONCPP_FOUND AND NOT TARGET JsonCpp::JsonCpp )
    add_library( JsonCpp::JsonCpp SHARED IMPORTED )
    set_target_properties( JsonCpp::JsonCpp PROPERTIES
            IMPORTED_LOCATION "${JSONCPP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_INCLUDE_DIRS}" )
endif()