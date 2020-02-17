find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Source)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -fkeep-static-functions -fkeep-inline-functions")
set(CMAKE_EXE_LINKER_FLAGS -lgcov)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS -lsupc++) #Added to resolve the undefined reference to `vtable for __gnu_cxx::recursive_init_error'
set(CMAKE_CXX_STANDARD 11)

enable_testing()

add_subdirectory(core)
add_subdirectory(tests)

