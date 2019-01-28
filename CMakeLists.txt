cmake_minimum_required(VERSION 3.3)

project(WPEFramework)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

message(STATUS "Setting up ${PROJECT_NAME} v${VERSION}")

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        "${CMAKE_SOURCE_DIR}/cmake"
        "${CMAKE_SOURCE_DIR}/cmake/modules")

include(platform)
include(CmakeHelperFunctions)

set(PLATFORM "PC_UNIX" CACHE STRING
        "Defines on what platform WPEFramework will run")

option(INSTALL_LOCALLY "Install this project locally" OFF)
option(BUILD_TESTS "Build tests (requires gtest)" OFF)

set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

if (INSTALL_LOCALLY)
    set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install)
    set(CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR}/cmake)
else()
    set(CMAKE_PREFIX_PATH ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/cmake ${CMAKE_PREFIX_PATH})
endif()

# make sure others can make use of the JSON creation tools as well!!!
configure_file( "${CMAKE_SOURCE_DIR}/cmake/wpeframework.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/WPEFramework.cmake"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WPEFramework.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake")
# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WPEFramework.cmake
#         DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake"
#         RENAME tools.cmake)
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/config/
#         DESTINATION include/cmake/tools)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/config/
        DESTINATION include/cmake/WPEFramework/config)

if(APPLE)
    # Mac needed variables
    # http://www.cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

add_subdirectory(Source)

if(BUILD_TESTS)
    add_subdirectory(Tests)
endif()
