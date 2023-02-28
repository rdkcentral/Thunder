# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(TEST_RUNNER_NAME "WPEFramework_test_core")



add_executable(${TEST_RUNNER_NAME}
   ../IPTestAdministrator.cpp
   test_cyclicbuffer.cpp
   #test_databuffer.cpp
   test_dataelement.cpp
   test_dataelementfile.cpp
   #test_doorbell.cpp
   test_enumerate.cpp
   test_event.cpp
   test_hex2strserialization.cpp
   test_filesystem.cpp
   test_frametype.cpp
   #test_hash.cpp
   #test_ipc.cpp
   #test_ipcclient.cpp
   test_iso639.cpp
   test_iterator.cpp
   #test_jsonparser.cpp
   test_keyvalue.cpp
   test_library.cpp
   test_lockablecontainer.cpp
   test_measurementtype.cpp
   test_memberavailability.cpp
   #test_messageException.cpp
   #test_networkinfo.cpp
   test_nodeid.cpp
   test_numbertype.cpp
   test_optional.cpp
   test_parser.cpp
   test_portability.cpp
   test_processinfo.cpp
   test_queue.cpp
   test_rangetype.cpp
   test_readwritelock.cpp
   test_rectangle.cpp
   #test_rpc.cpp
   test_semaphore.cpp
   test_sharedbuffer.cpp
   test_singleton.cpp
   test_socketstreamjson.cpp
   test_socketstreamtext.cpp
   test_statetrigger.cpp
   test_stopwatch.cpp
   test_synchronize.cpp
   test_synchronous.cpp
   test_systeminfo.cpp
   test_textfragment.cpp
   test_textreader.cpp
   test_thread.cpp
   #test_threadpool.cpp
   test_time.cpp
   #test_timer.cpp
   test_tristate.cpp
   #test_valuerecorder.cpp
   test_weblinkjson.cpp
   test_weblinktext.cpp
   test_websocketjson.cpp
   test_websockettext.cpp
   #test_workerpool.cpp
   test_xgetopt.cpp
   #test_message_dispatcher.cpp
)
    
#[[ if(MESSAGING)    
   target_sources(${TEST_RUNNER_NAME} PRIVATE test_message_unit.cpp)
   target_link_libraries(${TEST_RUNNER_NAME}
      WPEFrameworkMessaging
   )
else()
   target_sources(${TEST_RUNNER_NAME} PRIVATE test_tracing.cpp)
   target_link_libraries(${TEST_RUNNER_NAME}
      WPEFrameworkTracing
   )
endif() ]]

set_source_files_properties(test_systeminfo.cpp PROPERTIES COMPILE_OPTIONS "-fexceptions")

target_compile_definitions(${TEST_RUNNER_NAME}
   PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_compile_definitions(${TEST_RUNNER_NAME}
   PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_link_libraries(${TEST_RUNNER_NAME} 
    ${GTEST_LIBRARY}
    ${GTEST_MAIN_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    WPEFrameworkCore
    WPEFrameworkCOM
    WPEFrameworkWebSocket
)

install(
    TARGETS ${TEST_RUNNER_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

add_test(NAME ${TEST_RUNNER_NAME} COMMAND ${TEST_RUNNER_NAME})
