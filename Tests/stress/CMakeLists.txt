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

set(PROXY_STRESS_TEST_RUNNER "WPEFramework_proxypool_stress_test")

add_executable(${PROXY_STRESS_TEST_RUNNER}
   proxy_pool_stress_test.cpp
)

target_compile_definitions(${PROXY_STRESS_TEST_RUNNER}
   PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_compile_definitions(${PROXY_STRESS_TEST_RUNNER}
   PRIVATE BUILD_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\"
)

target_link_libraries(${PROXY_STRESS_TEST_RUNNER} 
    WPEFrameworkCore
    WPEFrameworkTracing
    WPEFrameworkCOM
    WPEFrameworkWebSocket
)

install(
    TARGETS ${PROXY_STRESS_TEST_RUNNER}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
