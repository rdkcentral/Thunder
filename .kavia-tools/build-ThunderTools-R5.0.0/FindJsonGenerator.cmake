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

if(NOT PYTHON_EXECUTABLE)
    find_package(Python3 3.5 REQUIRED QUIET)
endif()

set(JSON_GENERATOR "${CMAKE_CURRENT_LIST_DIR}/../JsonGenerator/JsonGenerator.py")

function(JsonGenerator)
    if (NOT JSON_GENERATOR)
        message(FATAL_ERROR "The path JSON_GENERATOR is not set!")
    endif()

    if(NOT EXISTS "${JSON_GENERATOR}" OR IS_DIRECTORY "${JSON_GENERATOR}")
        message(FATAL_ERROR "JsonGenerator path ${JSON_GENERATOR} invalid.")
    endif()

    set(optionsArgs CODE STUBS DOCS LEGACY_ALT AUTO_PREFIX NO_INCLUDES NO_WARNINGS NO_STYLE_WARNINGS DUPLICATE_OBJ_WARNINGS COPY_CTOR NO_REF_NAMES NO_INTERFACES_SECTION VERBOSE FORCE_GENERATE EMIT_INTERFACE_PATH )
    set(oneValueArgs OUTPUT CPP_OUTPUT INDENT DEF_STRING DEF_INT_SIZE PATH FORMAT CPP_INTERFACE_PATH JSON_INTERFACE_PATH FRAMEWORK_NAMESPACE)
    set(multiValueArgs INPUT IFDIR CPPIFDIR INCLUDE_PATH NAMESPACE)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to JsonGenerator(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_execute_command ${JSON_GENERATOR})

    if(Argument_CODE)
        list(APPEND _execute_command  "--code")
    endif()

    if(Argument_STUBS)
        list(APPEND _execute_command  "--stubs")
    endif()

    if(Argument_DOCS)
        list(APPEND _execute_command  "--docs")
    endif()

    if(Argument_LEGACY_ALT)
        list(APPEND _execute_command  "--legacy-alt")
    endif()

    if(Argument_AUTO_PREFIX)
        list(APPEND _execute_command  "--auto-prefix")
    endif()

    if(Argument_NO_INCLUDES)
        list(APPEND _execute_command  "--no-includes")
    endif()

    if(Argument_NO_WARNINGS)
        list(APPEND _execute_command  "--no-warnings")
    endif()

    if(Argument_NO_STYLE_WARNINGS)
        list(APPEND _execute_command  "--no-style-warnings")
    endif()

    if(DUPLICATE_OBJ_WARNINGS)
        list(APPEND _execute_command  "--duplicate-obj-warnings")
    endif()

    if(Argument_COPY_CTOR)
        list(APPEND _execute_command  "--copy-ctor")
    endif()

    if(Argument_NO_REF_NAMES)
        list(APPEND _execute_command  "--no-ref-names")
    endif()

    if(Argument_NO_INTERFACES_SECTION)
        list(APPEND _execute_command  "--no-interfaces-section")
    endif()

    if(Argument_VERBOSE)
        list(APPEND _execute_command  "--verbose")
    endif()

    if(Argument_FORCE_GENERATE)
        list(APPEND _execute_command  "--force")
    endif()

    if(Argument_EMIT_INTERFACE_PATH)
        list(APPEND _execute_command  "--emit-cpp-interface-path")
    endif()

    if (Argument_PATH)
        list(APPEND _execute_command  "-p" "${Argument_PATH}")
    endif()

    foreach(_include_path ${Argument_IFDIR})
        list(APPEND _execute_command  "-i" "${_include_path}")
    endforeach(_include_path)

    foreach(_include_path ${Argument_CPPIFDIR})
        list(APPEND _execute_command  "-j" "${_include_path}")
    endforeach(_include_path)

    if (Argument_OUTPUT)
        file(MAKE_DIRECTORY "${Argument_OUTPUT}")
        list(APPEND _execute_command  "--output" "${Argument_OUTPUT}")
    endif()

    if (Argument_CPP_OUTPUT)
        file(MAKE_DIRECTORY "${Argument_CPP_OUTPUT}")
        list(APPEND _execute_command  "--cpp-output" "${Argument_CPP_OUTPUT}")
    endif()

    if (Argument_INDENT)
        list(APPEND _execute_command  "--indent" "${Argument_INDENT}")
    endif()

    if (Argument_DEF_STRING)
        list(APPEND _execute_command  "--def-string" "${Argument_DEF_STRING}")
    endif()

    if (Argument_DEF_INT_SIZE)
        list(APPEND _execute_command  "--def-int-size" "${Argument_DEF_INT_SIZE}")
    endif()

    if (Argument_FORMAT)
        list(APPEND _execute_command "--format" "${Argument_FORMAT}")
    endif()

    if (Argument_CPP_INTERFACE_PATH)
        list(APPEND _execute_command "--emit-cpp-interface-path" "${Argument_CPP_INTERFACE_PATH}")
    endif()

    if (Argument_JSON_INTERFACE_PATH)
        list(APPEND _execute_command "--emit-json-interface-path" "${Argument_JSON_INTERFACE_PATH}")
    endif()

    if (Argument_FRAMEWORK_NAMESPACE)
        list(APPEND _execute_command "--framework-namespace" "${Argument_FRAMEWORK_NAMESPACE}")
    endif()

    foreach(_namespace ${Argument_NAMESPACE})
        list(APPEND _execute_command "--namespace" "${_namespace}")
    endforeach(_namespace)

    foreach(_include_path ${Argument_INCLUDE_PATH})
        list(APPEND _execute_command  "-I" "${_include_path}")
    endforeach(_include_path)

    foreach(_input ${Argument_INPUT})
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} ${_input} RESULT_VARIABLE rv)
        if(NOT ${rv} EQUAL 0)
            message(FATAL_ERROR "JsonGenerator generator failed.")
        endif()
    endforeach(_input)
endfunction(JsonGenerator)

message(VERBOSE "JsonGenerator ready ${JSON_GENERATOR}")

function(DocGenerator)

    set(oneValueArgs NAME)
    cmake_parse_arguments(Argument "" "${oneValueArgs}" "" ${ARGN})
    set(PLUGIN_NAME ${Argument_NAME})

    set(THUNDER_IFDIR_PATH ${CMAKE_INSTALL_FULL_INCLUDEDIR}/${NAMESPACE})
    set(CPPIFDIR_PATH ${THUNDER_IFDIR_PATH}/interfaces)
    set(IFDIR_PATH ${THUNDER_IFDIR_PATH}/interfaces/jsonrpc)
    JsonGenerator(INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_NAME}Plugin.json DOCS OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/doc" INCLUDE_PATH ${THUNDER_IFDIR_PATH} CPPIFDIR ${CPPIFDIR_PATH} IFDIR ${IFDIR_PATH})

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/doc/${PLUGIN_NAME}Plugin.md DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc" COMPONENT ${NAMESPACE}_Documentation)
endfunction(DocGenerator)
