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

set(TargetMarshalling ${NAMESPACE}Marshallings)
set(TargetDefinitions ${NAMESPACE}Definitions)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

file(GLOB PUBLIC_HEADERS I*.h)
file(GLOB JSON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/json/*.json")

list(APPEND PUBLIC_HEADERS Module.h)
list(APPEND PUBLIC_HEADERS definitions.h)

ProxyStubGenerator(INPUT "${CMAKE_CURRENT_SOURCE_DIR}" OUTDIR "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs")
JsonGenerator(CODE INPUT ${JSON_FILE} OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/json")
JsonGenerator(CODE INPUT "${CMAKE_CURRENT_SOURCE_DIR}/I*.h" OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/json")

file(GLOB PROXY_STUB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/proxystubs/ProxyStubs*.cpp")
add_library(${TargetMarshalling} SHARED 
        Module.cpp
        ${PROXY_STUB_SOURCES}
        )

target_include_directories(${TargetMarshalling} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

file(GLOB JSON_ENUM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/generated/json/JsonEnum*.cpp")
file(GLOB JSON_LINK_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/generated/json/J*.h")

add_library(${TargetDefinitions} SHARED
        Definitions.cpp
        ${JSON_ENUM_SOURCES}
        )

target_link_libraries(${TargetMarshalling}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          CompileSettingsDebug::CompileSettingsDebug
        )

target_compile_definitions(${TargetMarshalling}
        PUBLIC
            PROXYSTUB_BUILDING=1)

target_link_libraries(${TargetDefinitions}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

if (PROTOCOLS)
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

set_target_properties(${TargetMarshalling} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

set_target_properties(${TargetDefinitions} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        )

target_include_directories(${TargetDefinitions}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
          $<INSTALL_INTERFACE:include/${NAMESPACE}/interfaces>
        )

install(
        TARGETS ${TargetDefinitions} EXPORT ${TargetDefinitions}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/ COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/ COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/ COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/interfaces COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

install(
        TARGETS ${TargetMarshalling} EXPORT ${TargetMarshalling}Targets  # for downstream dependencies
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
)

install(
        FILES ${JSON_LINK_HEADERS}
        DESTINATION include/${NAMESPACE}/interfaces/json
)

InstallPackageConfig(
        TARGETS ${TargetDefinitions}
        DESCRIPTION "Share lookup tables associated with interface enums."
        )

InstallCMakeConfig(TARGETS ${TargetDefinitions})
