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

set(TARGET ${NAMESPACE}Core)
include(CompileSettings)

option(DEADLOCK_DETECTION 
        "Enable deadlock detection tooling." OFF)
option(WCHAR_SUPPORT 
        "Enable support for WCHAR." OFF)
option(DISABLE_TRACING 
        "Disable tracing in debug" OFF)
option(BLUETOOTH
        "Enable support for Bluetooth in the core." OFF)

find_package(Threads REQUIRED)


add_library(${TARGET} SHARED
	DoorBell.cpp
        CyclicBuffer.cpp
        DataElement.cpp
        DataElementFile.cpp
        FileSystem.cpp
        ISO639.cpp
        JSON.cpp
        JSONRPC.cpp
        Library.cpp
        MessageException.cpp
        Netlink.cpp
        NetworkInfo.cpp
        NodeId.cpp
        Number.cpp
        Parser.cpp
        Portability.cpp
        ProcessInfo.cpp
        SerialPort.cpp
        Serialization.cpp
        Services.cpp
        SharedBuffer.cpp
        Singleton.cpp
        SocketPort.cpp
        Sync.cpp
        SystemInfo.cpp
        TextReader.cpp
        Thread.cpp
        Time.cpp
        Trace.cpp
        WorkerPool.cpp
        XGetopt.cpp
        ResourceMonitor.cpp
        )

#TODO: Remove all non public headers from this list,
# All the interface headers are here, these will be installed to staging
set(PUBLIC_HEADERS
        ASN1.h
        DoorBell.h
        Config.h
        core.h
        CyclicBuffer.h
        DataBuffer.h
        DataElementFile.h
        DataElement.h
        Enumerate.h
        Factory.h
        FileSystem.h
        Frame.h
        IAction.h
        IIterator.h
        IObserver.h
        IPCMessage.h
        IPCChannel.h
        IPCConnector.h
        ISO639.h
        JSON.h
        JSONRPC.h
        KeyValue.h
        Library.h
        Link.h
        LockableContainer.h
        Measurement.h
        Media.h
        MessageException.h
        Module.h
        Netlink.h
        NetworkInfo.h
        NodeId.h
        Number.h
        Optional.h
        Parser.h
        Portability.h
        Process.h
        ProcessInfo.h
        Proxy.h
        Queue.h
        Range.h
        ReadWriteLock.h
        Rectangle.h
        RequestResponse.h
        ResourceMonitor.h
        Serialization.h
        SerialPort.h
        Services.h
        SharedBuffer.h
        Singleton.h
        SocketPort.h
        SocketServer.h
        StateTrigger.h
        StopWatch.h
        Stream.h
        StreamJSON.h
        StreamText.h
        StreamTypeLengthValue.h
        Sync.h
        Synchronize.h
        SynchronousChannel.h
        SystemInfo.h
        TextFragment.h
        TextReader.h
        Thread.h
        Time.h
        Timer.h
        Trace.h
        TriState.h
        TypeTraits.h
        ValueRecorder.h
        XGetopt.h
        WorkerPool.h
        Module.h
        )

#non-buildable interfce target to carry the definitions
if(DISABLE_TRACING)
    target_compile_definitions(${TARGET} PUBLIC _TRACE_LEVEL=0)
    message(STATUS "Force trace level to 0")
endif()

if(BUILD_TESTS)
    target_compile_definitions(${TARGET} PRIVATE BUILD_TESTS)
endif()

if(NOT WCHAR_SUPPORT)
    target_compile_definitions(${TARGET} PUBLIC __NO_WCHAR_SUPPORT__)
    message(STATUS "Disabled WCHAR support.")
endif()

if (BLUETOOTH)
    target_compile_definitions(${TARGET} PUBLIC CORE_BLUETOOTH)
    message(STATUS "Enable Bluetooth support.")
endif()

if(DEADLOCK_DETECTION)
    target_compile_definitions(${TARGET} PUBLIC CRITICAL_SECTION_LOCK_LOG)
    message(STATUS "Enabled deadlock detection.")
endif()

target_link_libraries(${TARGET}
        PUBLIC
          CompileSettings::CompileSettings
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
          ${CMAKE_DL_LIBS}
	  ${LIBEXECINFO_LIBRARY}
          LIBRT::LIBRT
          Threads::Threads
        )

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
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../> #core.h
          $<INSTALL_INTERFACE:include/${NAMESPACE}>)

find_package(LIBRT)
if(LIBRT_FOUND)
    target_link_libraries(${TARGET}
            PRIVATE
              LIBRT::LIBRT
            )
endif()

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS CompileSettings ${TARGET}  EXPORT ${TARGET}Targets # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/core COMPONENT devel   # for mac 
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Abstraction layer basis for all core functionality"
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallCMakeConfig(
        TARGETS CompileSettingsDebug)
