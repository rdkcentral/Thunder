set(TEST_RUNNER_NAME "WPEFramework_test_core")

add_executable(${TEST_RUNNER_NAME}
    ../IPTestAdministrator.cpp
    test_rpc.cpp
    test_cyclicbuffer.cpp
    test_doorbell.cpp
    test_ipc.cpp
    test_sharedbuffer.cpp
    test_timer.cpp
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkProtocols
)

