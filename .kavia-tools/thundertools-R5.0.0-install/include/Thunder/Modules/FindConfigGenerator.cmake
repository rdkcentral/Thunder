#!/usr/bin/env python3

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

if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
macro(IncludeConfig)
    if(NOT COMMAND map)
        ## includes all cmake files of cmakepp to write json files
        include("${MODULE_BASE_DIR}/config/core/require.cmake")
        require("${MODULE_BASE_DIR}/config/*.cmake")
    endif()
endmacro()
endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)

set(CONFIG_GENERATOR_PATH "${CMAKE_CURRENT_LIST_DIR}/../ConfigGenerator")
set(CONFIG_GENERATOR "${CONFIG_GENERATOR_PATH}/config_generator.py")
set(CONFIG_COMPARE "${CONFIG_GENERATOR_PATH}/config_compare.py")

# ----------------------------------------------------------------------------------------
# write_config
# ----------------------------------------------------------------------------------------
# Writes and installs a json configs for a plugin.
#
#    optional:
#    PLUGINS     List of the config files to write without extention defaults to
#                PROJECT_NAME only
#                e.g. "UX" will generate and install UX.json for the WebkitBrowser plugin.
#    CLASSNAME   Name of the package, defaults to PROJECT_NAME of
#                the latest cmake project call.
#    LOCATOR     Version of the package, defaults to lib + MODULE_NAME + CMAKE_SHARED_LIBRARY_SUFFIX
#                of the latest cmake project call.
#    COMPONENT   The cmake component group defaults to  ${NAMESPACE}_Runtime
#    INSTALL_PATH Overrule the default installation location. 
#    INSTALL_NAME Overrule the default file name
# ----------------------------------------------------------------------------------------
function(write_config)
    set(optionsArgs SKIP_COMPARE DISABLE_LEGACY_GENERATOR SKIP_CLASSNAME SKIP_LOCATOR)
    set(oneValueArgs CLASSNAME LOCATOR COMPONENT INSTALL_PATH INSTALL_NAME CUSTOM_PARAMS_WHITELIST)
    set(multiValueArgs PLUGINS)

    if (NOT CONFIG_GENERATOR)
        message(FATAL_ERROR "The path CONFIG_GENERATOR is not set!")
    endif()

    if(NOT EXISTS "${CONFIG_GENERATOR}" OR IS_DIRECTORY "${CONFIG_GENERATOR}")
        message(FATAL_ERROR "ConfigGenerator path ${CONFIG_GENERATOR} invalid.")
    endif()

    cmake_parse_arguments(ARG "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(ARG_UNPARSED_ARGUMENTS)
        if(ARG_PLUGINS)
            message(FATAL_ERROR "Unknown keywords given to write_config: \"${ARG_UNPARSED_ARGUMENTS}\"")
        else()
            if ("${ARG_UNPARSED_ARGUMENTS}" STREQUAL "${PROJECT_NAME}")
                set(_fix "write_config\(\)")
            else()
                set(_fix "write_config\(PLUGINS ${ARG_UNPARSED_ARGUMENTS}\)")
            endif()
            message(DEPRECATION "======DEPRECATION=DETECTED==================================================\nPROPOSED FIX:\n${CMAKE_CURRENT_LIST_FILE}\n\"write_config(${ARGN})\" --> \"${_fix}\"\n============================================================================")
            set(_plugins ${ARG_UNPARSED_ARGUMENTS})
            set(_classname ${PLUGIN_NAME})
            set(_component ${ARG_UNPARSED_ARGUMENTS})
            message(STATUS "Assuming:")
            message(STATUS "Plugin: ${_plugins}")
            message(STATUS "Classname: ${_classname}")
            message(STATUS "Component: ${_component}")
        endif()
    endif()

    if(ARG_CUSTOM_PARAMS_WHITELIST)
        set(CONFIG_GENERATOR_PARAMS_WHITELIST "${ARG_CUSTOM_PARAMS_WHITELIST}")
    else()
        set(CONFIG_GENERATOR_PARAMS_WHITELIST "${CONFIG_GENERATOR_PATH}/params.config")
    endif()

    if(NOT _component AND NOT _plugins AND NOT _classname)
    if (NOT ARG_SKIP_CLASSNAME)
        if(ARG_CLASSNAME)
            set(_classname ${ARG_CLASSNAME})
        else()
            set(_classname ${PROJECT_NAME})
        endif()
    endif()

    if(ARG_PLUGINS)
        set(_plugins ${ARG_PLUGINS})
    else()
        set(_plugins ${PROJECT_NAME})
    endif()

    if(ARG_COMPONENT)
        set(_component ${ARG_COMPONENT})
    else()
        set(_component  COMPONENT ${NAMESPACE}_Runtime)
    endif()
    endif()

    if(NOT ARG_INSTALL_PATH)
        set(_install_path "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${NAMESPACE}/plugins/")
    else()
        set(_install_path "${ARG_INSTALL_PATH}")
    endif(NOT ARG_INSTALL_PATH)

    if(NOT ARG_INSTALL_NAME)
        set(_install_var_filename plugin)
        set(extenstion ".json")
    else()
        list(LENGTH _plugins _plugins_length)
        if(_plugins_length GREATER 1)
            message(FATAL_ERROR "Using INSTALL_NAME together with multiple plugins will result in file overwrites :')")
        else()
            set(custom_filename "${ARG_INSTALL_NAME}")
            set(_install_var_filename custom_filename)
        endif()
    endif(NOT ARG_INSTALL_NAME)

    if (NOT ARG_SKIP_LOCATOR)
        if(ARG_LOCATOR)
            set(_locator ${ARG_LOCATOR})
        else()
            set(_locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
        endif()
    endif()

    set(COMPARE ON) # default on
    if(ARG_SKIP_COMPARE)
        set(COMPARE OFF)
    endif(ARG_SKIP_COMPARE)

    if(ARG_DISABLE_LEGACY_GENERATOR)
        set(LEGACY_CONFIG_GENERATOR OFF)
    endif(ARG_DISABLE_LEGACY_GENERATOR)

if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
    IncludeConfig()
endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)

    foreach(plugin ${_plugins})
        set(config_generated "N")
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}.conf.in")
            message(VERBOSE "Writing configuration for ${plugin} using config_generator")

            set(_execute_command "${CONFIG_GENERATOR}")

            if (NOT ARG_SKIP_LOCATOR)
                list(APPEND _execute_command  "-l" "${_locator}")
            endif()

            if (NOT ARG_SKIP_CLASSNAME)
                list(APPEND _execute_command  "-c" "${_classname}")
            endif()

            list(APPEND _execute_command  "-o" "${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json")
            list(APPEND _execute_command  "-p" "${CONFIG_GENERATOR_PARAMS_WHITELIST}")
            list(APPEND _execute_command  "--indent" 1)

            file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/config")

            if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}.conf.in")
                configure_file( "${CMAKE_CURRENT_LIST_DIR}/${plugin}.conf.in"
                    "${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.conf"
                    @ONLY)
                list(APPEND _execute_command  "-i" "${plugin}.conf")
            endif()

            list(APPEND _execute_command  ${plugin} "${CMAKE_CURRENT_BINARY_DIR}/config")

            execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} RESULT_VARIABLE rv)
            if(NOT ${rv} EQUAL 0)
                message(FATAL_ERROR "config_generator failed.")
                return()
            endif()
            set(config_generated "Y")
        else()
            if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20.0 OR NOT LEGACY_CONFIG_GENERATOR)
                message(SEND_ERROR "Missing config template:\n\'${CMAKE_CURRENT_LIST_DIR}/${plugin}.conf.in\'\n environment: CMAKE_VERSION=v${CMAKE_VERSION}, LEGACY_CONFIG_GENERATOR=${LEGACY_CONFIG_GENERATOR}")
            endif()
        endif()
       
if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
        if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
                "${config_generated}" STREQUAL "N")

            message(VERBOSE "Writing configuration for ${plugin} using Legacy Config writer")

            map()
            kv(locator ${_locator})
            kv(classname ${_classname})
            end()
            ans(plugin_config) # default configuration
            
            if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${plugin}.config")
                include(${CMAKE_CURRENT_LIST_DIR}/${plugin}.config)

                list(LENGTH preconditions number_preconditions)
                if (number_preconditions GREATER 0)
                    map_append(${plugin_config} precondition ___array___)
                    foreach(entry ${preconditions})
                        map_append(${plugin_config} precondition ${entry})
                    endforeach()
                endif()

                list(LENGTH terminations number_terminations)
                if (number_terminations GREATER 0)
                    map_append(${plugin_config} termination ___array___)
                    foreach(entry ${terminations})
                        map_append(${plugin_config} termination ${entry})
                    endforeach()
                endif()

                if (NOT ${callsign} STREQUAL "")
                    map_append(${plugin_config} callsign ${callsign})
                endif()

                if (NOT ${autostart} STREQUAL "")
                    if (${autostart} STREQUAL "false")
                        map_append(${plugin_config} startmode "Deactivated")
                    else()
                        map_append(${plugin_config} startmode "Activated")
                    endif()
                endif()

                if (NOT ${resumed} STREQUAL "")
                    map_append(${plugin_config} resumed ${resumed})
                endif()

                if (NOT ${persistentpathpostfix} STREQUAL "")
                    map_append(${plugin_config} persistentpathpostfix ${persistentpathpostfix})
                    unset(persistentpathpostfix)
                endif()

               if (NOT ${webui} STREQUAL "")
                    map_append(${plugin_config} webui ${webui})
                    unset(webui)
                endif()
                
                if (NOT ${systemrootpath} STREQUAL "")
                    map_append(${plugin_config} systemrootpath ${systemrootpath})
                    unset(systemrootpath)
                endif()

                if (NOT ${volatilepathpostfix} STREQUAL "")
                    map_append(${plugin_config} volatilepathpostfix ${volatilepathpostfix})
                    unset(volatilepathpostfix)
                endif()

                if (NOT ${startuporder} STREQUAL "")
                    map_append(${plugin_config} startuporder ${startuporder})
                    unset(startuporder)
                endif()

                if (NOT ${startmode} STREQUAL "")
                    map_append(${plugin_config} startmode ${startmode})
                    unset(startmode)
                endif()

                if (NOT ${communicator} STREQUAL "")
                    map_append(${plugin_config} communicator ${communicator})
                    unset(communicator)
                endif()

                if (NOT ${configuration} STREQUAL "")
                    map_append(${plugin_config} configuration ${configuration})
                endif()
            endif()

            if ("${config_generated}" STREQUAL "N")
                json_write("${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json" ${plugin_config})
            endif()

            if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND
                    "${config_generated}" STREQUAL "Y" AND COMPARE)

                json_write("${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}_legacy.json" ${plugin_config})

                if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}_legacy.json" AND 
                        EXISTS ${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json)

                    set(_execute_command "${CONFIG_COMPARE}")
                    list(APPEND _execute_command  "-i")
                    list(APPEND _execute_command  "${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}_legacy.json" "${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json")

                    execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} RESULT_VARIABLE compare_result)

                    if(compare_result EQUAL 0)
                        message(DEBUG "config json files generated using legacy method and Python ConfigGenerator are identical") 
                    elseif( compare_result EQUAL 1)
                        message(AUTHOR_WARNING " Comparing the config json files generated using legacy method and Python ConfigGenerator: \n"
                                            " ${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}_legacy.json and ${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json are not identical \n"
                                            " This could mean one of the following: \n"
                                            "    1. Parameters are out of order \n"
                                            "    2. Parameters are spelled incorrectly  \n")
                    else()
                        message(SEND_ERROR "Error while comparing the config json files generated using legacy method and Python ConfigGenerator")
                    endif()
                else()
                    message(FATAL_ERROR "Failed to compare the config json files generated using legacy method and Python ConfigGenerator - missing")
                endif()
            endif()
        endif()
endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)

        set(_install_filename "${${_install_var_filename}}${extenstion}")       

        install(
                FILES ${CMAKE_CURRENT_BINARY_DIR}/config/${plugin}.json 
                DESTINATION ${_install_path}
                RENAME ${_install_filename}
                COMPONENT ${_component})


    endforeach()


    unset(_plugins)
    unset(_classname)
    unset(_component)

endfunction(write_config)

