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

cmake_minimum_required(VERSION 3.15)

project(${NAMESPACE}Broadcast
        VERSION 1.0.0
        DESCRIPTION "Abstraction to parse DVB tables"
        LANGUAGES CXX)

set(TARGET ${PROJECT_NAME})
message("Setup ${TARGET} v${PROJECT_VERSION}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(NXCLIENT QUIET)

add_library(${TARGET} 
        ProgramTable.cpp
        Definitions.cpp
        TunerAdministrator.cpp
        Module.cpp
        )

set(PUBLIC_HEADERS
        broadcast.h
        Definitions.h
        Descriptors.h
        MPEGDescriptor.h
        MPEGSection.h
        MPEGTable.h
        ProgramTable.h
        TunerAdministrator.h
        Services.h
        Networks.h
        TimeDate.h
        Schedule.h
        NIT.h
        SDT.h
        TDT.h
        EIT.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
        )

if(NXCLIENT_FOUND)
    find_package(NEXUS REQUIRED)

    if (BROADCAST_IMPLEMENTATION_PATH)
        target_sources(${TARGET} PRIVATE ${BROADCAST_IMPLEMENTATION_PATH}/Tuner.cpp)
    else()
        include(GetExternalCode)
        set(BROADCAST_IMPLEMENTATION_VERSION "master" CACHE STRING "broadcast implementation version: commit_id_or_tag_or_branch_name")
        set(BROADCAST_IMPLEMENTATION_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/broadcast" CACHE STRING "broadcast implementation repository")
        GetExternalCode(
            GIT_REPOSITORY ${BROADCAST_IMPLEMENTATION_REPOSITORY}
            GIT_VERSION ${BROADCAST_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Implementation/Nexus"
        )
        target_sources(${TARGET} PRIVATE Implementation/Nexus/Tuner.cpp)
    endif()

     target_link_libraries(${TARGET}
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT
        )
else()
    target_sources(${TARGET} PRIVATE Implementation/V4L/Tuner.cpp)
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/broadcast COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/broadcast # headers
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "${PROJECT_DESCRIPTION}" )

InstallCMakeConfig(
        TARGETS ${TARGET})

add_subdirectory(test)
