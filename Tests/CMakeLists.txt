option(LOADER_TEST "Utility to load a plugin in isolation." OFF)
option(HTTPSCLIENT_TEST "Example how to do https requests with Thunder." OFF)
option(WORKERPOOL_TEST "WorkerPool stress test" OFF)
option(FILE_UNLINK_TEST "File unlink test" OFF)

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
