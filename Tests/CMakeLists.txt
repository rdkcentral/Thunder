option(BUILD_TESTS "Build framework tests (requires gtest)" OFF)
option(BUILD_CRYPTOGRAPHY_TESTS "Build cryptography tests" OFF)
option(TEST_LOADER "Build the plugin loader utility" OFF)


if (BUILD_TESTS)
    add_subdirectory(unit)
endif()

if (BUILD_CRYPTOGRAPHY_TESTS)
    add_subdirectory(cryptography)
endif()

if (TEST_LOADER)
    add_subdirectory(loader)
endif()