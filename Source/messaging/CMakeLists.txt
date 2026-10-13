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

set(TARGET ${NAMESPACE}Messaging)

add_library(${TARGET} 
        Module.cpp
        MessageClient.cpp
        MessageUnit.cpp
        TraceCategories.cpp
        Logging.cpp
        DirectOutput.cpp
        ConsoleStreamRedirect.cpp
        OperationalCategories.cpp
        AssertionUnit.cpp)

option(MESSAGING_EXTERNAL_OUTPUT "Enable External Direct output support." OFF)
option(MESSAGING_EXTERNAL_OUTPUT_USE_MOCK "Use the console-printing external output backend." OFF)
set(MESSAGING_EXTERNAL_OUTPUT_REPOSITORY "https://github.com/WebPlatformForEmbedded/ThunderNanoServicesRDK.git" CACHE STRING "Repository containing the external messaging adapter")
set(MESSAGING_EXTERNAL_OUTPUT_VERSION "master" CACHE STRING "Version of the external messaging adapter repository")

set(THUNDER_MESSAGING_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(MESSAGING_EXTERNAL_OUTPUT)
        set(MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR "" CACHE PATH "Source directory for the optional external messaging adapter")

        if(NOT MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR)
                include(GetExternalCode)

                set(_MESSAGING_EXTERNAL_REPOSITORY_SOURCE_DIR "${CMAKE_BINARY_DIR}/ThunderExternalOutputRepository")
                GetExternalCode(
                        GIT_REPOSITORY "${MESSAGING_EXTERNAL_OUTPUT_REPOSITORY}"
                        GIT_VERSION "${MESSAGING_EXTERNAL_OUTPUT_VERSION}"
                        SOURCE_DIR "${_MESSAGING_EXTERNAL_REPOSITORY_SOURCE_DIR}")

                file(GLOB _MESSAGING_EXTERNAL_OUTPUT_CANDIDATES
                        "${_MESSAGING_EXTERNAL_REPOSITORY_SOURCE_DIR}/MessageControl/*/CMakeLists.txt")

                foreach(_candidate IN LISTS _MESSAGING_EXTERNAL_OUTPUT_CANDIDATES)
                        file(READ "${_candidate}" _candidate_contents)
                        if(_candidate_contents MATCHES "MessagingExternalOutput")
                                get_filename_component(MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR
                                        "${_candidate}" DIRECTORY)
                                break()
                        endif()
                endforeach()
        endif()

        if(NOT EXISTS "${MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR}/CMakeLists.txt")
                message(FATAL_ERROR "Could not locate the external messaging adapter sources; set MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR explicitly")
        endif()

        add_subdirectory("${MESSAGING_EXTERNAL_OUTPUT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/ExternalOutput")
        target_compile_definitions(${TARGET} PRIVATE MESSAGING_EXTERNAL_OUTPUT=1)
        target_link_libraries(${TARGET} PRIVATE ${NAMESPACE}MessagingExternalOutput)
endif()

set(PUBLIC_HEADERS
        Module.h
        MessageClient.h
        messaging.h
        MessageDispatcher.h
        MessageUnit.h
        TraceCategories.h
        TraceControl.h
        TelemetryControl.h
        Control.h
        Logging.h
        LoggingCategories.h
        DirectOutput.h
        Module.h
        TraceFactory.h
        ConsoleStreamRedirect.h
        OperationalCategories.h
        AssertionUnit.h)

target_compile_definitions(${TARGET} PRIVATE MESSAGING_EXPORTS)

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          CompileSettingsDebug::CompileSettingsDebug
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD ${CXX_STD}
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

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/messaging COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

InstallCMakeConfig(TARGETS ${TARGET})

option(LEGACY_TRACING_SUPPORT "Enable backward compatibilaty support for the Tracing" ON)

if(LEGACY_TRACING_SUPPORT)
include(CreateLink)

InstallCompatibleCMakeConfig( 
        TARGET ${TARGET}
        LEGACY_TARGET ${NAMESPACE}Tracing
        LEGACY_INCLUDE_DIR  "${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}"
        LEGACY_PUBLIC_HEADER_LOCATION "${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/tracing") 

createlink(
        LINK ${CMAKE_CURRENT_BINARY_DIR}/tracing.h
        TARGET messaging.h
       )
install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/tracing.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/tracing/
       )
endif()
