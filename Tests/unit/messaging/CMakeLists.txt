set(TEST_RUNNER_NAME "WPEFramework_test_messaging_external_output")

add_executable(${TEST_RUNNER_NAME}
    test_external_output.cpp)

target_link_libraries(${TEST_RUNNER_NAME}
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkMessaging
    WPEFrameworkMessagingExternalOutput)

install(
    TARGETS ${TEST_RUNNER_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

add_test(NAME ${TEST_RUNNER_NAME} COMMAND ${TEST_RUNNER_NAME})