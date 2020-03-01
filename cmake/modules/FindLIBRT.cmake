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

## clock-monotonic, just see if we need to link with rt
set(CMAKE_REQUIRED_LIBRARIES_SAVE ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES rt)
CHECK_SYMBOL_EXISTS(_POSIX_TIMERS "unistd.h;time.h" POSIX_TIMERS_FOUND)
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_SAVE})

if(POSIX_TIMERS_FOUND)
    find_library(LIBRT_LIBRARY NAMES rt)

    if(NOT TARGET LIBRT::LIBRT)
        add_library(LIBRT::LIBRT UNKNOWN IMPORTED)
        if(EXISTS "${LIBRT_LIBRARY}")
            set_target_properties(LIBRT::LIBRT PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                    IMPORTED_LOCATION "${LIBRT_LIBRARY}")
        endif()
    endif()

    mark_as_advanced(LIBRT_LIBRARY)
endif()
