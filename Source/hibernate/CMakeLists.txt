# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
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

set(TARGET ${NAMESPACE}Hibernate)

# Implementation options
option(HIBERNATE_CHECKPOINTLIB "Use checkpoint lib implementation." OFF)
option(HIBERNATE_CHECKPOINTSERVER "Use checkpoint server implementation." OFF)

# Construct a library object
add_library(${TARGET} SHARED )

# Selects implementation
if (HIBERNATE_CHECKPOINTLIB)
    target_sources(${TARGET} PRIVATE checkpointlib/CheckpointLib.c)
elseif(HIBERNATE_CHECKPOINTSERVER)
    target_sources(${TARGET} PRIVATE checkpointserver/CheckpointServer.c)
endif()

set(PUBLIC_HEADERS
        hibernate.h
        )

set_target_properties(${TARGET} PROPERTIES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

if(HIBERNATE_CHECKPOINTLIB)
        find_package(Memcr REQUIRED)
        target_link_libraries(${TARGET} PRIVATE MEMCR::MEMCR)
        target_include_directories(${TARGET} PRIVATE MEMCR::MEMCR)
elseif(HIBERNATE_CHECKPOINTSERVER)
    # nothing to add here
endif()

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/hibernate COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/hibernate    # headers
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Enable ease of development for Hibernate support." )