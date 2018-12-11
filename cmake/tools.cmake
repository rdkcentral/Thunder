## This is an stripped version of cmakepp
##
## https://github.com/toeb/cmakepp
##
cmake_minimum_required(VERSION 2.8.7)

get_property(is_included GLOBAL PROPERTY INCLUDE_GUARD)
if(is_included)
  _return()
endif()
set_property(GLOBAL PROPERTY INCLUDE_GUARD true)

cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0012 NEW)
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif()
# installation dir of cmakepp
set(CMAKE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")

# get temp dir which is needed by a couple of functions in cmakepp
# first uses env variable TMP if it does not exists TMPDIR is used
# if both do not exists current_list_dir/tmp is used
if(UNIX)
  set(TMP_DIR $ENV{TMPDIR} /var/tmp)
else()
  set(TMP_DIR $ENV{TMP}  ${CMAKE_CURRENT_LIST_DIR}/tmp)
endif()
list(GET TMP_DIR 0 TMP_DIR)
file(TO_CMAKE_PATH "${TMP_DIR}" TMP_DIR)

# dummy function which is overwritten and in this form just returns the temp_dir
function(config_dir key)
	return("${TMP_DIR}")
endfunction()

## includes all cmake files of cmakepp 
include("${CMAKE_BASE_DIR}/tools/core/require.cmake")

require("${CMAKE_BASE_DIR}/tools/*.cmake")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_BASE_DIR}" "${CMAKE_BASE_DIR}/@NAMESPACE@/")
set(NAMESPACE "@NAMESPACE@")

macro(write_config)
    foreach(plugin ${ARGV})
        message("Writing configuration for ${plugin}")

        map()
        kv(locator lib${MODULE_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
        kv(classname ${PLUGIN_NAME})
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

            if (NOT ${autostart} STREQUAL "")
                map_append(${plugin_config} autostart ${autostart})
            endif()

            if (NOT ${configuration} STREQUAL "")
                map_append(${plugin_config} configuration ${configuration})
            endif()
        endif()

        json_write("${CMAKE_CURRENT_LIST_DIR}/${plugin}.json" ${plugin_config})

        install(
                FILES ${plugin}.json DESTINATION
                ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/plugins/
                COMPONENT ${MODULE_NAME})
    endforeach()
endmacro()

function(add_compiler_flags modulename options)
    set(cleanLine ${options})
    foreach(VAL ${cleanLine})
        target_compile_options (${modulename} PRIVATE ${VAL})
    endforeach()
endfunction()

function(add_linker_flags modulename options)
    get_target_property(original ${modulename} LINK_FLAGS)
    if (original) 
        set(cleanLine ${options} ${original})
    else(original)
        set(cleanLine ${options})
    endif(original)
    set (linkoptions "")

    foreach(VAL ${options})
        string(APPEND linkoptions "${VAL} ")
    endforeach()
   
    if (NOT "${linkoptions}" STREQUAL "")
        set_target_properties(${modulename} PROPERTIES LINK_FLAGS "${linkoptions}")
    endif()
endfunction()
