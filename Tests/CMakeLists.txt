option(LOADER_TEST "Utility to load a plugin in isolation." OFF)
option(HTTPSCLIENT_TEST "Example how to do https requests with Thunder." OFF)

if(BUILD_TESTS)
    add_subdirectory(unit)
endif()

if(HTTPSCLIENT_TEST) 
    add_subdirectory(httpsclient)
endif()

if(LOADER_TEST) 
    add_subdirectory(loader)
endif()

if(STRESS_TEST) 
    add_subdirectory(stress)
endif()
