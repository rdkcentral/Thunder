set(TEST_RUNNER_NAME "WPEFramework_test_core")

add_executable(${TEST_RUNNER_NAME}
    ../IPTestAdministrator.cpp
    test_cyclicbuffer.cpp
    test_databuffer.cpp
    test_doorbell.cpp
    test_hex2strserialization.cpp
    test_ipc.cpp
    test_measurementtype.cpp
    test_numbertype.cpp
    test_rpc.cpp
    test_jsonparser.cpp
    test_sharedbuffer.cpp
    test_socketstreamjson.cpp
    test_socketstreamtext.cpp
    test_thread.cpp
    test_timer.cpp
    test_weblinkjson.cpp
    test_weblinktext.cpp
    test_websocketjson.cpp
    test_websockettext.cpp
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkProtocols
)

