set(TEST_RUNNER_NAME "WPEFramework_test_tests")

add_executable(${TEST_RUNNER_NAME}
   test_iptestmanager.cpp
   ../IPTestAdministrator.cpp
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    Core
    Tracing
    Protocols
)
