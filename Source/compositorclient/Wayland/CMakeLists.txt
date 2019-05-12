find_package(WaylandClient REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(WesterosClient REQUIRED)
find_package(NXCLIENT)

add_library(${TARGET} SHARED Westeros.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        WaylandEGL::WaylandEGL
        ${EGL_LIBRARIES}
        ${NAMESPACE}Core::${NAMESPACE}Core
        WesterosClient::WesterosClient
        WaylandClient::WaylandClient)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
)

if (NXCLIENT_FOUND)
    target_link_libraries(${TARGET}
        PRIVATE
            NXCLIENT::NXCLIENT)
endif ()

find_package(BCM_HOST QUIET)
if(BCM_HOST_FOUND)
    target_link_libraries(${TARGET}
        PRIVATE
            BCM_HOST::BCM_HOST)
    target_compile_definitions(${TARGET}
        PRIVATE
            BCM_HOST=ON)
endif(BCM_HOST_FOUND)

if(EGL_FOUND)
    add_definitions(${EGL_CFLAGS})
endif(EGL_FOUND)

if (PLUGIN_COMPOSITOR_NXSERVER)
    if (V3D_DRM_DISABLE)
        target_compile_definitions(${TARGET}
            PRIVATE
                V3D_DRM_DISABLE=1)
    endif ()
endif ()

target_include_directories(${TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../Client>
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        ${EGL_INCLUDE_DIRS}
)

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
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
