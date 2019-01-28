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

target_compile_definitions(CompileSettings INTERFACE PLATFORM_${PLATFORM}=1)
message(STATUS "Selected platform ${PLATFORM}")

target_compile_options(CompileSettings INTERFACE -std=c++11)

#
# Build type specific options
#
if(${BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(CompileSettings INTERFACE _DEBUG)
    set(CONFIG_DIR "Debug" CACHE STRING "Build config directory" FORCE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettings INTERFACE -g -Og)
    endif()

elseif(${BUILD_TYPE} STREQUAL "DebugOptimized")
    target_compile_definitions(CompileSettings INTERFACE _DEBUG)
    set(CONFIG_DIR "DebugOptimized" CACHE STRING "Build config directory" FORCE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettings INTERFACE -g)
    endif()

elseif(${BUILD_TYPE} STREQUAL "ReleaseSymbols")
    target_compile_definitions(CompileSettings INTERFACE NDEBUG)
    set(CONFIG_DIR "ReleaseSymbols" CACHE STRING "Build config directory" FORCE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
    target_compile_options(CompileSettings INTERFACE "${CMAKE_C_FLAGS_DEBUG}")

elseif(${BUILD_TYPE} STREQUAL "Release")
    set(CONFIG_DIR "Release" CACHE STRING "Build config directory" FORCE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    target_compile_definitions(CompileSettings INTERFACE NDEBUG)

elseif(${BUILD_TYPE} STREQUAL "Production")
    set(CONFIG_DIR "Production" CACHE STRING "Build config directory" FORCE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build." FORCE)
    target_compile_definitions(CompileSettings INTERFACE NDEBUG PRODUCTION)

else()
    message(FATAL_ERROR "Invalid build type: " ${BUILD_TYPE})
endif()

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
