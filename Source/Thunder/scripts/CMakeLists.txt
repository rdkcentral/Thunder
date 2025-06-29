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
option(UDEV_VIDEO_RULE "Install the a thunder tailored udev rule to access the video device" OFF)

option(ENABLE_THUNDER_ENVIRONMENT "support loading thunder system environment fragments (*.env, *.sysconf)" OFF)

if (SYSTEMD_SERVICE)
    set(SYSTEMD_EXTRA_DEPENDS "" CACHE STRING "Adds extra dependecies to the systemd service")
    
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/thunder.service.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/thunder.service" @ONLY)

    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/thunder.service
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/systemd/system/ 
            COMPONENT ${NAMESPACE}_Runtime
    )

endif()

if (INITV_SCRIPT)
    set(SYSV_INIT_LEVEL "80" CACHE STRING "Level of the initv start script")
    set(SYSTEM_ROOT_PATH "/root" CACHE STRING "System root directory location")
    set(ROOT_PATH "${SYSTEM_ROOT_PATH}/${NAMESPACE}" CACHE STRING "Thunders root path")

    if(ENABLE_THUNDER_ENVIRONMENT)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/thunder.env.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/thunder" @ONLY)
    else()
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/thunder.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/thunder" @ONLY)
    endif()
    
    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/thunder
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            RENAME S${SYSV_INIT_LEVEL}thunder
            DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/init.d/ 
            COMPONENT ${NAMESPACE}_Runtime
    )
endif()

if (UDEV_VIDEO_RULE)
    set(PLATFORM_UDEV_VIDEO_LEVEL "20" CACHE STRING "Level of the udev rule")
    set(PLATFORM_VIDEO_SUBSYSTEM "" CACHE STRING "Subsystem to set the video group to be set on")
    set(PLATFORM_VIDEO_DEVICE_GROUP "" CACHE STRING "Group name that needs access to the video device")

    if(NOT "${PLATFORM_VIDEO_SUBSYSTEM}" STREQUAL "" AND NOT "${PLATFORM_VIDEO_DEVICE_GROUP}" STREQUAL "")
        set(UDEV_RULE_SUBSYSTEM "${PLATFORM_VIDEO_SUBSYSTEM}")
        set(UDEV_RULE_GROUP "${PLATFORM_VIDEO_DEVICE_GROUP}")

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/set-subsystem-group-udev.rules.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/video-device-udev.rules" @ONLY)

        unset(UDEV_RULE_SUBSYSTEM)
        unset(UDEV_RULE_GROUP)

        install(
                FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/video-device-udev.rules
                PERMISSIONS OWNER_WRITE OWNER_READ
                RENAME ${PLATFORM_UDEV_VIDEO_LEVEL}-thunder-video-device-udev.rules
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/udev/rules.d
                COMPONENT ${NAMESPACE}_Runtime
        )
    else()
        message(SEND_ERROR "Udev video device rule is enabled, but there is missing information.")
    endif()
endif()

if(ENABLE_THUNDER_ENVIRONMENT)
add_subdirectory(environment)
endif()
