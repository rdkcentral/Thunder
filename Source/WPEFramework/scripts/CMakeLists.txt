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

set(SYSTEMD_SERVICE "false" CACHE STRING "Select systemd service scripts, by default it install initv scripts")
set(SYSTEM_ROOT_PATH "root" CACHE STRING "System root directory location")

if (SYSTEMD_SERVICE)
    file(READ wpeframework.service.in WPEFRAMEWORK_SYSTEMD_SERVICE)
    string(
           REGEX REPLACE "EXTRA_DEPENDS" "${SYSTEMD_EXTRA_DEPENDS}"
           MOD_WPEFRAMEWORK_SYSTEMD_SERVICE "${WPEFRAMEWORK_SYSTEMD_SERVICE}"
    )
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/wpeframework.service "${MOD_WPEFRAMEWORK_SYSTEMD_SERVICE}")

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/../lib)
    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/wpeframework.service
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../${SYSTEMD_INSTALL_PATH}/system/
    )
else()
    file(READ wpeframework.in WPEFRAMEWORK_INITD)
    string(
           REGEX REPLACE "ROOT_PATH" "${SYSTEM_ROOT_PATH}/${NAMESPACE}"
           MOD_WPEFRAMEWORK_INITD "${WPEFRAMEWORK_INITD}"
    )
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/wpeframework "${MOD_WPEFRAMEWORK_INITD}")

    install(
            FILES ${CMAKE_CURRENT_BINARY_DIR}/wpeframework
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/init.d/
    )
endif()

