set(TEST_RUNNER_NAME "WPEFramework_test_core")

add_executable(${TEST_RUNNER_NAME}
   ../IPTestAdministrator.cpp
   test_cyclicbuffer.cpp
   test_databuffer.cpp
   test_dataelementfile.cpp
   test_dataelement.cpp
   test_doorbell.cpp
   test_event.cpp
   test_hex2strserialization.cpp
   test_enumerate.cpp
   test_filesystem.cpp
   test_frametype.cpp
   test_ipc.cpp
   test_iso639.cpp
   test_iterator.cpp
   test_jsonparser.cpp
   test_library.cpp
   test_logging.cpp
   test_measurementtype.cpp
   test_messageException.cpp
   test_numbertype.cpp
   test_optional.cpp
   test_parser.cpp
   test_portability.cpp
   test_queue.cpp
   test_rangetype.cpp
   test_readwritelock.cpp
   test_rectangle.cpp
   test_rpc.cpp
   test_semaphore.cpp
   test_sharedbuffer.cpp
   test_singleton.cpp
   test_socketstreamjson.cpp
   test_socketstreamtext.cpp
   test_statetrigger.cpp
   test_stopwatch.cpp
   test_synchronize.cpp
   test_textfragment.cpp
   test_textreader.cpp
   test_thread.cpp
   test_time.cpp
   test_timer.cpp
   test_tracing.cpp
   test_tristate.cpp
   test_weblinkjson.cpp
   test_weblinktext.cpp
   test_websocketjson.cpp
   test_websockettext.cpp
   test_xgetopt.cpp
)

target_compile_definitions(${TEST_RUNNER_NAME}
   PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkProtocols
)

