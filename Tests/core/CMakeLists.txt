set(TEST_RUNNER_NAME "WPEFramework_test_core")

add_executable(${TEST_RUNNER_NAME}
    ../IPTestAdministrator.cpp
    test_cyclicbuffer.cpp
    test_databuffer.cpp
    test_doorbell.cpp
    test_ipc.cpp
    test_json.cpp
    test_measurement_type.cpp
    test_number_type.cpp
    test_rpc.cpp
    test_sharedbuffer.cpp
    test_socket_streamjson.cpp
    test_socket_streamtext.cpp
    test_thread.cpp
    test_timer.cpp
    test_weblink_json.cpp
    test_weblink_text.cpp
    test_websocket_json.cpp
    test_websocket_text.cpp
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkProtocols
)

