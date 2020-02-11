find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
set(CMAKE_CXX_STANDARD 11)

enable_testing()

add_subdirectory(core)
add_subdirectory(tests)

