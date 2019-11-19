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
find_package(ExecInfo)

set(PLATFORM "PC_UNIX" CACHE STRING
        "Defines on what platform the application will run")

option(INSTALL_LOCALLY "Install this project locally" OFF)
option(BUILD_TESTS "Build tests (requires gtest)" OFF)

set(NAMESPACE ${PROJECT_NAME} CACHE STRING "Namespace of the project")

find_package(PythonInterp 2.7 REQUIRED)
set(JSON_GENERATOR "${PROJECT_SOURCE_DIR}/Tools/JsonGenerator/JsonGenerator.py")
set(PROXYSTUB_GENERATOR "${PROJECT_SOURCE_DIR}/Tools/ProxyStubGenerator/StubGenerator.py")

if (INSTALL_LOCALLY)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/install")
    set(CMAKE_PREFIX_PATH "${PROJECT_BINARY_DIR}/cmake")
endif()

# make sure others can make use of the JSON creation tools as well!!!
configure_file( "${CMAKE_SOURCE_DIR}/cmake/project.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
                @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}.cmake"
        RENAME "${NAMESPACE}Config.cmake"
        DESTINATION lib/cmake/${NAMESPACE})

install(DIRECTORY
            "${CMAKE_SOURCE_DIR}/cmake/config"
            "${CMAKE_SOURCE_DIR}/cmake/templates"
            "${CMAKE_SOURCE_DIR}/cmake/modules"
        DESTINATION lib/cmake/${NAMESPACE})

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
