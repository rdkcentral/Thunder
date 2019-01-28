include(config)

option(WPEFRAMEWORK_LIBWPEFRAMEWORK
        "Include the libWPEFramework library." ON)
option(WPEFRAMEWORK_CRYPTALGO
        "Include the encyption algorithm library." ON)
option(WPEFRAMEWORK_BROADCAST
        "Include the broadcasting library." OFF)
option(WPEFRAMEWORK_WEBSOCKET
        "Include the websocket library." OFF)
option(WPEFRAMEWORK_PROTOCOLS
        "Include the protocols library." ON)
option(WPEFRAMEWORK_TRACING
        "Include the tracing library." ON)
option(WPEFRAMEWORK_PROFILER
        "Include the profiler library." OFF)
option(WPEFRAMEWORK_COM
        "Include the COM library." OFF)
option(WPEFRAMEWORK_PROTOCOLS
        "Include Protocols lib." ON)
option(WPEFRAMEWORK_CDMI
        "Include OpenCDM interface." OFF)
option(WPEFRAMEWORK_PROCESS
        "Include WPEProcess executable." ON)
option(WPEFRAMEWORK_PROXY_STUB
        "Include proxy/stubs." ON)
option(WPEFRAMEWORK_PLUGINS_LIB
        "Include plugins library." ON)
option(WPEFRAMEWORK_INTERFACES_LIB
        "Include interfaces library." ON)
option(WPEFRAMEWORK_WPEFRAMEWORK_EXECUTABLE
        "Include WPEFramework executable." ON)
option(WPEFRAMEWORK_VIRTUALINPUT
        "Include VirtualInput for external applications." OFF)
option(WPEFRAMEWORK_DEADLOCK_DETECTION
        "Enable deadlock detection tooling." OFF)
option(WPEFRAMEWORK_WCHAR_SUPPORT
        "Enable support for WCHAR in ccpsdk." OFF)
option(WPEFRAMEWORK_TEST_APPS
        "Include test applications." OFF)
option(WPEFRAMEWORK_UNIT_TESTS
        "Include unit tests (new gtest based)." OFF)
option(WPEFRAMEWORK_TEST_MEASUREMENTS
        "Include code coverage measurments." OFF)
option(WPEFRAMEWORK_STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)

# TODO make the helpstings more helpful.
set(WPEFRAMEWORK_PORT 80 CACHE STRING "The port for the webinterface")
set(WPEFRAMEWORK_BINDING "0.0.0.0" CACHE STRING "The binding interface")
set(WPEFRAMEWORK_IDLE_TIME 180 CACHE STRING "Idle time")
set(WPEFRAMEWORK_PERSISTENT_PATH "/root" CACHE STRING "Persistent path")
set(WPEFRAMEWORK_DATA_PATH "/usr/share/WPEFramework" CACHE STRING "Data path")
set(WPEFRAMEWORK_SYSTEM_PATH "/usr/lib/wpeframework/plugins" CACHE STRING "System path")
set(WPEFRAMEWORK_WEBSERVER_PATH "/boot/www" CACHE STRING "Root path for the HTTP server")
set(WPEFRAMEWORK_WEBSERVER_PORT 8080 CACHE STRING "Port for the HTTP server")
set(WPEFRAMEWORK_PROXYSTUB_PATH "/usr/lib/wpeframework/proxystubs" CACHE STRING "Proxy stub path")
set(WPEFRAMEWORK_CONFIG_INSTALL_PATH "/etc/wpeframework" CACHE STRING "Install location of the wpeframework configuration")
set(WPEFRAMEWORK_IPV6_SUPPORT false CACHE STRING "Controls if wpeframework should support ipv6")
set(WPEFRAMEWORK_PRIORITY 0 CACHE STRING "Change the nice level [-20 - 20]")
set(WPEFRAMEWORK_POLICY 0 CACHE STRING "NA")
set(WPEFRAMEWORK_OOMADJUST 0 CACHE STRING "Adapt the OOM score [-15 - 15]")
set(WPEFRAMEWORK_STACKSIZE 0 CACHE STRING "Default stack size per thread")


set(EXTERN_EVENTS ${EXTERN_EVENTS} CACHE INTERNAL
        "List of events assumed to be provided by other wpeframework plugins.")
string(REPLACE " " ";" EXTERN_EVENT_LIST "${EXTERN_EVENTS}")
message("-- Assuming external events: ${EXTERN_EVENT_LIST}")

#FIXME: We shouldn't touch cflags...
if(WPEFRAMEWORK_TEST_MEASUREMENTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS -lgcov)
endif ()

if(WPEFRAMEWORK_LIBWPEFRAMEWORK)
    add_subdirectory(core)
endif()

if(WPEFRAMEWORK_CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(WPEFRAMEWORK_BROADCAST)
    add_subdirectory(broadcast)
endif()

if(WPEFRAMEWORK_VIRTUALINPUT)
    add_subdirectory(virtualinput)
endif()

if(WPEFRAMEWORK_PROVISIONPROXY)
    add_subdirectory(provisionproxy)
endif()

if(WPEFRAMEWORK_CDMI)
    add_subdirectory(ocdm)
endif()

if(WPEFRAMEWORK_WEBSOCKET)
    add_subdirectory(websocket)
endif()

if(WPEFRAMEWORK_TRACING)
    add_subdirectory(tracing)
endif()

if(WPEFRAMEWORK_PROFILER)
    message(FATAL_ERROR "WPEFRAMEWORK_PROFILER option does not exist?")
    set(PROFILER_INCLUDE_DIR "${WPEFRAMEWORK_ROOT}/Source/profiler")
    add_subdirectory(profiler)
endif()

if(WPEFRAMEWORK_COM)
    add_subdirectory(com)
endif()

if(WPEFRAMEWORK_PROTOCOLS)
    add_subdirectory(protocols)
endif()

if(WPEFRAMEWORK_PROCESS)
    add_subdirectory(WPEProcess)
endif()

if(WPEFRAMEWORK_PROXY_STUB)
    add_subdirectory(proxystubs)
endif()

if(WPEFRAMEWORK_PLUGINS_LIB)
    add_subdirectory(plugins)
endif()

if(WPEFRAMEWORK_INTERFACES_LIB)
    add_subdirectory(interfaces)
endif()

if(WPEFRAMEWORK_WPEFRAMEWORK_EXECUTABLE)
    add_subdirectory(WPEFramework)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
        DESTINATION include/WPEFramework)

include(GenericConfig.cmake)
