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