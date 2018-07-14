include(config)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-D_DEBUG)
    set(CONFIG_DIR "debug" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DNDEBUG)
    set(CONFIG_DIR "release" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
    add_definitions(-DNDEBUG)
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
    add_definitions(-DNDEBUG)
    set(CONFIG_DIR "releaseMinSize" CACHE STRING "Build config directory" FORCE)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Production")
    add_definitions(-DNDEBUG)
    add_definitions(-DPRODUCTION)
    set(CONFIG_DIR "production" CACHE STRING "Build config directory" FORCE)
else()
    message(FATAL_ERROR "Invalid build type: " ${CMAKE_BUILD_TYPE})
endif()

option(WPEFRAMEWORK_LIBWPEFRAMEWORK
        "Include the libWPEFramework library." ON)
option(WPEFRAMEWORK_CRYPTALGO
        "Include the encyption algorithm library." ON)
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


set(EXTERN_EVENTS ${WPEFRAMEWORK_EXTERN_EVENTS} CACHE INTERNAL
        "List of events assumed to be provided by other wpeframework plugins.")
string(REPLACE " " ";" EXTERN_EVENT_LIST "${EXTERN_EVENTS}")
message("-- Assuming external events: ${EXTERN_EVENT_LIST}")

#FIXME: This is only here for package config files
set(WCHAR_SUPPORT_DEFINITION)
if(NOT WPEFRAMEWORK_WCHAR_SUPPORT)
    set(WCHAR_SUPPORT_DEFINITION __NO_WCHAR_SUPPORT__)
endif()

#FIXME: This is only here for package config files
set(CRITICAL_SECTION_LOCK_DEFINITION)
if(WPEFRAMEWORK_DEADLOCK_DETECTION)
    set(CRITICAL_SECTION_LOCK_DEFINITION CRITICAL_SECTION_LOCK_LOG)
endif()

#FIXME: We shouldn't touch cflags...
if(WPEFRAMEWORK_TEST_MEASUREMENTS)
    list(APPEND CMAKE_CXX_FLAGS "-fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS -lgcov)
endif ()


#FIXME: for now this is at directory scope instead of target scope for convenience.
add_definitions(-DPLATFORM_${WPEFRAMEWORK_PLATFORM}=1)

if (BUILDREF_WPEFRAMEWORK)
    set(BUILDREF_WPEFRAMEWORK "${WPEFRAMEWORK_VERSION}|${BUILDREF_WPEFRAMEWORK}")
else()
    set(BUILDREF_WPEFRAMEWORK "${WPEFRAMEWORK_VERSION}")
endif()

#FIXME: for now this is at directory scope instead of target scope for convenience.
add_definitions(-DBUILDREF_WPEFRAMEWORK=${BUILDREF_WPEFRAMEWORK})

# We are introducing *_ARTIFACT variables, that enable linking to already built targets.
# These need to be defined here, contrary to inside the module folder, as variables are not normally exported to the parent.
# The *_ARTIFACT variables are also used as the names of generated libraries, so you can link against them by using the variables.

if(WPEFRAMEWORK_LIBWPEFRAMEWORK)
    set(CORE_ARTIFACT ${NAMESPACE}Core)
    add_subdirectory(core)
endif()

if(WPEFRAMEWORK_CRYPTALGO)
    set(CRYPTALGO_ARTIFACT ${NAMESPACE}Cryptalgo)
    add_subdirectory(cryptalgo)
endif()

if(WPEFRAMEWORK_VIRTUALINPUT)
    set(VIRTUALINPUT_ARTIFACT ${NAMESPACE}VirtualInput)
    add_subdirectory(virtualinput)
endif()

if(WPEFRAMEWORK_PROVISIONPROXY)
    set(PROVISIONPROXY_ARTIFACT provisionproxy)
    add_subdirectory(provisionproxy)
endif()

if(WPEFRAMEWORK_CDMI)
    set(CDMI_ARTIFACT ocdm)
    add_subdirectory(ocdm)
endif()

if(WPEFRAMEWORK_WEBSOCKET)
    set(WEBSOCKET_ARTIFACT ${NAMESPACE}WebSocket)
    add_subdirectory(websocket)
endif()

if(WPEFRAMEWORK_TRACING)
    set(TRACING_ARTIFACT ${NAMESPACE}Tracing)
    add_subdirectory(tracing)
endif()

if(WPEFRAMEWORK_PROFILER)
    message(FATAL_ERROR "WPEFRAMEWORK_PROFILER option does not exist?")
    set(PROFILER_INCLUDE_DIR "${WPEFRAMEWORK_ROOT}/Source/profiler")
    add_subdirectory(profiler)
endif()

if(WPEFRAMEWORK_COM)
    set(COM_ARTIFACT ${NAMESPACE}COM)
    add_subdirectory(com)
endif()

if(WPEFRAMEWORK_PROTOCOLS)
    set(PROTOCOLS_ARTIFACT ${NAMESPACE}Protocols)
    add_subdirectory(protocols)
endif()

if(WPEFRAMEWORK_PROCESS)
    set(WPEPROCESS_ARTIFACT WPEProcess)
    add_subdirectory(WPEProcess)
endif()

if(WPEFRAMEWORK_PROXY_STUB)
    set(PROXYSTUBS_ARTIFACT ${NAMESPACE}ProxyStubs)
    add_subdirectory(proxystubs)
endif()

if(WPEFRAMEWORK_PLUGINS_LIB)
    set(PLUGINS_ARTIFACT ${NAMESPACE}Plugins)
    add_subdirectory(plugins)
endif()

if(WPEFRAMEWORK_INTERFACES_LIB)
    set(PLUGINS_INTERFACES_ARTIFACT ${NAMESPACE}Interfaces)
    add_subdirectory(interfaces)
endif()

if(WPEFRAMEWORK_WPEFRAMEWORK_EXECUTABLE)
    set(PLUGINHOST_ARTIFACT WPEFramework)
    add_subdirectory(WPEFramework)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
        DESTINATION include/WPEFramework)

include(GenericConfig.cmake)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/WPEFramework.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/WPEFramework.pc"
        @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WPEFramework.pc
        DESTINATION "lib/pkgconfig")
