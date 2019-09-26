set(TEST_RUNNER_NAME "WPEFramework_test_core")

add_executable(${TEST_RUNNER_NAME}
   ../IPTestAdministrator.cpp
   test_rpc.cpp
   test_jsonparser.cpp
   test_hex2strserialization.cpp
   test_sharedbuffer.cpp
   test_time.cpp
   test_tracing.cpp
   test_logging.cpp
   test_textfragment.cpp
   test_statetrigger.cpp
   test_queue.cpp
   test_event.cpp
   test_singleton.cpp
   test_semahore.cpp
   test_XGetopt.cpp
   test_dataElement.cpp
   test_portability.cpp
   test_frametype.cpp
   test_dataelementfile.cpp
   test_ISO639.cpp
#  test_sharedbuffer.cpp
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkProtocols
)

