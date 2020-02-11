option(BUILD_TESTS "Build framework tests (requires gtest)" OFF)
option(BUILD_CRYPTOGRAPHY_TESTS "Build cryptography tests" OFF)
option(TEST_LOADER "Build the plugin loader utility" OFF)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
set(CMAKE_CXX_STANDARD 11)

if (BUILD_CRYPTOGRAPHY_TESTS)
    add_subdirectory(cryptography)
endif()

if (TEST_LOADER)
    add_subdirectory(loader)
endif()
