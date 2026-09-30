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

cmake_minimum_required(VERSION 3.15)

set(TARGET ${NAMESPACE}Messaging)

if(MESSAGING_RDKLOGGER_USE_MOCK)
    target_sources(${TARGET} PRIVATE RDKLoggerOutputMock.cpp)
else()
    find_path(RDKLOGGER_INCLUDE_DIR rdk_logger.h)
    find_library(RDKLOGGER_LIBRARY NAMES rdkloggers)

    if(NOT RDKLOGGER_INCLUDE_DIR OR NOT RDKLOGGER_LIBRARY)
        message(FATAL_ERROR "RDKLogger support requires rdk_logger.h and librdkloggers; set MESSAGING_RDKLOGGER_USE_MOCK=ON for SDK-independent builds")
    endif()

    target_sources(${TARGET} PRIVATE RDKLoggerOutput.cpp)
    target_include_directories(${TARGET} PRIVATE ${RDKLOGGER_INCLUDE_DIR})
    target_link_libraries(${TARGET} PRIVATE ${RDKLOGGER_LIBRARY})
endif()