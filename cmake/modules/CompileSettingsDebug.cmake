#[[    CompileSettingsDebug
This is a non-buildable target used to set the optional settings for the
compiler for all sources of the Framework and its Plugins based on the build type
#]]

add_library(CompileSettingsDebug INTERFACE)
add_library(CompileSettingsDebug::CompileSettingsDebug ALIAS CompileSettingsDebug)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CompileSettingsDebugConfigVersion.cmake"
    VERSION ${VERSION}
    COMPATIBILITY AnyNewerVersion
)

file(WRITE "${PROJECT_SOURCE_DIR}/cmake/CompileSettingsDebugConfig.cmake.in"
"@PACKAGE_INIT@

include("\$\{CMAKE_CURRENT_LIST_DIR\}/CompileSettingsDebugTargets.cmake")
check_required_components("@PROJECT_NAME@")")

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/CompileSettingsDebugConfig.cmake.in"
   "${CMAKE_CURRENT_SOURCE_DIR}/CompileSettingsDebugConfig.cmake"
    INSTALL_DESTINATION lib/cmake/CompileSettingsDebug
)
message("VERSION == ")
message(${VERSION})
#
# Build type specific options
#
if("${BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(CompileSettingsDebug INTERFACE _DEBUG)
    set(CONFIG_DIR "Debug" CACHE STRING "Build config directory" FORCE)

elseif("${BUILD_TYPE}" STREQUAL "DebugOptimized")
    target_compile_definitions(CompileSettingsDebug INTERFACE _DEBUG)
    set(CONFIG_DIR "DebugOptimized" CACHE STRING "Build config directory" FORCE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR ${CMAKE_COMPILER_IS_GNUCXX} )
        target_compile_options(CompileSettingsDebug INTERFACE -g)
    endif()

elseif("${BUILD_TYPE}" STREQUAL "ReleaseSymbols")
    target_compile_definitions(CompileSettingsDebug INTERFACE NDEBUG)
    set(CONFIG_DIR "ReleaseSymbols" CACHE STRING "Build config directory" FORCE)
    target_compile_options(CompileSettingsDebug INTERFACE "${CMAKE_C_FLAGS_DEBUG}")

elseif("${BUILD_TYPE}" STREQUAL "Release")
    set(CONFIG_DIR "Release" CACHE STRING "Build config directory" FORCE)
    target_compile_definitions(CompileSettingsDebug INTERFACE NDEBUG)

elseif("${BUILD_TYPE}" STREQUAL "Production")
    set(CONFIG_DIR "Production" CACHE STRING "Build config directory" FORCE)
    target_compile_definitions(CompileSettingsDebug INTERFACE NDEBUG PRODUCTION)

else()
    message(FATAL_ERROR "Invalid BUILD_TYPE: '${BUILD_TYPE}'")
endif()

install(TARGETS CompileSettingsDebug EXPORT CompileSettingsDebugTargets)
install(EXPORT CompileSettingsDebugTargets DESTINATION lib/cmake/CompileSettingsDebug)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CompileSettingsDebugConfigVersion.cmake"
              "${CMAKE_CURRENT_SOURCE_DIR}/CompileSettingsDebugConfig.cmake"
        DESTINATION lib/cmake/CompileSettingsDebug)
