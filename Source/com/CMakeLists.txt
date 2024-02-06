#If not stated otherwise in this file or this component's license file the
#following copyright and licenses apply:
#
#Copyright 2020 Metrological
#
#Licensed under the Apache License, Version 2.0(the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#http: // www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.

set(TARGET ${NAMESPACE}COM)

ProxyStubGenerator(NAMESPACE "Thunder::RPC" INPUT "${CMAKE_CURRENT_SOURCE_DIR}/ICOM.h" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

add_library(${TARGET}
    Module.cpp
    Administrator.cpp
    Communicator.cpp
    IUnknown.cpp
    ConnectorType.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/generated/ProxyStubs_COM.cpp
)

set(PUBLIC_HEADERS
    Administrator.h
    com.h
    Communicator.h
    ConnectorType.h
    Ids.h
    IteratorType.h
    IIteratorType.h
    IStringIterator.h
    IValueIterator.h
    ICOM.h
    IUnknown.h
    Messages.h
    ProxyStubs.h
    Module.h
)

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Messaging::${NAMESPACE}Messaging
          CompileSettingsDebug::CompileSettingsDebug
        )

if (WARNING_REPORTING)
target_compile_definitions(${TARGET}
PUBLIC
        WARNING_REPORTING_ENABLED
)

target_link_libraries(${TARGET}
PUBLIC
        ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
)
endif()

target_include_directories(${TARGET}
        PRIVATE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )

target_compile_definitions(${TARGET} PRIVATE COM_EXPORTS)

if(PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif(PROCESSCONTAINERS)

set(CPP_STD 11)
if(APPLE)
    set(CPP_STD 17)
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD ${CPP_STD}
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/com COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

InstallCMakeConfig(TARGETS ${TARGET})
