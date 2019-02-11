include(config)

option(LIBWPEFRAMEWORK
        "Include the libWPEFramework library." ON)
option(CRYPTALGO
        "Include the encyption algorithm library." ON)
option(BROADCAST
        "Include the broadcasting library." OFF)
option(WEBSOCKET
        "Include the websocket library." OFF)
option(PROTOCOLS
        "Include the protocols library." ON)
option(TRACING
        "Include the tracing library." ON)
option(PROFILER
        "Include the profiler library." OFF)
option(COM
        "Include the COM library." OFF)
option(PROTOCOLS
        "Include Protocols lib." ON)
option(CDMI
        "Include OpenCDM interface." OFF)
option(PROCESS
        "Include WPEProcess executable." ON)
option(PROXY_STUB
        "Include proxy/stubs." ON)
option(PLUGINS_LIB
        "Include plugins library." ON)
option(INTERFACES_LIB
        "Include interfaces library." ON)
option(EXECUTABLE
        "Include WPEFramework executable." ON)
option(VIRTUALINPUT
        "Include VirtualInput for external applications." OFF)
option(TEST_APPS
        "Include test applications." OFF)
option(UNIT_TESTS
        "Include unit tests (new gtest based)." OFF)
option(TEST_MEASUREMENTS
        "Include code coverage measurments." OFF)
option(STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)

set(PORT 80 CACHE STRING "The port for the webinterface")
set(BINDING "0.0.0.0" CACHE STRING "The binding interface")
set(IDLE_TIME 180 CACHE STRING "Idle time")
set(PERSISTENT_PATH "/root" CACHE STRING "Persistent path")
set(DATA_PATH "/usr/share/WPEFramework" CACHE STRING "Data path")
set(SYSTEM_PATH "/usr/lib/wpeframework/plugins" CACHE STRING "System path")
set(WEBSERVER_PATH "/boot/www" CACHE STRING "Root path for the HTTP server")
set(WEBSERVER_PORT 8080 CACHE STRING "Port for the HTTP server")
set(PROXYSTUB_PATH "/usr/lib/wpeframework/proxystubs" CACHE STRING "Proxy stub path")
set(CONFIG_INSTALL_PATH "/etc/wpeframework" CACHE STRING "Install location of the wpeframework configuration")
set(IPV6_SUPPORT false CACHE STRING "Controls if wpeframework should support ipv6")
set(PRIORITY 0 CACHE STRING "Change the nice level [-20 - 20]")
set(POLICY 0 CACHE STRING "NA")
set(OOMADJUST 0 CACHE STRING "Adapt the OOM score [-15 - 15]")
set(STACKSIZE 0 CACHE STRING "Default stack size per thread")


set(EXTERN_EVENTS ${EXTERN_EVENTS} CACHE INTERNAL
        "List of events assumed to be provided by other wpeframework plugins.")
string(REPLACE " " ";" EXTERN_EVENT_LIST "${EXTERN_EVENTS}")
message("-- Assuming external events: ${EXTERN_EVENT_LIST}")

#FIXME: We shouldn't touch cflags...
if(TEST_MEASUREMENTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS -lgcov)
endif ()

if(LIBWPEFRAMEWORK)
    add_subdirectory(core)
endif()

if(CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(BROADCAST)
    add_subdirectory(broadcast)
endif()

if(VIRTUALINPUT)
    add_subdirectory(virtualinput)
endif()

if(PROVISIONPROXY)
    add_subdirectory(provisionproxy)
endif()

if(CDMI)
    add_subdirectory(ocdm)
endif()

if(WEBSOCKET)
    add_subdirectory(websocket)
endif()

if(TRACING)
    add_subdirectory(tracing)
endif()

if(PROFILER)
    message(FATAL_ERROR "PROFILER option does not exist?")
    set(PROFILER_INCLUDE_DIR "${ROOT}/Source/profiler")
    add_subdirectory(profiler)
endif()

if(COM)
    add_subdirectory(com)
endif()

if(PROTOCOLS)
    add_subdirectory(protocols)
endif()

if(PROCESS)
    add_subdirectory(WPEProcess)
endif()

if(PROXY_STUB)
    add_subdirectory(proxystubs)
endif()

if(PLUGINS_LIB)
    add_subdirectory(plugins)
endif()

if(INTERFACES_LIB)
    add_subdirectory(interfaces)
endif()

if(EXECUTABLE)
    add_subdirectory(WPEFramework)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
        DESTINATION include/WPEFramework)

include(GenericConfig.cmake)
