# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
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

set(TARGET ${NAMESPACE}ProcessContainers)

# Backend options
option(PROCESSCONTAINERS_LXC
        "Use LXC backend." OFF)
option(PROCESSCONTAINERS_RUNC
        "Use RunC backend." OFF)
option(PROCESSCONTAINERS_CRUN
        "Use CRun backend." OFF)
option(PROCESSCONTAINERS_CLIB
        "Use c-library backend." OFF)
option(PROCESSCONTAINERS_DOBBY
        "Use Dobby backend." OFF)
option(PROCESSCONTAINERS_AWC
        "Use AWC/slauncher - One Middleware backend." OFF)

# Construct a library object
add_library(${TARGET} SHARED
        Module.cpp
        )

# Selects backend
if (PROCESSCONTAINERS_LXC)
        target_sources(${TARGET} PRIVATE implementations/LXCImplementation/LXCImplementation.cpp)
elseif(PROCESSCONTAINERS_RUNC)
        target_sources(${TARGET} PRIVATE implementations/RunCImplementation/RunCImplementation.cpp
        )
elseif(PROCESSCONTAINERS_CRUN)
        target_sources(${TARGET} PRIVATE implementations/CRunImplementation/CRunImplementation.cpp
        )
elseif(PROCESSCONTAINERS_CLIB)
        target_sources(${TARGET} PRIVATE implementations/RunCImplementation/RunCImplementation.cpp)
elseif(PROCESSCONTAINERS_DOBBY)
        target_sources(${TARGET} PRIVATE implementations/DobbyImplementation/DobbyImplementation.cpp)
elseif(PROCESSCONTAINERS_AWC)
        target_sources(
            ${TARGET} PRIVATE
            implementations/AWCImplementation/AWC.cpp
            implementations/AWCImplementation/AWCContainerAdministrator.cpp
            implementations/AWCImplementation/AWCContainerBase.cpp
            implementations/AWCImplementation/AWCImplementation.cpp
            implementations/AWCImplementation/AWCProxyContainer.cpp
            implementations/AWCImplementation/dbus/Client.cpp
            implementations/AWCImplementation/dbus/api.c)
endif()

set(PUBLIC_HEADERS
        ProcessContainer.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include>
        )

if (PROCESSCONTAINERS_LXC)
        find_package(LXC REQUIRED)
        target_link_libraries(${TARGET}
                PRIVATE
                LXC::LXC
                CompileSettingsDebug::CompileSettingsDebug
                )

        target_include_directories( ${TARGET}
                PRIVATE
                LXC::LXC
                )
elseif (PROCESSCONTAINERS_CRUN)
        find_package(LibCRUN REQUIRED)

        target_link_libraries(${TARGET}
                PRIVATE
                libcrun::libcrun
                )

        target_include_directories( ${TARGET}
                PRIVATE
                libcrun::libcrun
        )
elseif (PROCESSCONTAINERS_DOBBY)
        find_package(Dobby REQUIRED CONFIG)

        find_package(Systemd REQUIRED)

        target_link_libraries(${TARGET}
                PRIVATE
                DobbyClientLib
                Systemd::Systemd
        )
elseif (PROCESSCONTAINERS_AWC)
        find_package(LXC REQUIRED)
        find_package(Slauncher REQUIRED)
        find_package(LibGio REQUIRED)
        find_package(LibGioUnix REQUIRED)
        find_package(LibGlib REQUIRED)
        find_package(LibGobject REQUIRED)
        target_link_libraries(${TARGET}
                PRIVATE
                Slauncher::Slauncher
                CompileSettingsDebug::CompileSettingsDebug
                ${LIBGIO_INCLUDE_LIBRARIES}
                ${LIBGIOUNIX_LIBRARIES}
                ${LIBGLIB_LIBRARIES}
                ${LIBGOBJECT_LIBRARIES}
                )

        target_include_directories( ${TARGET}
                PRIVATE
                Slauncher::Slauncher
                ${LIBGIO_INCLUDE_DIRS}
                ${LIBGIOUNIX_INCLUDE_DIRS}
                ${LIBGLIB_INCLUDE_DIRS}
                ${LIBGOBJECT_INCLUDE_DIRS}
                )
endif()

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/processcontainers COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/processcontainers
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Enable ease of development for Process Container support." )
