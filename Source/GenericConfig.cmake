map()
  key(plugins)
  key(tracing)
  key(version)
  val(${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})
end()
ans(WPEFRAMEWORK_CONFIG)

map_set(${WPEFRAMEWORK_CONFIG} port ${WPEFRAMEWORK_PORT})
map_set(${WPEFRAMEWORK_CONFIG} binding ${WPEFRAMEWORK_BINDING})
map_set(${WPEFRAMEWORK_CONFIG} ipv6 ${WPEFRAMEWORK_IPV6_SUPPORT})
map_set(${WPEFRAMEWORK_CONFIG} idletime ${WPEFRAMEWORK_IDLE_TIME})
map_set(${WPEFRAMEWORK_CONFIG} persistentpath ${WPEFRAMEWORK_PERSISTENT_PATH})
map_set(${WPEFRAMEWORK_CONFIG} datapath ${WPEFRAMEWORK_DATA_PATH})
map_set(${WPEFRAMEWORK_CONFIG} systempath ${WPEFRAMEWORK_SYSTEM_PATH})
map_set(${WPEFRAMEWORK_CONFIG} proxystubpath ${WPEFRAMEWORK_PROXYSTUB_PATH})
map_set(${WPEFRAMEWORK_CONFIG} redirect "/Service/Controller/UI")

map()
    kv(priority ${WPEFRAMEWORK_PRIORITY})
    kv(policy ${WPEFRAMEWORK_POLICY})
    kv(oomadjust ${WPEFRAMEWORK_OOMADJUST})
    kv(stacksize ${WPEFRAMEWORK_STACKSIZE})
end()
ans(WPEFRAMEWORK_PROCESS_CONFIG)
map_append(${WPEFRAMEWORK_CONFIG} process ${WPEFRAMEWORK_PROCESS_CONFIG})

map()
    kv(callsign Controller)
    key(configuration)
end()
ans(WPEFRAMEWORK_PLUGIN_CONTROLLER)

map()
    kv(subsystems)
    key(resumes)
end()
ans(WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION)

list(LENGTH EXTERN_EVENT_LIST EXTERN_EVENT_LIST_LENGTH)
if (EXTERN_EVENT_LIST_LENGTH GREATER 0)
    map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION} subsystems ___array___)

    foreach(SUBSYSTEM IN LISTS EXTERN_EVENT_LIST)
        map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION} subsystems ${SUBSYSTEM})
    endforeach(SUBSYSTEM)
endif()

if (WPEFRAMEWORK_PLUGIN_WEBSERVER OR WPEFRAMEWORK_PLUGIN_WEBKITBROWSER)
    #Add ___array___ to the key to force json array creation.
    map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION} resumes ___array___)
endif()

#if (WPEFRAMEWORK_PLUGIN_WEBKITBROWSER)
    map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION} resumes WebKitBrowser)
#endif (WPEFRAMEWORK_PLUGIN_WEBKITBROWSER)

#if (WPEFRAMEWORK_PLUGIN_WEBSERVER)
    map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION} resumes WebServer)
#endif (WPEFRAMEWORK_PLUGIN_WEBSERVER)

map()
    kv(category "Startup")
    kv(enabled true)
end()
ans(WPEFRAMEWORK_PLUGIN_STARTUP_TRACING)

map()
    kv(category "Shutdown")
    kv(enabled true)
end()
ans(WPEFRAMEWORK_PLUGIN_SHUTDOWN_TRACING)

map()
    kv(category "Fatal")
    kv(enabled true)
end()
ans(WPEFRAMEWORK_PLUGIN_FATAL_TRACING)


if(NOT WPEFRAMEWORK_VIRTUALINPUT)
    map()
    kv(locator "/dev/uinput")
    kv(type "device")
    end()
    ans(WPEFRAMEWORK_PLUGIN_INPUT_DEVICE)

    map_append(${WPEFRAMEWORK_CONFIG} input ${WPEFRAMEWORK_PLUGIN_INPUT_DEVICE})
endif(NOT WPEFRAMEWORK_VIRTUALINPUT)


map_append(${WPEFRAMEWORK_CONFIG} tracing ___array___)
map_append(${WPEFRAMEWORK_CONFIG} tracing ${WPEFRAMEWORK_PLUGIN_STARTUP_TRACING})
map_append(${WPEFRAMEWORK_CONFIG} tracing ${WPEFRAMEWORK_PLUGIN_SHUTDOWN_TRACING})
map_append(${WPEFRAMEWORK_CONFIG} tracing ${WPEFRAMEWORK_PLUGIN_FATAL_TRACING})


map_append(${WPEFRAMEWORK_PLUGIN_CONTROLLER} configuration ${WPEFRAMEWORK_PLUGIN_CONTROLLER_CONFIGURATION})

map_append(${WPEFRAMEWORK_CONFIG} plugins ___array___)
map_append(${WPEFRAMEWORK_CONFIG} plugins ${WPEFRAMEWORK_PLUGIN_CONTROLLER})

#[[ ================================ Add additional config above this line  ================================ ]]

json_write("${CMAKE_BINARY_DIR}/Config.json" ${WPEFRAMEWORK_CONFIG})

install(
        FILES ${CMAKE_BINARY_DIR}/Config.json
        DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/WPEFramework/
        RENAME config.json
        COMPONENT ${MODULE_NAME})