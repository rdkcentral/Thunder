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

find_package(PkgConfig)
pkg_check_modules(PC_DBusClient dbus-client)

if(${PC_DBusClient_FOUND})
    find_library(DBusClient_LIBRARY dbus_client
        HINTS ${PC_DBusClient_LIBDIR} ${PC_DBusClient_LIBRARY_DIRS} REQUIRED
        )
    find_path(DBusClient_INCLUDE DBusClient.h
        PATH_SUFFIXES dbus-client
        )

    if(NOT TARGET DBusClient::DBusClient)
        add_library(DBusClient::DBusClient UNKNOWN IMPORTED)

        set_target_properties(DBusClient::DBusClient
            PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${DBusClient_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${DBusClient_INCLUDE}"
            INTERFACE_LINK_LIBRARIES "${PC_DBusClient_LIBRARIES}"
        )
    endif()
endif()
