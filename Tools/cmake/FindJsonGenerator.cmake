# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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
    find_package(PythonInterp 3.5 REQUIRED QUIET)
endif()

set(JSON_GENERATOR "@GENERATOR_INSTALL_PATH@/JsonGenerator/JsonGenerator.py")

function(JsonGenerator)
    if (NOT JSON_GENERATOR)
        message(FATAL_ERROR "The path JSON_GENERATOR is not set!")
    endif()

    if(NOT EXISTS "${JSON_GENERATOR}" OR IS_DIRECTORY "${JSON_GENERATOR}")
        message(FATAL_ERROR "JsonGenerator path ${JSON_GENERATOR} invalid.")
    endif()

    set(optionsArgs CODE STUBS DOCS NO_WARNINGS COPY_CTOR NO_REF_NAMES)
    set(oneValueArgs OUTPUT IFDIR INDENT DEF_STRING DEF_INT_SIZE PATH)
    set(multiValueArgs INPUT)

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

    if(Argument_NO_WARNINGS)
        list(APPEND _execute_command  "--no-warnings")
    endif()

    if(Argument_COPY_CTOR)
        list(APPEND _execute_command  "--copy-ctor")
    endif()

    if(Argument_NO_REF_NAMES)
        list(APPEND _execute_command  "--no-ref-names")
    endif()

    if (Argument_PATH)
        list(APPEND _execute_command  "-p" "${Argument_PATH}")
    endif()

    if (Argument_IFDIR)
        list(APPEND _execute_command  "-i" "${Argument_IFDIR}")
    endif()

    if (Argument_OUTPUT)
        file(MAKE_DIRECTORY "${Argument_OUTPUT}")
        list(APPEND _execute_command  "--output" "${Argument_OUTPUT}")
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

    foreach(_input ${Argument_INPUT})
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} ${_input} RESULT_VARIABLE rv)
        if(NOT ${rv} EQUAL 0)
            message(FATAL_ERROR "JsonGenerator generator failed.")
        endif()
    endforeach(_input)
endfunction(JsonGenerator)

message(STATUS "JsonGenerator ready ${JSON_GENERATOR}")
