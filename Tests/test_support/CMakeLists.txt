# ============================================================================
# thunder_test_support - Static library for Thunder plugin integration testing
#
# This library embeds the Thunder PluginHost server (PluginServer, Controller,
# SystemInfo, etc.) into a static archive so that GTest-based test binaries
# can spin up a real Thunder runtime in-process without launching the
# standalone Thunder daemon.
#
# Usage:
#   1. Link your test executable against thunder_test_support when the test is
#      built in the same CMake build as Thunder.
#   2. Use ThunderTestRuntime::Initialize() to start the embedded server.
#   3. Call JSON-RPC or COM-RPC methods directly against loaded plugins.
#   4. Call ThunderTestRuntime::Deinitialize() when done.
#
# NOTE: PluginHost.cpp is deliberately excluded — it contains main().
#       The test binary provides its own main() via GTest.
# NOTE: The install rules below publish only the archive and public header.
#       They do not export a CMake package target for external find_package()
#       consumers.
# ============================================================================

find_package(Threads REQUIRED)

set(THUNDER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Thunder")

set(THREADPOOL_COUNT "4" CACHE STRING "The number of threads in the thread pool for test runtime")

add_library(thunder_test_support STATIC
    ThunderTestRuntime.cpp
    Module.cpp
    ${THUNDER_SOURCE_DIR}/PluginServer.cpp
    ${THUNDER_SOURCE_DIR}/Controller.cpp
    ${THUNDER_SOURCE_DIR}/SystemInfo.cpp
    ${THUNDER_SOURCE_DIR}/PostMortem.cpp
    ${THUNDER_SOURCE_DIR}/Probe.cpp
)

# PluginHost.cpp is deliberately excluded — it contains main().
# The test binary supplies its own main via GTest/GMock.

target_compile_definitions(thunder_test_support
    PRIVATE
        NAMESPACE=${NAMESPACE}
        APPLICATION_NAME=ThunderTestRuntime
        MODULE_NAME=ThunderTestRuntime
        THREADPOOL_COUNT=${THREADPOOL_COUNT}
)

target_compile_options(thunder_test_support PRIVATE -Wno-psabi)

set_target_properties(thunder_test_support PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(thunder_test_support
    PRIVATE
        CompileSettings::CompileSettings
)

if(EXCEPTION_CATCHING)
    set_source_files_properties(${THUNDER_SOURCE_DIR}/PluginServer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")
endif()

target_include_directories(thunder_test_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${THUNDER_SOURCE_DIR}
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/plugins/generated/jsonrpc>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/Thunder/generated>
)

target_link_libraries(thunder_test_support
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess
        Threads::Threads
)

if(WARNING_REPORTING)
    target_sources(thunder_test_support PRIVATE ${THUNDER_SOURCE_DIR}/WarningReportingCategories.cpp)
endif()

if(PROCESSCONTAINERS)
    target_link_libraries(thunder_test_support
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers)
    target_compile_definitions(thunder_test_support
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1)
endif()

if(HIBERNATESUPPORT)
    target_link_libraries(thunder_test_support PUBLIC
        ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate)
    target_compile_definitions(thunder_test_support PUBLIC
        HIBERNATE_SUPPORT_ENABLED=1)
endif()

# Enforce whole-archive semantics for this target so consumers automatically pull in
# Thunder's static initializers (MODULE_NAME_DECLARATION, SERVICE_REGISTRATION).
# Use linker-specific flags only on supported platforms/toolchains.
if(APPLE)
    target_link_options(thunder_test_support
        INTERFACE
            "-Wl,-force_load,$<TARGET_FILE:thunder_test_support>"
    )
elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android") AND
       (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    # Scoped to this archive only via $<TARGET_FILE:...> — won't affect other libs.
    target_link_options(thunder_test_support
        INTERFACE
            -Wl,--whole-archive $<TARGET_FILE:thunder_test_support> -Wl,--no-whole-archive
    )
else()
    message(WARNING
        "Automatic whole-archive linking for thunder_test_support is not configured for "
        "${CMAKE_SYSTEM_NAME} with ${CMAKE_CXX_COMPILER_ID}. Consumers may need to link "
        "the archive with platform-specific whole-archive/force-load options to ensure "
        "Thunder static initializers are included."
    )
endif()

# --- Install rules ---
install(TARGETS thunder_test_support
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/ThunderTestRuntime.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thunder_test_support
)

add_subdirectory(tests)
