set(TARGET thunder_test_runtime_smoke)

find_package(GTest REQUIRED)

add_executable(${TARGET}
    SmokeTest.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=SmokeTest
        BUILD_REFERENCE=${BUILD_REFERENCE}
)

target_link_libraries(${TARGET}
    PRIVATE
        thunder_test_support
        GTest::GTest
        GTest::Main
)

add_test(NAME ${TARGET} COMMAND ${TARGET})

# ------------------------------------------------------------------
# TestPlugin integration test
#
# Requires:
#   - ThunderInterfaces qa_interfaces headers installed (for ITestPlugin.h)
#   - ThunderNanoServices TestPlugin .so built and path passed via
#     TEST_PLUGIN_PATH (defaults to the Thunder install plugins dir)
# ------------------------------------------------------------------
set(TEST_PLUGIN_PATH "" CACHE PATH "Directory containing libThunderTestPlugin.so")

if(TEST_PLUGIN_PATH)
    set(PLUGIN_TEST_TARGET thunder_test_runtime_plugin)

    add_executable(${PLUGIN_TEST_TARGET}
        TestPluginTest.cpp
        Module.cpp
    )

    target_compile_definitions(${PLUGIN_TEST_TARGET}
        PRIVATE
            MODULE_NAME=TestPluginTest
            BUILD_REFERENCE=${BUILD_REFERENCE}
            TEST_PLUGIN_PATH="${TEST_PLUGIN_PATH}"
    )

    target_include_directories(${PLUGIN_TEST_TARGET}
        PRIVATE
            ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
    )

    target_link_libraries(${PLUGIN_TEST_TARGET}
        PRIVATE
            thunder_test_support
            GTest::GTest
            GTest::Main
    )

    add_test(NAME ${PLUGIN_TEST_TARGET} COMMAND ${PLUGIN_TEST_TARGET})
endif()
