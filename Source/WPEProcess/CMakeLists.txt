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

get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_executable(${TARGET}
        Process.cpp
        )

# Non-buildable interface target to carry the definitions for users
add_library(COMProcess INTERFACE)
add_library(${NAMESPACE}COMProcess::${NAMESPACE}COMProcess ALIAS COMProcess)
target_compile_definitions(COMProcess
        INTERFACE
          HOSTING_COMPROCESS=${TARGET}
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Core::${NAMESPACE}Core
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

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

install(
        TARGETS COMProcess ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)
