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

set(THREADPOOL_COUNT "4" CACHE STRING "The number of threads in the thread pool")
set(ENABLE_INITSCRIPT_SUPPORT ON CACHE STRING "Enable init script(systemd/initd) install support from the thunder source tree")

# JsonGenerator(CODE INPUT ${CMAKE_CURRENT_SOURCE_DIR}/json/Controller.json OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/json")

add_executable(${TARGET}
        Controller.cpp
        ControllerJsonRpc.cpp
        PluginHost.cpp
        PluginServer.cpp
        Probe.cpp
        SystemInfo.cpp
        PostMortem.cpp
        )

target_compile_definitions(${TARGET}
        PRIVATE
          NAMESPACE=${NAMESPACE}
          APPLICATION_NAME=${TARGET}
          THREADPOOL_COUNT=${THREADPOOL_COUNT}
        )

if (TREE_REFERENCE)
    target_compile_definitions(${TARGET}
            PRIVATE
              -DTREE_REFERENCE=${TREE_REFERENCE}
            )
endif()

if (EXCEPTION_CATCHING)
    set_source_files_properties( PluginServer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions" )
endif()

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${CMAKE_DL_LIBS}
          ${LIBEXECINFO_LIBRARY}
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}Plugins::${NAMESPACE}Plugins
          ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess  # For COM executable define
        )
        
if (PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif(PROCESSCONTAINERS)

if(HIBERNATESUPPORT)
    target_link_libraries(${TARGET} PRIVATE
        ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate)
    target_compile_definitions(${TARGET} PUBLIC
         HIBERNATE_SUPPORT_ENABLED=1)
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

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

if (ENABLE_INITSCRIPT_SUPPORT)
    add_subdirectory(scripts)
endif()
include(GenericConfig.cmake)

