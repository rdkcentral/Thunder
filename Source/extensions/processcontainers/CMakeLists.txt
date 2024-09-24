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
message("Setup ${TARGET}")

# Backend options
option(PROCESSCONTAINERS_LXC "Use LXC backend" OFF)
option(PROCESSCONTAINERS_RUNC "Use RunC backend" OFF)
option(PROCESSCONTAINERS_CRUN "Use CRun backend" OFF)
option(PROCESSCONTAINERS_DOBBY "Use Dobby backend" OFF)
option(PROCESSCONTAINERS_AWC "Use AWC/slauncher - One Middleware backend" OFF)

# Construct a library object
add_library(${TARGET} SHARED
        Module.cpp
        ContainerAdministrator.cpp
        )

# Selects backend
if(PROCESSCONTAINERS_LXC)
        add_subdirectory(implementations/LXCImplementation)
endif()

if(PROCESSCONTAINERS_RUNC)
        add_subdirectory(implementations/RunCImplementation)
endif()

if(PROCESSCONTAINERS_CRUN)
        add_subdirectory(implementations/CRunImplementation)
endif()

if(PROCESSCONTAINERS_DOBBY)
        add_subdirectory(implementations/DobbyImplementation)
endif()

if(PROCESSCONTAINERS_AWC)
        add_subdirectory(implementations/AWCImplementation)
endif()

if("${CONTAINERS}" STREQUAL "")
        message(FATAL_ERROR "No container runtime backend selected!")
endif()

set(PUBLIC_HEADERS
        processcontainers.h
        IProcessContainers.h
        ContainerAdministrator.h
        Module.h
        )

if(APPLE)
    target_link_libraries(${TARGET}
        PRIVATE
        CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        "-Wl,-force_load" ${CONTAINERS}
    )
else()
    target_link_libraries(${TARGET}
        PRIVATE
        CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        -Wl,--whole-archive ${CONTAINERS} -Wl,--no-whole-archive
    )
endif()

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
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

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
