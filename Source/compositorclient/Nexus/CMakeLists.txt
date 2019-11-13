set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(EGL REQUIRED)
find_package(NXCLIENT REQUIRED)

add_library(${TARGET} SHARED Implementation.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        NXCLIENT::NXCLIENT
        ${EGL_LIBRARIES}
        )

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        ${EGL_INCLUDE_DIRS})

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        DEFINE_SYMBOL WPEFRAMEWORK_WAYLAND_EGL_CLIENT
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A compositor implementation wrapper for EGL and key handling.")

InstallCMakeConfig(TARGETS ${TARGET})
