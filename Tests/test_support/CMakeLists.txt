# ==========================================================================
# thunder_test_support — static library for in-process Thunder plugin testing
#
# Compiles the test runtime together with the subset of Source/Thunder
# objects needed to host an embedded PluginHost::Server.  The resulting
# archive is linked via whole-archive so that MODULE_NAME_DECLARATION
# statics are not dropped by the linker.
# ==========================================================================

find_package(Threads REQUIRED)

set(THUNDER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Source/Thunder")

set(THREADPOOL_COUNT "4" CACHE STRING "The number of threads in the thread pool for test runtime")

set(TARGET thunder_test_support)

add_library(${TARGET} STATIC
    ThunderTestRuntime.cpp
    Module.cpp
    ${THUNDER_SOURCE_DIR}/PluginServer.cpp
    ${THUNDER_SOURCE_DIR}/Controller.cpp
    ${THUNDER_SOURCE_DIR}/SystemInfo.cpp
    ${THUNDER_SOURCE_DIR}/PostMortem.cpp
    ${THUNDER_SOURCE_DIR}/Probe.cpp
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
    PRIVATE
        ${THUNDER_SOURCE_DIR}
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/plugins/generated/jsonrpc>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Source/Thunder/generated>
)

target_compile_definitions(${TARGET}
    PRIVATE
        NAMESPACE=${NAMESPACE}
        APPLICATION_NAME=ThunderTestRuntime
        MODULE_NAME=ThunderTestRuntime
        THREADPOOL_COUNT=${THREADPOOL_COUNT}
        DEFAULT_SYSTEM_PATH="${SYSTEM_PATH}"
        DEFAULT_PROXYSTUB_PATH="${PROXYSTUB_PATH}"
)

target_compile_options(${TARGET} PRIVATE -Wno-psabi)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettings::CompileSettings
        CompileSettingsDebug::CompileSettingsDebug
)

if(EXCEPTION_CATCHING)
    set_source_files_properties(${THUNDER_SOURCE_DIR}/PluginServer.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")
endif()

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        Threads::Threads
)

# COMProcess is an INTERFACE library carrying only a compile definition.
# It is not in any export set, so we absorb its effect directly.
target_compile_definitions(${TARGET} PRIVATE HOSTING_COMPROCESS=ThunderPlugin)

# ------------------------------------------------------------------
# Whole-archive link.  Ensures that the MODULE_NAME_DECLARATION
# constructors from the Server objects are not discarded.
# BUILD_INTERFACE only — installed consumers get this via the
# custom Config.cmake.in template instead.
# ------------------------------------------------------------------
if(APPLE)
    target_link_options(${TARGET} INTERFACE
        "$<BUILD_INTERFACE:SHELL:-Wl,-force_load,$<TARGET_FILE:${TARGET}>>"
    )
else()
    target_link_options(${TARGET} INTERFACE
        "$<BUILD_INTERFACE:SHELL:-Wl,--whole-archive $<TARGET_FILE:${TARGET}> -Wl,--no-whole-archive>"
    )
endif()

# ------------------------------------------------------------------
# Optional features – only link when enabled in the main build.
# ------------------------------------------------------------------
if(WARNING_REPORTING)
    target_sources(${TARGET}
        PRIVATE
            ${THUNDER_SOURCE_DIR}/WarningReportingCategories.cpp
    )
endif()

if(PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers
    )
    target_compile_definitions(${TARGET}
        PUBLIC
            PROCESSCONTAINERS_ENABLED=1
    )
endif()

if(HIBERNATESUPPORT)
    target_link_libraries(${TARGET}
        PUBLIC
            ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate
    )
    target_compile_definitions(${TARGET}
        PUBLIC
            HIBERNATE_SUPPORT_ENABLED=1
    )
endif()

# ------------------------------------------------------------------
# Install the library and header so external projects can use it
# via find_package(thunder_test_support)
# ------------------------------------------------------------------
install(
    FILES ThunderTestRuntime.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/test_support
    COMPONENT ${NAMESPACE}_Development
)

install(
    TARGETS ${TARGET} EXPORT ${TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

# ------------------------------------------------------------------
# thunder_test_main — GTest main() that calls _Exit() after
# RUN_ALL_TESTS() to avoid static destruction order crashes when
# external plugin .so files are loaded.
#
# Plugin tests link this instead of GTest::Main.
# ------------------------------------------------------------------
set(MAIN_TARGET thunder_test_main)

add_library(${MAIN_TARGET} STATIC
    ThunderTestMain.cpp
)

target_link_libraries(${MAIN_TARGET}
    PUBLIC
        GTest::GTest
)

set_target_properties(${MAIN_TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

install(
    TARGETS ${MAIN_TARGET} EXPORT ${MAIN_TARGET}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
)

InstallCMakeConfig(
    TARGETS ${TARGET}
    TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/thunder_test_supportConfig.cmake.in
)

InstallCMakeConfig(
    TARGETS ${MAIN_TARGET}
)

# ------------------------------------------------------------------
# Smoke test sub-directory
# ------------------------------------------------------------------
add_subdirectory(tests)
