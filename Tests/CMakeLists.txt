option(BUILD_TESTS "Build framework tests (requires gtest)" OFF)
option(TEST_LOADER "Build the plugin loader utility" OFF)


if (BUILD_TESTS)
    add_subdirectory(unit)
endif()

if (TEST_LOADER)
    add_subdirectory(loader)
endif()
