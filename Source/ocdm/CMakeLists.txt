# If not stated otherwise in this file or this component's LICENSE file the
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

set(TARGET ocdm)

option(CDMI_ADAPTER_IMPLEMENTATION "Defines which implementation is used." "None")

ProxyStubGenerator(NAMESPACE "OCDM" INPUT "${CMAKE_CURRENT_SOURCE_DIR}" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs")

add_library(${TARGET} SHARED
        open_cdm.cpp
        open_cdm_ext.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs/ProxyStubs_OCDM.cpp"
        )

set(PUBLIC_HEADERS
        DataExchange.h
        IOCDM.h
        open_cdm.h
        adapter/open_cdm_adapter.h
        open_cdm_ext.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<INSTALL_INTERFACE:include/opencdm>
        )

target_include_directories( ${TARGET}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/adapter>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

if (PROTOCOLS)
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

target_sources(${TARGET} PRIVATE adapter/${CDMI_ADAPTER_IMPLEMENTATION}/open_cdm_adapter.cpp)

find_package(GSTREAMER REQUIRED)
find_package(GSTREAMER_BASE REQUIRED)

target_link_libraries(${TARGET}
        PRIVATE
        ${GSTREAMER_LIBRARIES}
        ${GSTREAMER_BASE_LIBRARIES}
        CompileSettingsDebug::CompileSettingsDebug
        )

target_include_directories( ${TARGET}
        PRIVATE
        ${GSTREAMER_INCLUDES}
        ${GSTREAMER_BASE_INCLUDES}
        )

if (${CDMI_ADAPTER_IMPLEMENTATION} STREQUAL "broadcom-svp")
    find_package(NEXUS REQUIRED)
    target_link_libraries(${TARGET}
            PRIVATE
            NEXUS::NEXUS
            -lbrcmsvpmeta
            )
endif()

if (${CDMI_ADAPTER_IMPLEMENTATION} STREQUAL "broadcom-svp-secbuf")
    target_link_libraries(${TARGET}
            PRIVATE
            -lb_secbuf
            -lgstsvpext
            )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/ocdm COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/ocdm   # default include path
)

#FIXME: This needs to become a sysmlink.
install(
        FILES ${PUBLIC_HEADERS}
        DESTINATION include/opencdm
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "OCDM library")
