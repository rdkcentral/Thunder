# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2025 Metrological
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

include(InstallEnvironmentFragments)

set(THUNDER_ENVIRONMENT_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${NAMESPACE}" CACHE STRING "Path to the Thunder environment base directory")
set(THUNDER_ENVIRONMENT_FRAGMENTS_PATH "${THUNDER_ENVIRONMENT_PATH}/environment.d" CACHE STRING "Path to the Thunder environment.d directory")

option(ENABLE_THUNDER_ENVIRONMENT_COREDUMP "Enable support for Thunder environment coredump handling" OFF)
option(ENABLE_THUNDER_ENVIRONMENT_POWERDOWN_DISPLAYS "Enable support for Thunder environment power-down displays at start" OFF)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/environment.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/environment" @ONLY
)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/environment
        DESTINATION ${THUNDER_ENVIRONMENT_PATH}
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        COMPONENT ${NAMESPACE}_Runtime
)

# make sure the fragments directory exists.  
install(
    DIRECTORY DESTINATION "${THUNDER_ENVIRONMENT_FRAGMENTS_PATH}"
    COMPONENT ${NAMESPACE}_Runtime
)

set(ENVIRONMENT_FILES "${CMAKE_CURRENT_LIST_DIR}/_base.env.in")

if(ENABLE_THUNDER_ENVIRONMENT_COREDUMP)
    list(APPEND ENVIRONMENT_FILES "${CMAKE_CURRENT_LIST_DIR}/coredump.sysconf.in")
endif()     

if(ENABLE_THUNDER_ENVIRONMENT_POWERDOWN_DISPLAYS)
    list(APPEND ENVIRONMENT_FILES "${CMAKE_CURRENT_LIST_DIR}/poweroff-displays.sysconf.in")
endif() 

InstallEnvironmentFragments(FILES ${ENVIRONMENT_FILES})
