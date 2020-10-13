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
include(GetExternalCode)

set(CRYPTOGRAPHY_IMPLEMENTATION "None" CACHE STRING "Defines what implementation to use")

if(${CRYPTOGRAPHY_IMPLEMENTATION} STREQUAL "Nexus")
    if (CRYPTOGRAPHY_IMPLEMENTATION_PATH)
        add_subdirectory(${CRYPTOGRAPHY_IMPLEMENTATION_PATH})
    else()
        include(GetExternalCode)
        GetExternalCode(
            GIT_REPOSITORY "git@github.com:WebPlatformForEmbedded/ICrypto-Nexus.git"
            GIT_TAG "master"
            SOURCE_DIR "Nexus"
        )
        add_subdirectory(Nexus)
    endif()
elseif (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CRYPTOGRAPHY_IMPLEMENTATION})
    add_subdirectory(${CRYPTOGRAPHY_IMPLEMENTATION})
else ()
    message(FATAL_ERROR "Invalid value \'${CRYPTOGRAPHY_IMPLEMENTATION}\' for CRYPTOGRAPHY_IMPLEMENTATION.")
endif()
