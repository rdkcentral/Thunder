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

set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(EGL REQUIRED)

add_library(${TARGET} SHARED Implementation.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        ${EGL_LIBRARIES}
)

if(PROTOCOLS)
    target_link_libraries(${TARGET}
            PUBLIC
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TARGET}
            PUBLIC
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
)

target_include_directories(${TARGET}
    PUBLIC
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        ${EGL_INCLUDE_DIRS})

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A compositor implementation wrapper for RPI and key handling.")

InstallCMakeConfig(TARGETS ${TARGET})
