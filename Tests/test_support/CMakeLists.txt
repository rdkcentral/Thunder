# ==========================================================================
# thunder_test_support — static library for in-process Thunder plugin testing
#
# Compiles the test runtime together with the subset of Source/Thunder
# objects needed to host an embedded PluginHost::Server. The resulting
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
        ${NAMESPACE}Common::${NAMESPACE}Common
        Threads::Threads
)

target_compile_definitions(${TARGET} PRIVATE HOSTING_COMPROCESS=ThunderPlugin)

if(WIN32)
    message(FATAL_ERROR "thunder_test_support requires POSIX and is not supported on Windows.")
elseif(APPLE)
    target_link_options(${TARGET} INTERFACE
        "$<BUILD_INTERFACE:SHELL:-Wl,-force_load,$<TARGET_FILE:${TARGET}>>"
    )
else()
    target_link_options(${TARGET} INTERFACE
        "$<BUILD_INTERFACE:SHELL:-Wl,--whole-archive $<TARGET_FILE:${TARGET}> -Wl,--no-whole-archive>"
    )
endif()

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

InstallCMakeConfig(
    TARGETS ${TARGET}
    TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/thunder_test_supportConfig.cmake.in
)

add_subdirectory(tests)