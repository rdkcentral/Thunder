#[[    CompileSettings 
This is a non-buildable target used to set the overall mendatory settings for the 
compiler for all sources of the Framework and its Plugins
#]]
option(POSITION_INDEPENDENT_CODE "Create position independent code on all targets including static libs" ON)

add_library(CompileSettings INTERFACE)
add_library(CompileSettings::CompileSettings ALIAS CompileSettings)

#
# Global options
#
target_include_directories(CompileSettings INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>)

if (POSITION_INDEPENDENT_CODE)
    set_target_properties(CompileSettings PROPERTIES INTERFACE_POSITION_INDEPENDENT_CODE ON)
    message(STATUS "Enabled position independent code")
endif()

if (SYSTEM_PREFIX)
    target_compile_definitions(CompileSettings INTERFACE SYSTEM_PREFIX=${SYSTEM_PREFIX})
    message(STATUS "System prefix is set to: ${SYSTEM_PREFIX}")
endif()

if(NOT BUILD_TYPE)
    set(BUILD_TYPE Production)
    message(AUTHOR_WARNING "BUILD_TYPE not set, assuming '${BUILD_TYPE}'")
endif()

target_compile_definitions(CompileSettings INTERFACE PLATFORM_${PLATFORM}=1)
message(STATUS "Selected platform ${PLATFORM}")

target_compile_options(CompileSettings INTERFACE -std=c++11 -Wno-psabi)

#
# Compiler specific options
#
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Compiling with Clang")
    target_compile_options(CompileSettings INTERFACE -Weverything)
elseif(${CMAKE_COMPILER_IS_GNUCXX})
    message(STATUS "Compiling with GCC")
    target_compile_options(CompileSettings INTERFACE -Wall)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiling with MS Visual Studio")
    target_compile_options(CompileSettings INTERFACE /W4)
else()
    message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

# END CompileSettings
