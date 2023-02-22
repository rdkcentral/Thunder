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

option(LOCALTRACER
        "Include local tracing support library in the build." OFF)
option(HIBERNATESUPPORT
        "Include hibernate support in the build." OFF)
option(PRIVILEGEDREQUEST
        "Include hthe privileged request support in the build." OFF)
option(PROCESSCONTAINERS
        "Include containerization in the build." OFF)
option(WARNING_REPORTING
        "Include warning reporting in the build." OFF)

if(LOCALTRACER)
    add_subdirectory(localtracer)
endif()

if(HIBERNATESUPPORT)
    add_subdirectory(hibernate)
endif()

if(PRIVILEGEDREQUEST)
    add_subdirectory(privilegedrequest)
endif()

if(PROCESSCONTAINERS)
    add_subdirectory(processcontainers)
endif()

if(WARNING_REPORTING)
    add_subdirectory(warningreporting)
endif()

