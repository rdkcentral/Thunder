# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2021 Metrological
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

option(SYSTEMD_SERVICE "Install the systemd service" OFF)
option(INITV_SCRIPT "Install the initv script" ON)

if (SYSTEMD_SERVICE)
    set(SYSTEMD_EXTRA_DEPENDS "" CACHE STRING "Adds extra dependecies to the systemd service")
    
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/thunder.service.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/thunder.service" @ONLY)

    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/thunder.service
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../${SYSTEMD_INSTALL_PATH}/system/ 
            COMPONENT ${NAMESPACE}_Runtime
    )

endif()

if (INITV_SCRIPT)
    set(SYSV_INIT_LEVEL "80" CACHE STRING "Level of the initv start script")
    set(SYSTEM_ROOT_PATH "/root" CACHE STRING "System root directory location")
    set(ROOT_PATH "${SYSTEM_ROOT_PATH}/${NAMESPACE}" CACHE STRING "Thunders root path")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/thunder.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/thunder" @ONLY)

    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/thunder
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            RENAME S${SYSV_INIT_LEVEL}thunder
            DESTINATION ../${CMAKE_INSTALL_SYSCONFDIR}/init.d/ 
            COMPONENT ${NAMESPACE}_Runtime
    )
endif()

