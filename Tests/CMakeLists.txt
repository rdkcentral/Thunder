option(BUILD_TESTS "Build framework tests (requires gtest)" OFF)
option(BUILD_CRYPTOGRAPHY_TESTS "Build cryptography tests" OFF)
option(TEST_LOADER "Build the plugin loader utility" OFF)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fkeep-static-functions -fkeep-inline-functions")
set(CMAKE_EXE_LINKER_FLAGS -lgcov)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS -lsupc++) #Added to resolve the undefined reference to `vtable for __gnu_cxx::recursive_init_error'
set(CMAKE_CXX_STANDARD 11)

if (BUILD_CRYPTOGRAPHY_TESTS)
    add_subdirectory(cryptography)
endif()

if (TEST_LOADER)
    add_subdirectory(loader)
endif()
