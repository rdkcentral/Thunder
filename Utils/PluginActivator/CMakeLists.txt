
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

cmake_minimum_required(VERSION 3.10.3)

project( PluginActivator )

set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

add_executable(${PROJECT_NAME}
    source/Module.cpp
    source/main.cpp
    source/COMRPCStarter.cpp
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
    -Wall
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}COM::${NAMESPACE}COM
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
