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

set(TARGET gstreamerclient)

option(GSTREAMER_CLIENT_IMPLEMENTATION "Defines which implementation is used." "None")

message("Setup ${TARGET} v${VERSION}...")

if (NOT "${GSTREAMER_CLIENT_IMPLEMENTATION}" STREQUAL "None")

	message("GstreamerClient impl. for ${GSTREAMER_CLIENT_IMPLEMENTATION}")
	find_package(GSTREAMER REQUIRED)
	find_package(GSTREAMER_BASE REQUIRED)

	add_subdirectory (${GSTREAMER_CLIENT_IMPLEMENTATION})
endif()

install(
	FILES gstreamerclient.h
        DESTINATION include/${NAMESPACE}/gstreamerclient/
)
