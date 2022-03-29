option(LOADER_TEST "Utility to load a plugin in isolation." OFF)
option(HTTPSCLIENT_TEST "Example how to do https requests with Thunder." OFF)
option(STORE_TEST "Utility to check performance between PersistentStore and Dictionary PluginUtility." OFF)

if(BUILD_TESTS)
    add_subdirectory(unit)
endif()

if(HTTPSCLIENT_TEST) 
    add_subdirectory(httpsclient)
endif()

if(LOADER_TEST) 
    add_subdirectory(loader)
endif()

if(STORE_TEST)
    # Note:
    # This test has dependency with ThunderInterface module,
    # so please ensure ThunderInterface is built before enabling this test
    add_subdirectory(storeperformance)
endif()
