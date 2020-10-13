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

set(TARGET compositorclient)

option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")

message("Setup ${TARGET} v${VERSION}...")

if("${PLUGIN_COMPOSITOR_IMPLEMENTATION}" STREQUAL "Nexus")
    if (COMPOSITORCLIENT_IMPLEMENTATION_PATH)
        add_subdirectory(${COMPOSITORCLIENT_IMPLEMENTATION_PATH})
    else()
        include(GetExternalCode)
        set(COMPOSITORCLIENT_IMPLEMENTATION_VERSION "master" CACHE STRING "compositorclient implementation version")
        GetExternalCode(
            GIT_REPOSITORY "https://code.rdkcentral.com/r/soc/broadcom/components/rdkcentral/thunder/compositorclient"
            GIT_TAG ${COMPOSITORCLIENT_IMPLEMENTATION_VERSION}
            SOURCE_DIR "Nexus"
        )
        add_subdirectory(Nexus)
    endif()
else()
    add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})
endif()

install(
	FILES Client.h
        DESTINATION include/${NAMESPACE}/compositor
)
