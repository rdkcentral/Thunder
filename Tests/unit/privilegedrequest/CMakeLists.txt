# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
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

set(TEST_RUNNER_NAME "Thunder_test_privilegedrequest")

if(LINUX)
    add_executable(${TEST_RUNNER_NAME}
        test_privilegedrequest.cpp
    )

    target_compile_definitions(${TEST_RUNNER_NAME}
        PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
    )

    if (APPLE)
        set(GTEST_LIBRARY GTest::gtest GTest::gtest_main)
    else()
        set(GTEST_LIBRARY "${GTEST_BOTH_LIBRARIES}")
    endif()

    target_link_libraries(${TEST_RUNNER_NAME}
        ${GTEST_LIBRARY}
        GTest::gmock
        ${CMAKE_THREAD_LIBS_INIT}
        ${NAMESPACE}Core::${NAMESPACE}Core
    )

    install(
        TARGETS ${TEST_RUNNER_NAME}
        DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test
    )

    add_test(NAME ${TEST_RUNNER_NAME} COMMAND ${TEST_RUNNER_NAME})
else()
    message(STATUS "PrivilegedRequest tests are only supported on Linux platform")
endif()
