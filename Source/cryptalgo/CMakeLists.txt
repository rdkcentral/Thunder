# If not stated otherwise in this file or this component's LICENSE file the
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

set(TARGET  ${NAMESPACE}Cryptalgo)

add_library(${TARGET}
        Module.cpp
        AES.cpp
        AESImplementation.cpp
        Hash.cpp
        Random.cpp
        )

set(PUBLIC_HEADERS
        AES.h
        AESImplementation.h
        cryptalgo.h
        Hash.h
        HashStream.h
        HMAC.h
        Random.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          CompileSettingsDebug::CompileSettingsDebug
        )

target_compile_definitions(${TARGET} PRIVATE CRYPTALGO_EXPORTS)

if(SECURE_SOCKET)
    find_package(OpenSSL REQUIRED)

    if (OPENSSL_FOUND)
        include_directories(${OPENSSL_INCLUDE_DIR})
        target_sources(${TARGET}
            PRIVATE
                SecureSocketPort.cpp)
        target_link_libraries(${TARGET}
            PRIVATE
                OpenSSL::SSL)
        target_compile_definitions(${TARGET}
            PUBLIC
                SECURESOCKETS_ENABLED=1)

        list(APPEND PUBLIC_HEADERS "SecureSocketPort.h")
    endif()
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/cryptalgo COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

InstallCMakeConfig(TARGETS ${TARGET})
