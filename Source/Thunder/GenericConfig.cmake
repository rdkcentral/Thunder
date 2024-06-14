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

# Library installation section
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

set(PORT 80 CACHE STRING "The port for the webinterface")
set(BINDING "0.0.0.0" CACHE STRING "The binding interface")
set(IDLE_TIME 180 CACHE STRING "Idle time")
set(SOFT_KILL_CHECK_WAIT_TIME 10  CACHE STRING "Soft kill check waiting time")
set(HARD_KILL_CHECK_WAIT_TIME 4  CACHE STRING "Hard kill check waiting time")
set(PERSISTENT_PATH "/root" CACHE STRING "Persistent path")
set(DATA_PATH "${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}" CACHE STRING "Data path")
set(SYSTEM_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/plugins" CACHE STRING "System path")
set(WEBSERVER_PATH "/boot/www" CACHE STRING "Root path for the HTTP server")
set(WEBSERVER_PORT 8080 CACHE STRING "Port for the HTTP server")
set(PROXYSTUB_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${NAMESPACE_LIB}/proxystubs" CACHE STRING "Proxy stub path")
set(POSTMORTEM_PATH "/opt/minidumps" CACHE STRING "Core file path to do the postmortem of the crash")
set(MESSAGECONTROL_PATH "MessageDispatcher" CACHE STRING "MessageControl base path to create message files")
set(MESSAGING_PORT 0 CACHE STRING "The port for the messaging")
set(MESSAGING_STDOUT false CACHE STRING "Enable message rederict from stdout")
set(MESSAGING_STDERR false CACHE STRING "Enable message rederict from stderr")
set(MESSAGING_DATASIZE 20480 CACHE STRING "Size of the data buffer in bytes [max 63KB]")
set(CONFIG_INSTALL_PATH "/etc/${NAMESPACE}" CACHE STRING "Install location of the configuration")
set(IPV6_SUPPORT false CACHE STRING "Controls if should application supports ipv6")
set(LEGACY_INITIALZE false CACHE STRING "Enables legacy Plugin Initialize behaviour (Deinit not called on failed Init)")
set(PRIORITY 0 CACHE STRING "Change the nice level [-20 - 20]")
set(POLICY "OTHER" CACHE STRING "NA")
set(OOMADJUST 0 CACHE STRING "Adapt the OOM score [-15 - 15]")
set(STACKSIZE 0 CACHE STRING "Default stack size per thread")
set(KEY_OUTPUT_DISABLED false CACHE STRING "New outputs on the VirtualInput will be disabled by default")
set(EXIT_REASONS "Failure;MemoryExceeded;WatchdogExpired" CACHE STRING "Process exit reason list for which the postmortem is required")
set(ETHERNETCARD_NAME "eth0" CACHE STRING "Ethernet Card name which has to be associated for the Raw Device Id creation")
set(GROUP "" CACHE STRING "Define which system group will be used")
set(UMASK "" CACHE STRING "Set the permission mask for the creation of new files. e.g. 0760")
set(COMMUNICATOR "" CACHE STRING "Define the ComRPC socket e.g. 127.0.0.1:62000 or /tmp/communicator|750")

# Controller Plugin Settings.
set(PLUGIN_CONTROLLER_UI_ENABLED "true" CACHE STRING "Enable the Controller's UI")

if (PLUGIN_CONTROLLER_UI_ENABLED STREQUAL "false")
    set (BINDING "127.0.0.1")
endif()

if(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
map()
  key(plugins)
  if(MESSAGING)
    key(messaging)
  else()
    key(tracing)
  endif()
  key(version)
  val(${Thunder_VERSION_MAJOR}.${Thunder_VERSION_MINOR}.${Thunder_VERSION_PATCH})
end()
ans(CONFIG)

map_set(${CONFIG} port ${PORT})
map_set(${CONFIG} binding ${BINDING})
map_set(${CONFIG} ipv6 ${IPV6_SUPPORT})
if(LEGACY_INITIALZE)
    map_set(${CONFIG} legacyinitialize true)
endif()
map_set(${CONFIG} idletime ${IDLE_TIME})
map_set(${CONFIG} softkillcheckwaittime ${SOFT_KILL_CHECK_WAIT_TIME})
map_set(${CONFIG} hardkillcheckwaittime ${HARD_KILL_CHECK_WAIT_TIME})
map_set(${CONFIG} persistentpath ${PERSISTENT_PATH}/${NAMESPACE})
map_set(${CONFIG} volatilepath ${VOLATILE_PATH})
map_set(${CONFIG} datapath ${DATA_PATH})
map_set(${CONFIG} systempath ${SYSTEM_PATH})
map_set(${CONFIG} proxystubpath ${PROXYSTUB_PATH})
map_set(${CONFIG} postmortempath ${POSTMORTEM_PATH})
map_set(${CONFIG} redirect "/Service/Controller/UI")
map_set(${CONFIG} ethernetcard ${ETHERNETCARD_NAME})
if(NOT COMMUNICATOR STREQUAL "")
    map_set(${CONFIG} communicator ${COMMUNICATOR})
endif()

map()
    kv(priority ${PRIORITY})
    kv(policy ${POLICY})
    kv(oomadjust ${OOMADJUST})
    kv(stacksize ${STACKSIZE})

    if(NOT "${UMASK}" STREQUAL "")
        set(UMASK_LENGTH 0)
        set(UMASK_POS 0)
        set(UMASK_RESULT 0)
        
        string(LENGTH ${UMASK} UMASK_LENGTH)

        if(NOT ${UMASK_LENGTH} EQUAL 3)
            message(FATAL_ERROR "Invalid umask lenght provided, expected was 3 (0-7)(0-7)(0-7)")
        endif()

        while(${UMASK_POS} LESS ${UMASK_LENGTH})
            math(EXPR UMASK_READ_POS "${UMASK_LENGTH}-${UMASK_POS}-1" OUTPUT_FORMAT DECIMAL)

            string(SUBSTRING ${UMASK} ${UMASK_READ_POS} 1 OCTAL_CHAR)

            math(EXPR UMASK_DEC  "0x${OCTAL_CHAR}" OUTPUT_FORMAT DECIMAL)

            if(${UMASK_DEC} GREATER_EQUAL 8)
                message(FATAL_ERROR "Value ${OCTAL_CHAR} is an invalid input for umask")
            endif()

            math(EXPR UMASK_RESULT  "${UMASK_RESULT} | (${UMASK_DEC} << (${UMASK_POS} * 3))" OUTPUT_FORMAT DECIMAL)

            message(TRACE "UMASK_RESULT: ${UMASK_RESULT}, UMASK_READ_POS: ${UMASK_READ_POS} UMASK_POS: ${UMASK_POS}, OCTAL_CHAR: ${OCTAL_CHAR}, UMASK_DEC: ${UMASK_DEC}")
    
            math(EXPR UMASK_POS "${UMASK_POS} +1")
        endwhile()

        kv(umask ${UMASK_RESULT})
    endif()

    if(NOT "${GROUP}" STREQUAL "")
        kv(group ${GROUP})
    endif()
end()
ans(PROCESS_CONFIG)
map_append(${CONFIG} process ${PROCESS_CONFIG})

list(LENGTH EXIT_REASONS EXIT_REASONS_LENGTH)
if (EXIT_REASONS_LENGTH GREATER 0)
    map_append(${CONFIG} exitreasons ___array___ ${EXIT_REASONS})
endif()

map()
    kv(callsign Controller)
    key(configuration)
end()
ans(PLUGIN_CONTROLLER)

map()
    kv(subsystems)
    key(resumes)
    kv(ui ${PLUGIN_CONTROLLER_UI_ENABLED})
end()
ans(PLUGIN_CONTROLLER_CONFIGURATION)

list(LENGTH EXTERN_EVENT_LIST EXTERN_EVENT_LIST_LENGTH)
if (EXTERN_EVENT_LIST_LENGTH GREATER 0)
    map_append(${PLUGIN_CONTROLLER_CONFIGURATION} subsystems ___array___)

    foreach(SUBSYSTEM IN LISTS EXTERN_EVENT_LIST)
        map_append(${PLUGIN_CONTROLLER_CONFIGURATION} subsystems ${SUBSYSTEM})
    endforeach(SUBSYSTEM)
endif()

if (PLUGIN_WEBSERVER OR PLUGIN_WEBKITBROWSER OR PLUGIN_ESPIAL)
    #Add ___array___ to the key to force json array creation.
    map_append(${PLUGIN_CONTROLLER_CONFIGURATION} resumes ___array___)

    if (PLUGIN_WEBKITBROWSER)
        map_append(${PLUGIN_CONTROLLER_CONFIGURATION} resumes WebKitBrowser)
    endif (PLUGIN_WEBKITBROWSER)

    if (PLUGIN_ESPIAL)
        map_append(${PLUGIN_CONTROLLER_CONFIGURATION} resumes EspialBrowser)
    endif (PLUGIN_ESPIAL)

    if (PLUGIN_WEBSERVER)
        map_append(${PLUGIN_CONTROLLER_CONFIGURATION} resumes WebServer)
    endif (PLUGIN_WEBSERVER)
endif()


if(NOT VIRTUALINPUT)
    map()
    kv(locator "/dev/uinput")
    kv(type "device")
    if(KEY_OUTPUT_DISABLED)
        kv(output false)
    endif()
    end()
    ans(PLUGIN_INPUT_DEVICE)

    map_append(${CONFIG} input ${PLUGIN_INPUT_DEVICE})
else(VIRTUALINPUT)
    if(KEY_OUTPUT_DISABLED)
        map()
        kv(type "virtual")
        kv(output false)
        end()
        ans(PLUGIN_INPUT_DEVICE)

        map_append(${CONFIG} input ${PLUGIN_INPUT_DEVICE})
    endif()
endif(NOT VIRTUALINPUT)

map()
    kv(path ${MESSAGECONTROL_PATH})
    kv(port ${MESSAGING_PORT})
    kv(stout ${MESSAGING_STDOUT})
    kv(stderr ${MESSAGING_STDERR})
    kv(datasize ${MESSAGING_DATASIZE})
    key(logging)
    key(tracing)
    key(reporting)
end()
ans(MESSAGING_SETTINGS)

map()
    key(settings)
end()
ans(MESSAGING_TRACING_SETTINGS)

map()
    kv(abbreviated true)
    key(settings)
end()
ans(MESSAGING_LOGGING_SETTINGS)

map()
    kv(abbreviated true)
    key(settings)
end()
ans(MESSAGING_REPORTING_SETTINGS)

if(MESSAGING)
    map()
        kv(category "AnyCategory")
        kv(enabled false)
    end()
    ans(PLUGIN_ANY_CATEGORY_LOGGING)

    map()
        kv(category "AnyCategory")
        kv(enabled true)
    end()
    ans(PLUGIN_ANY_CATEGORY_TRACING)

    map()
        kv(module "Plugin_AnyPlugin")
        kv(enabled true)
    end()
    ans(PLUGIN_ANY_PLUGIN_TRACING)

    map()
        kv(category "AnyCategory")
        kv(enabled false)
    end()
    ans(PLUGIN_ANY_CATEGORY_REPORTING)

    map_append(${MESSAGING_SETTINGS} logging ${MESSAGING_LOGGING_SETTINGS})
    map_append(${MESSAGING_LOGGING_SETTINGS} settings ___array___)
    map_append(${MESSAGING_LOGGING_SETTINGS} settings ${PLUGIN_ANY_CATEGORY_LOGGING})

    map_append(${CONFIG} messaging ${MESSAGING_SETTINGS})
    map_append(${MESSAGING_SETTINGS} tracing ${MESSAGING_TRACING_SETTINGS})
    map_append(${MESSAGING_TRACING_SETTINGS} settings ___array___)
    map_append(${MESSAGING_TRACING_SETTINGS} settings ${PLUGIN_ANY_CATEGORY_TRACING})
    map_append(${MESSAGING_TRACING_SETTINGS} settings ${PLUGIN_ANY_PLUGIN_TRACING})


    map_append(${MESSAGING_SETTINGS} reporting ${MESSAGING_REPORTING_SETTINGS})
    map_append(${MESSAGING_REPORTING_SETTINGS} settings ___array___)
    map_append(${MESSAGING_REPORTING_SETTINGS} settings ${PLUGIN_ANY_CATEGORY_REPORTING})
endif()

map_append(${PLUGIN_CONTROLLER} configuration ${PLUGIN_CONTROLLER_CONFIGURATION})

map_append(${CONFIG} plugins ___array___)
map_append(${CONFIG} plugins ${PLUGIN_CONTROLLER})

#[[ ================================ Add additional config above this line  ================================ ]]

json_write("${CMAKE_BINARY_DIR}/Config.json" ${CONFIG})

install(
        FILES ${CMAKE_BINARY_DIR}/Config.json
        DESTINATION ../${CMAKE_INSTALL_SYSCONFDIR}/${NAMESPACE}/
        RENAME config.json
        COMPONENT ${NAMESPACE}_Runtime)
else()
    find_package(ConfigGenerator REQUIRED)

    write_config(
        SKIP_COMPARE
        SKIP_CLASSNAME
        SKIP_LOCATOR
        CUSTOM_PARAMS_WHITELIST "${CMAKE_CURRENT_LIST_DIR}/params.config"
        INSTALL_PATH "../${CMAKE_INSTALL_SYSCONFDIR}/${NAMESPACE}/"
        INSTALL_NAME "config.json"
        PLUGINS "Thunder"
    )

endif(CMAKE_VERSION VERSION_LESS 3.20.0 AND LEGACY_CONFIG_GENERATOR)
