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

find_package(Breakpad QUIET)

if(EXCEPTION_CATCHING)
    set_source_files_properties( Process.cpp PROPERTIES COMPILE_FLAGS "-fexceptions" )
endif()

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
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        )
        
if (WARNING_REPORTING)
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
            )
endif()

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

if(BREAKPAD_FOUND)
    add_definitions(-DUSE_BREAKPAD=1 -D__STDC_FORMAT_MACROS)
    target_link_libraries(${TARGET} PUBLIC ${BREAKPAD_LIBRARIES} pthread)
    target_include_directories(${TARGET} PUBLIC ${BREAKPAD_INCLUDE_DIRS})
endif(BREAKPAD_FOUND)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        )

install(
        TARGETS COMProcess ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)
