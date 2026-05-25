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

# =============================================================================
# JSON Library Evaluation Suite
#
# Compares Thunder's built-in JSON implementation against:
#   - nlohmann/json  (https://github.com/nlohmann/json)
#   - RapidJSON      (https://github.com/Tencent/rapidjson)
#
# Evaluation dimensions:
#   - Parse performance  (ns/op at small / medium / large / deep payloads)
#   - Serialize performance
#   - Memory allocation behaviour (count + bytes per operation)
#   - Correctness / RFC 8259 compliance
#   - API usability (code patterns, error reporting)
#   - Integration complexity (documented here via FetchContent)
#
# Build with:
#   cmake -DJSON_BENCHMARK=ON ...
# Run with:
#   ctest -R JsonBenchmark -V
#   ./Thunder_json_benchmark --gtest_also_run_disabled_tests
# =============================================================================

cmake_minimum_required(VERSION 3.15)

find_package(GTest REQUIRED)

# ---------------------------------------------------------------------------
# Integration complexity metric #1 — nlohmann/json
# Header-only, single include, Apache 2.0 license.
# GitHub: ~45k stars, actively maintained, C++11 minimum.
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Integration complexity metric #2 — RapidJSON
# Header-only, MIT license, no CMake install needed.
# GitHub: ~14k stars, C++11 minimum, very fast SAX/DOM parser.
# ---------------------------------------------------------------------------
FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        v1.1.0
    GIT_SHALLOW    TRUE
)
set(RAPIDJSON_BUILD_DOC          OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_THIRDPARTY_GTEST OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_HAS_STDSTRING      ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidjson)

# ---------------------------------------------------------------------------
# Benchmark executable
# ---------------------------------------------------------------------------
set(TARGET_NAME Thunder_json_benchmark)

add_executable(${TARGET_NAME}
    MemoryHooks.cpp
    bench_parse.cpp
    bench_serialize.cpp
    bench_memory.cpp
    bench_correctness.cpp
    bench_api_usability.cpp
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${rapidjson_SOURCE_DIR}/include
)

# nlohmann is header-only; its CMake target sets the include dirs automatically.
# RapidJSON is also header-only; we add the include dir above manually because
# the v1.1.0 tag does not export a proper CMake target.
target_link_libraries(${TARGET_NAME}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        nlohmann_json::nlohmann_json
        GTest::gtest
        GTest::gtest_main
        ${CMAKE_THREAD_LIBS_INIT}
)

# Exceptions must be enabled for nlohmann/json internals even when using
# allow_exceptions=false in the API (the library catches them internally),
# and for the error-quality test in bench_correctness.cpp.
# CompileSettings (pulled in via ThunderCore) adds -fno-exceptions globally;
# source-file-level COMPILE_OPTIONS are appended last, so -fexceptions wins.
# This mirrors the technique already used for test_systeminfo.cpp in the
# existing Thunder unit test suite.
set_source_files_properties(
    MemoryHooks.cpp
    bench_parse.cpp
    bench_serialize.cpp
    bench_memory.cpp
    bench_correctness.cpp
    bench_api_usability.cpp
    PROPERTIES COMPILE_OPTIONS "-fexceptions"
)

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
)

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        RAPIDJSON_HAS_STDSTRING=1
)

install(
    TARGETS ${TARGET_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test
)

add_test(NAME JsonBenchmark COMMAND ${TARGET_NAME} --gtest_color=yes)
