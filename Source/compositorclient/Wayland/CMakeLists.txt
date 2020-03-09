# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(NOT PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION)
    set(PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION "Westeros")
endif()

if ((NOT "${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Weston") AND
    (NOT "${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}" STREQUAL "Westeros"))
message( FATAL_ERROR "Invalid Wayland implementation, set PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION (currently: [${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}]) to \"Weston\" or \"Westeros\"" )
endif ()

find_package(WaylandClient REQUIRED)
find_package(EGL REQUIRED)
find_package(WaylandEGL REQUIRED)
find_package(${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client REQUIRED)
find_package(NXCLIENT)

add_library(${TARGET} SHARED ${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        WaylandEGL::WaylandEGL
        ${EGL_LIBRARIES}
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client::${PLUGIN_COMPOSITOR_SUB_IMPLEMENTATION}Client
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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
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

install(
	FILES Implementation.h
        DESTINATION include/${NAMESPACE}/compositorclient
)

