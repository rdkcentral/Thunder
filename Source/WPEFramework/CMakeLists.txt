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

# ---------------------------------------------------------------------------
# Distributed Tracing (optional)
#
# Enables Thunder JSON-RPC and COM-RPC child span creation via librdk_otlp.
# librdk_otlp is resolved at runtime with dlsym(RTLD_DEFAULT) — WPEFramework
# does NOT hard-link it, so it starts cleanly even when the library is absent.
# ---------------------------------------------------------------------------
option(DISTRIBUTED_TRACING "Enable distributed tracing via librdk_otlp" OFF)

if(DISTRIBUTED_TRACING)
    target_sources(${TARGET} PRIVATE DistributedTracing.cpp)
    target_compile_definitions(${TARGET} PRIVATE THUNDER_DISTRIBUTED_TRACING)
    target_link_libraries(${TARGET} PRIVATE ${CMAKE_DL_LIBS})

    # Hard-link librdk_otlp into the WPEFramework binary.
    # rdk_otlp_init/shutdown are called once at startup/shutdown.
    # IUnknown.cpp (libWPEFrameworkCOM.so) does NOT link here — it uses
    # dlsym(RTLD_DEFAULT) instead, because that .so is also loaded by
    # WPEProcess which does not have librdk_otlp.
    find_library(RDK_OTLP_LIB rdk_otlp
                 PATHS ${CMAKE_SYSROOT}/usr/lib
                 NO_DEFAULT_PATH)
    if(RDK_OTLP_LIB)
        target_link_libraries(${TARGET} PRIVATE ${RDK_OTLP_LIB})
    else()
        message(FATAL_ERROR "DISTRIBUTED_TRACING enabled but librdk_otlp not found in sysroot. "
                            "Install librdk_otlp or set CMAKE_SYSROOT.")
    endif()

    find_path(RDK_OTLP_INCLUDE_DIR rdk_otlp_instrumentation.h
              PATHS ${CMAKE_SYSROOT}/usr/include
              NO_DEFAULT_PATH)
    if(RDK_OTLP_INCLUDE_DIR)
        target_include_directories(${TARGET} PRIVATE ${RDK_OTLP_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "DISTRIBUTED_TRACING enabled but rdk_otlp_instrumentation.h not found.")
    endif()

    message(STATUS "Distributed tracing ENABLED (hard-linked librdk_otlp in WPEFramework; "
                   "COM lib uses dlsym — safe for WPEProcess)")
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

