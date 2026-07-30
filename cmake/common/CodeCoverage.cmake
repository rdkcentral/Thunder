# Copyright (c) 2012 - 2017, Lars Bilke
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# CHANGES:
#
# 2012-01-31, Lars Bilke
# - Enable Code Coverage
#
# 2013-09-17, Joakim Söderberg
# - Added support for Clang.
# - Some additional usage instructions.
#
# 2016-02-03, Lars Bilke
# - Refactored functions to use named parameters
#
# 2017-06-02, Lars Bilke
# - Merged with modified version from github.com/ufz/ogs
#
# 2019-05-06, Anatolii Kurotych
# - Remove unnecessary --coverage flag
#
# 2019-12-13, FeRD (Frank Dana)
# - Deprecate COVERAGE_LCOVR_EXCLUDES and COVERAGE_GCOVR_EXCLUDES lists in favor
#   of tool-agnostic COVERAGE_EXCLUDES variable, or EXCLUDE setup arguments.
# - CMake 3.4+: All excludes can be specified relative to BASE_DIRECTORY
# - All setup functions: accept BASE_DIRECTORY, EXCLUDE list
# - Set lcov basedir with -b argument
# - Add automatic --demangle-cpp in lcovr, if 'c++filt' is available (can be
#   overridden with NO_DEMANGLE option in setup_target_for_coverage_lcovr().)
# - Delete output dir, .info file on 'make clean'
# - Remove Python detection, since version mismatches will break gcovr
# - Minor cleanup (lowercase function names, update examples...)
#
# 2019-12-19, FeRD (Frank Dana)
# - Rename Lcov outputs, make filtered file canonical, fix cleanup for targets
#
# 2020-01-19, Bob Apthorpe
# - Added gfortran support
#
# 2020-02-17, FeRD (Frank Dana)
# - Make all add_custom_target()s VERBATIM to auto-escape wildcard characters
#   in EXCLUDEs, and remove manual escaping from gcovr targets
#
# 2026-06-19, Bram Oosterhuis
# - Require CMake 3.15, drop all legacy version guards
# - Remove lcov/genhtml support, gcovr only
# - Remove Fortran support
# - Remove Clang version check
# - Remove include(CMakeParseArguments), built-in since CMake 3.5
# - Replace CMAKE_COMPILER_IS_GNUCXX with modern CMAKE_CXX_COMPILER_ID check
# - Remove COVERAGE_LCOVR_EXCLUDES and COVERAGE_GCOVR_EXCLUDES legacy variables
# - Add GCOVR_ARGS parameter to both gcovr targets for passing extra gcovr flags
# - Add EXCLUDE_THROW_BRANCHES / EXCLUDE_UNREACHABLE_BRANCHES
# - Deduplicate the html/xml gcovr targets into one shared implementation
# - Pass the discovered GCOV_PATH to gcovr via --gcov-executable so the gcov used
#   matches the compiler that wrote the .gcno files
#
# USAGE:
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt (best inside an if-condition
#    using a CMake option() to enable it just optionally):
#      include(CodeCoverage)
#
# 3. Append necessary compiler flags:
#      append_coverage_compiler_flags()
#
# 3.a (OPTIONAL) Set appropriate optimization flags, e.g. -O0, -O1 or -Og
#
# 4. If you need to exclude additional directories from the report, specify them
#    using full paths in the COVERAGE_EXCLUDES variable before calling
#    setup_target_for_coverage_*().
#    Example:
#      set(COVERAGE_EXCLUDES
#          "${PROJECT_SOURCE_DIR}/src/dir1/*"
#          "/path/to/my/src/dir2/*")
#    Or, use the EXCLUDE argument to setup_target_for_coverage_*().
#    Example:
#      setup_target_for_coverage_gcovr_html(
#          NAME coverage
#          EXCLUDE "${PROJECT_SOURCE_DIR}/src/dir1/*" "/path/to/my/src/dir2/*")
#
# 5. Build a Debug build and run your tests, then:
#      cmake --build . --target coverage-html-report

cmake_minimum_required(VERSION 3.15)

# Check prereqs
find_program(GCOV_PATH gcov REQUIRED)
find_program(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test REQUIRED)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Code coverage requires GCC or Clang. Aborting...")
endif()

# Helper to collect and normalise exclude patterns into -e arguments for gcovr.
# Usage: _gcovr_collect_excludes(<out_var> <base_dir> [exclude...])
function(_gcovr_collect_excludes out_var base_dir)
    set(excludes "")
    foreach(exclude IN LISTS COVERAGE_EXCLUDES ARGN)
        get_filename_component(exclude "${exclude}" ABSOLUTE BASE_DIR "${base_dir}")
        list(APPEND excludes "${exclude}")
    endforeach()
    list(REMOVE_DUPLICATES excludes)

    set(exclude_args "")
    foreach(exclude IN LISTS excludes)
        list(APPEND exclude_args "-e" "${exclude}")
    endforeach()

    set(${out_var} "${exclude_args}" PARENT_SCOPE)
endfunction()

# setup_target_for_coverage_gcovr_html / setup_target_for_coverage_gcovr_xml
#     NAME       <target>                 # cmake target name
#     EXCLUDE    "src/dir1/*" ...         # patterns to exclude (absolute or relative to BASE_DIRECTORY)
#     GCOVR_ARGS --some-flag ...          # extra flags forwarded verbatim to gcovr
#     BASE_DIRECTORY <path>               # source root for gcovr -r (defaults to PROJECT_SOURCE_DIR)
#     EXECUTABLE <cmd> [args...]          # test executable to run before collecting coverage
#     DEPENDENCIES <target> ...           # cmake targets to build before running
#     EXCLUDE_THROW_BRANCHES ON           # drop exception-unwind branches from the report
#     EXCLUDE_UNREACHABLE_BRANCHES ON     # drop compiler-unreachable branches from the report
#
# Shared implementation behind both wrappers. <format> is "html" or "xml";
# everything except the output flags, the output path and the html
# make_directory step is identical between the two.
function(_setup_target_for_coverage_gcovr format)
    set(one_value_args BASE_DIRECTORY NAME EXCLUDE_THROW_BRANCHES EXCLUDE_UNREACHABLE_BRANCHES)
    set(multi_value_args DEPENDENCIES EXCLUDE EXECUTABLE EXECUTABLE_ARGS GCOVR_ARGS)
    cmake_parse_arguments(PARSE_ARGV 1 Coverage "" "${one_value_args}" "${multi_value_args}")

    if(Coverage_BASE_DIRECTORY)
        get_filename_component(basedir "${Coverage_BASE_DIRECTORY}" ABSOLUTE)
    else()
        set(basedir "${PROJECT_SOURCE_DIR}")
    endif()

    _gcovr_collect_excludes(exclude_args "${basedir}" ${Coverage_EXCLUDE})

    set(extra_args "")
    if(Coverage_EXCLUDE_THROW_BRANCHES)
        list(APPEND extra_args --exclude-throw-branches)
    endif()
    if(Coverage_EXCLUDE_UNREACHABLE_BRANCHES)
        list(APPEND extra_args --exclude-unreachable-branches)
    endif()

    set(pre_commands "")
    if(format STREQUAL "html")
        set(output "${PROJECT_BINARY_DIR}/${Coverage_NAME}")
        set(format_args --html --html-details -o "${output}/index.html")
        set(byproduct "${output}/index.html")
        set(pre_commands COMMAND ${CMAKE_COMMAND} -E make_directory "${output}")
    elseif(format STREQUAL "xml")
        set(output "${PROJECT_BINARY_DIR}/${Coverage_NAME}.xml")
        set(format_args --xml -o "${output}")
        set(byproduct "${output}")
    else()
        message(FATAL_ERROR "_setup_target_for_coverage_gcovr: unknown format '${format}'")
    endif()

    add_custom_target(${Coverage_NAME}
        COMMAND ${Coverage_EXECUTABLE} ${Coverage_EXECUTABLE_ARGS}
        ${pre_commands}
        COMMAND ${GCOVR_PATH}
            --gcov-executable "${GCOV_PATH}"
            -r "${basedir}"
            ${exclude_args}
            ${extra_args}
            ${Coverage_GCOVR_ARGS}
            --object-directory=${PROJECT_BINARY_DIR}
            ${format_args}
        BYPRODUCTS "${byproduct}"
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        DEPENDS ${Coverage_DEPENDENCIES}
        VERBATIM
        COMMENT "Running gcovr ${format} report -> ${output}"
    )
endfunction()

function(setup_target_for_coverage_gcovr_html)
    _setup_target_for_coverage_gcovr(html ${ARGN})
endfunction()

function(setup_target_for_coverage_gcovr_xml)
    _setup_target_for_coverage_gcovr(xml ${ARGN})
endfunction()

function(append_coverage_compiler_flags)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    message(STATUS "Appending code coverage compiler flags")
endfunction()
