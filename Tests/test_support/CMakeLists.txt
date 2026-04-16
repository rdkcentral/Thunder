# ==========================================================================
# thunder_test_support — static library for in-process Thunder plugin testing
#
# Compiles the test runtime together with the subset of Source/Thunder
# objects needed to host an embedded PluginHost::Server.  The resulting
# archive is linked via whole-archive so that MODULE_NAME_DECLARATION
# statics are not dropped by the linker.
# ==========================================================================

set(TARGET thunder_test_support)

add_library(${TARGET} STATIC
    ThunderTestRuntime.cpp
    Module.cpp
    ${CMAKE_SOURCE_DIR}/Source/Thunder/PluginServer.cpp
    ${CMAKE_SOURCE_DIR}/Source/Thunder/Controller.cpp
    ${CMAKE_SOURCE_DIR}/Source/Thunder/SystemInfo.cpp
    ${CMAKE_SOURCE_DIR}/Source/Thunder/PostMortem.cpp
    ${CMAKE_SOURCE_DIR}/Source/Thunder/Probe.cpp
)

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source/Thunder
)

target_compile_definitions(${TARGET}
    PUBLIC
        NAMESPACE=${NAMESPACE}
        APPLICATION_NAME=ThunderTestRuntime
        MODULE_NAME=ThunderTestRuntime
        THREADPOOL_COUNT=${THREADPOOL_COUNT}
    PRIVATE
        DEFAULT_SYSTEM_PATH="${SYSTEM_PATH}"
        DEFAULT_PROXYSTUB_PATH="${PROXYSTUB_PATH}"
)

target_link_libraries(${TARGET}
    PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        CompileSettingsDebug::CompileSettingsDebug
        COMProcess::COMProcess
        Threads::Threads
)

# ------------------------------------------------------------------
# Whole-archive link.  Ensures that the MODULE_NAME_DECLARATION
# constructors from the Server objects are not discarded.
# ------------------------------------------------------------------
if(APPLE)
    target_link_options(${TARGET} INTERFACE
        "SHELL:-Wl,-force_load,$<TARGET_FILE:${TARGET}>"
    )
else()
    target_link_options(${TARGET} INTERFACE
        "SHELL:-Wl,--whole-archive $<TARGET_FILE:${TARGET}> -Wl,--no-whole-archive"
    )
endif()

# ------------------------------------------------------------------
# Optional features – only link when enabled in the main build.
# ------------------------------------------------------------------
if(WARNING_REPORTING)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
    )
    target_sources(${TARGET}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/Source/Thunder/WarningReportingCategories.cpp
    )
endif()

if(PROCESSCONTAINERS)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}ProcessContainers::${NAMESPACE}ProcessContainers
    )
endif()

if(HIBERNATESUPPORT)
    target_link_libraries(${TARGET}
        PRIVATE
            ${NAMESPACE}Hibernate::${NAMESPACE}Hibernate
    )
endif()

# ------------------------------------------------------------------
# Smoke test sub-directory
# ------------------------------------------------------------------
add_subdirectory(tests)
