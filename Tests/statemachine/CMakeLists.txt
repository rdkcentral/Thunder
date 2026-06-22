set(TARGET thunder_test_statemachine)

find_package(GTest REQUIRED)

add_executable(${TARGET}
    StateMachineTest.cpp
    BravePlugin.cpp
    FailInitPlugin.cpp
    ReentrantPlugin.cpp
    ObserverPlugin.cpp
    StateControlPlugin.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=StateMachineTest
        HOSTING_COMPROCESS=ThunderPlugin
        THREADPOOL_COUNT=4
)

if(BUILD_REFERENCE)
    target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

set(PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/data/plugins" CACHE PATH "Directory where the test plugins are located")



target_link_libraries(${TARGET}
    PRIVATE
        CompileSettings::CompileSettings
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        thunder_test_support
        GTest::gtest
        GTest::gtest_main
)

target_compile_definitions(${TARGET} PRIVATE
    TEST_PLUGIN_DIR="${PLUGIN_DIR}"
)

add_custom_command(TARGET ${TARGET} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DIR}"
)

add_subdirectory(provider)
target_compile_definitions(${TARGET} PRIVATE
    TEST_PLUGIN_LOCATOR="$<TARGET_FILE_NAME:ThunderTestStreamingProvider>"
)
add_dependencies(${TARGET} ThunderTestStreamingProvider)

add_subdirectory(statecontrol_oop)
target_compile_definitions(${TARGET} PRIVATE
    STATECTRL_OOP_LOCATOR="$<TARGET_FILE_NAME:ThunderTestStateControlOOP>")
add_dependencies(${TARGET} ThunderTestStateControlOOP)

add_test(NAME ${TARGET} COMMAND ${TARGET})
