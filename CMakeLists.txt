cmake_minimum_required(VERSION 3.3)

project(WPEFramework)

set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        "${CMAKE_SOURCE_DIR}/cmake"
        "${CMAKE_SOURCE_DIR}/cmake/modules")

include(platform)
include(HandleStandardLib)
include(GetGitRepo)

set(WPEFRAMEWORK_PLATFORM "PC_UNIX" CACHE STRING
        "Defines on what platform WPEFramework will run")

option(INSTALL_LOCALLY "Install this project locally" OFF)
option(POSITION_INDEPENDENT_CODE "Create position independent code on all targets including static libs" ON)

#FIXME: find how to set -Wall cmake style :-)
set(CMAKE_COMPILER_IS_CLANG OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compiling with Clang")
    set(CMAKE_COMPILER_IS_CLANG ON)
    add_definitions(-Weverything)
elseif(${CMAKE_COMPILER_IS_GNUCXX})
    message(STATUS "Compiling with GCC")
    add_definitions(-Wall)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiling with MS Visual Studio")
    add_definitions(/W4)
else()
    message(STATUS "Compiler " ${CMAKE_CXX_COMPILER_ID})
endif()

if (INSTALL_LOCALLY)
    set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install)
    set(CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR}/cmake)
else()
    set(CMAKE_PREFIX_PATH ${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/cmake ${CMAKE_PREFIX_PATH})
endif()

if (POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_REVISION "0000000")

# Git (and its revision)
find_package(Git QUIET) # if we don't find git or FindGit.cmake is not on the system we ignore it.

if(GIT_FOUND) # GIT revision
    include(GetGitRevisionDescription)
    get_git_short_revision(GIT_SHA1)
    message(STATUS "GIT revision ${GIT_SHA1}")
    set(VERSION_REVISION ${GIT_SHA1})
endif()

set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION})

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

message(STATUS "Setting up ${PROJECT_NAME} v${VERSION}")

set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

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
