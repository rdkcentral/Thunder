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

set(TARGET ${NAMESPACE}Bluetooth)

find_package(Bluez REQUIRED)
find_package(BCM_HOST QUIET)
find_package(NEXUS QUIET)

add_library(${TARGET} SHARED
        HCISocket.cpp
        GATTSocket.cpp
        Profile.cpp
        UUID.cpp
        Definitions.cpp
        Module.cpp)

set(PUBLIC_HEADERS
        IDriver.h
        HCISocket.h
        GATTSocket.h
        Profile.h
        UUID.h
        Module.h
        bluetooth.h
        )

if (BCM_HOST_FOUND)
    target_sources(${TARGET} PRIVATE drivers/BCM43XX.cpp)
else ()
    target_sources(${TARGET} PRIVATE drivers/Basic.cpp)
endif ()

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        PRIVATE
          ${BLUEZ_LIBRARIES}
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/drivers"
          ${BLUEZ_INCLUDE_DIRS}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/bluetooth COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Bluetooth library" )

InstallCMakeConfig(
        TARGETS ${TARGET})
