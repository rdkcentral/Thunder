#[[    CompileSettingsDebug
This is a non-buildable target used to set the optional settings for the
compiler for all sources of the Framework and its Plugins based on the build type
#]]

add_library(CompileSettingsDebug INTERFACE)
add_library(CompileSettingsDebug::CompileSettingsDebug ALIAS CompileSettingsDebug)

include(CMakePackageConfigHelpers)

#
# Build type specific options
#
if("${BUILD_TYPE}" STREQUAL "DebugOptimized")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettingsDebug INTERFACE -g)
    endif()

elseif("${BUILD_TYPE}" STREQUAL "ReleaseSymbols")
    target_compile_options(CompileSettingsDebug INTERFACE "${CMAKE_C_FLAGS_DEBUG}")
endif()

install(TARGETS CompileSettingsDebug EXPORT CompileSettingsDebugTargets)
