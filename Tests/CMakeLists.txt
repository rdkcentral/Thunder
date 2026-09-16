option(LOADER_TEST "Utility to load a plugin in isolation." OFF)
option(HTTPSCLIENT_TEST "Example how to do https requests with Thunder." OFF)
option(WORKERPOOL_TEST "WorkerPool stress test" OFF)
option(FILE_UNLINK_TEST "File unlink test" OFF)
option(STREAMJSON_GARBAGE_TEST "Reproducer for issue #1963: infinite loop on garbage data in StreamJSONType::ReceiveData()" OFF)
option(INTENTIONAL_CRASH_TEST "Reproducer for the INTENTIONAL_CRASH() macro (Source/core/Trace.h)" OFF)
option(NOTIFICATION_TIMEOUT_INSTRUMENTATION_TEST "Reproducer for the special, non-upstream IPlugin::INotification timeout instrumentation. Requires -DTHUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION=ON" OFF)

if(BUILD_TESTS)
    add_subdirectory(unit)
endif()

if (FILE_UNLINK_TEST)
    add_subdirectory(file-unlink)
endif()

if(HTTPSCLIENT_TEST) 
    add_subdirectory(httpsclient)
endif()

if(LOADER_TEST) 
    add_subdirectory(loader)
endif()

if(WORKERPOOL_TEST)
    add_subdirectory(workerpool-test)
endif()

if(STREAMJSON_GARBAGE_TEST)
    add_subdirectory(streamjson-garbage)
endif()

if(INTENTIONAL_CRASH_TEST)
    add_subdirectory(intentional-crash-test)
endif()

if(NOTIFICATION_TIMEOUT_INSTRUMENTATION_TEST)
    if(NOT THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION)
        message(FATAL_ERROR "NOTIFICATION_TIMEOUT_INSTRUMENTATION_TEST also requires -DTHUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION=ON")
    endif()
    add_subdirectory(notification-timeout-instrumentation-test)
endif()
