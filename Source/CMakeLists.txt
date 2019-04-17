include(config)

option(CORE
        "Include the core library." ON)
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
option(PLUGINS
        "Include plugins library." ON)
option(INTERFACES
        "Include interfaces library." ON)
option(EXECUTABLE
        "Include the hosting executable." ON)
option(VIRTUALINPUT
        "Include VirtualInput for external applications." OFF)
option(JSONRPC
        "Include JSONRPC C++ utility library for external applications." ON)
option(TEST_APPS
        "Include test applications." OFF)
option(UNIT_TESTS
        "Include unit tests (new gtest based)." OFF)
option(TEST_MEASUREMENTS
        "Include code coverage measurments." OFF)
option(STUB_HARDWARE
        "Use hardware stubs for unittests)." OFF)
option(BUILD_TYPE
        "How do you want to build it, Production, Release or Debug" Production)

set(EXTERN_EVENTS ${EXTERN_EVENTS} CACHE INTERNAL
        "List of events assumed to be provided by other plugins.")
string(REPLACE " " ";" EXTERN_EVENT_LIST "${EXTERN_EVENTS}")
message("-- Assuming external events: ${EXTERN_EVENT_LIST}")

#FIXME: We shouldn't touch cflags...
if(TEST_MEASUREMENTS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS -lgcov)
endif ()

if(CORE)
    add_subdirectory(core)
endif()

if(CRYPTALGO)
    add_subdirectory(cryptalgo)
endif()

if(BROADCAST)
    add_subdirectory(broadcast)
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

if(PLUGINS)
    add_subdirectory(plugins)
endif()

if(INTERFACES)
    add_subdirectory(interfaces)
endif()

if(EXECUTABLE)
    add_subdirectory(WPEFramework)
endif()

if(VIRTUALINPUT)
    add_subdirectory(virtualinput)
endif()

if(JSONRPC)
    add_subdirectory(jsonrpc)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake
        DESTINATION include/${NAMESPACE})
