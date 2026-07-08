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

find_package(Threads REQUIRED)

get_filename_component(TARGET ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(THREADPOOL_COUNT "4" CACHE STRING "The number of threads in the thread pool")
set(ENABLE_INITSCRIPT_SUPPORT ON CACHE STRING "Enable init script(systemd/initd) install support from the thunder source tree")

# --- Post-mortem crash diagnostics sinks ---
# Helper macro to map a sink string to its integer enum value
macro(thunder_postmortem_sink_value VAR_IN VAR_OUT)
    string(TOUPPER "${${VAR_IN}}" _s)
    if (_s STREQUAL "LOG")
        set(${VAR_OUT} 1)
    elseif(_s STREQUAL "ALL")
        set(${VAR_OUT} 2)
    elseif(_s STREQUAL "DISABLED")
        set(${VAR_OUT} 3)
    else()
        set(${VAR_OUT} 0)
    endif()
endmacro()

set(POSTMORTEM_WORKERPOOL_SINK "FILE" CACHE STRING
    "Default WorkerPool JSON dump sink on crash (FILE | LOG | ALL | DISABLED)")
set(POSTMORTEM_CALLSTACK_SINK "DISABLED" CACHE STRING
    "Default inline callstack print sink on crash (FILE | LOG | ALL | DISABLED)")
set_property(CACHE POSTMORTEM_WORKERPOOL_SINK PROPERTY STRINGS FILE LOG ALL DISABLED)
set_property(CACHE POSTMORTEM_CALLSTACK_SINK  PROPERTY STRINGS FILE LOG ALL DISABLED)

thunder_postmortem_sink_value(POSTMORTEM_WORKERPOOL_SINK _wp_sink)
thunder_postmortem_sink_value(POSTMORTEM_CALLSTACK_SINK  _cs_sink)

add_executable(${TARGET}
        Controller.cpp
        PluginHost.cpp
        PluginServer.cpp
        Probe.cpp
        SystemInfo.cpp
        PostMortem.cpp
        )
separate_arguments(BREAKPAD_LDFLAGS)
target_link_libraries(${TARGET} PRIVATE "${BREAKPAD_LDFLAGS}")
target_compile_definitions(${TARGET}
        PRIVATE
          NAMESPACE=${NAMESPACE}
          APPLICATION_NAME=${TARGET}
          THREADPOOL_COUNT=${THREADPOOL_COUNT}
          THUNDER_POSTMORTEM_WORKERPOOL_SINK_DEFAULT=${_wp_sink}
          THUNDER_POSTMORTEM_CALLSTACK_SINK_DEFAULT=${_cs_sink}
        )

target_compile_options (${TARGET} PRIVATE -Wno-psabi)

if (TREE_REFERENCE)
    target_compile_definitions(${TARGET}
            PRIVATE
              -DTREE_REFERENCE=${TREE_REFERENCE}
            )
endif()

target_include_directories( ${TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../plugins/generated/jsonrpc>
        )


if (EXCEPTION_CATCHING)
    set_source_files_properties( PluginServer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions" )
endif()

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}COM::${NAMESPACE}COM
          ${NAMESPACE}Messaging::${NAMESPACE}Messaging
          ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
          ${NAMESPACE}Plugins::${NAMESPACE}Plugins
          ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess  # For COM executable define
          Threads::Threads
        )

if(WARNING_REPORTING)
    target_sources(${TARGET} PRIVATE WarningReportingCategories.cpp)
endif()

if (PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif(PROCESSCONTAINERS)

if(HIBERNATESUPPORT)
    option(HIBERNATESUPPORT_AUTOWAKEUP "Auto-wakeup for Hibernate" OFF)
    target_link_libraries(${TARGET} PRIVATE
        ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate)
    target_compile_definitions(${TARGET} PUBLIC
         HIBERNATE_SUPPORT_ENABLED=1)
endif()

find_package(Systemd)
if (SYSTEMD_FOUND)
    target_link_libraries( ${TARGET}
            PRIVATE
            Systemd::Systemd
            )
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DSYSTEMD_FOUND")
endif()

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

install(DIRECTORY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../${PERSISTENT_PATH/${NAMESPACE}}
    DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE
)

if (ENABLE_INITSCRIPT_SUPPORT)
    add_subdirectory(scripts)
endif()
include(GenericConfig.cmake)

