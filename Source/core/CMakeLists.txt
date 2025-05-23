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

set(TARGET ${NAMESPACE}Core)
include(CompileSettings)

find_package(Threads REQUIRED)
find_package(ExecInfo)
find_package(LIBRT)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/core/Version.h" @ONLY)
message(VERBOSE "Generated ${CMAKE_CURRENT_BINARY_DIR}/generated/core/Version.h")

add_library(${TARGET}
        Module.cpp
        DoorBell.cpp
        CyclicBuffer.cpp
        DataElement.cpp
        DataElementFile.cpp
        FileSystem.cpp
        ISO639.cpp
        JSON.cpp
        JSONRPC.cpp
        Library.cpp
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
        AssertionControl.h
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
        FileObserver.h
        Frame.h
        IAction.h
        IAssertionControl.h
        IIterator.h
        IObserver.h
        IPCMessage.h
        IPFrame.h
        IPCChannel.h
        IPCConnector.h
        ISO639.h
        IWarningReportingControl.h
        JSON.h
        JSONRPC.h
        KeyValue.h
        Library.h
        Link.h
        LockableContainer.h
        Measurement.h
        MACAddress.h
        Media.h
        MessageException.h
        Messaging.h
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
        TextStreamRedirectType.h
        Thread.h
        ThreadPool.h
        Time.h
        Timer.h
        Trace.h
        TriState.h
        TypeTraits.h
        ValueRecorder.h
        XGetopt.h
        WorkerPool.h
        WarningReportingControl.h
        WarningReportingCategories.h
        Module.h
        CallsignTLS.h
        TokenizedStringList.h
        MessageStore.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/core/Version.h
        )

target_compile_options (${TARGET} PRIVATE -Wno-psabi)

# non-buildable interface target to carry the definitions
# All build flags that are applicable to the whole system should be set here in the 
# core since the core is included everywhere so if they are set here, they are 
# picked up everywhere!
if(DISABLE_TRACING OR NOT ENABLED_TRACING_LEVEL)
    target_compile_definitions(${TARGET} PUBLIC _TRACE_LEVEL=0)
    message(STATUS "Force trace level to 0")
else()
    if(ENABLED_TRACING_LEVEL)
        target_compile_definitions(${TARGET} PUBLIC _TRACE_LEVEL=${ENABLED_TRACING_LEVEL})
        message(STATUS "Force trace level to ${ENABLED_TRACING_LEVEL}")
    endif()
endif()

if(MESSAGING)
    target_compile_definitions(${TARGET} PUBLIC __CORE_MESSAGING__)
    target_sources(${TARGET} PRIVATE Messaging.cpp MessageStore.cpp AssertionControl.cpp)
    message(STATUS "Messaging enabled")
endif()

if(WARNING_REPORTING)
    target_sources(${TARGET} PRIVATE WarningReportingControl.cpp WarningReportingCategories.cpp)
    target_compile_definitions(${TARGET} PUBLIC __CORE_WARNING_REPORTING__)
    message(STATUS "Warning reporting enabled")
endif()

if(EXCEPTION_CATCHING)
    target_compile_definitions(${TARGET} PUBLIC __CORE_EXCEPTION_CATCHING__)
    set_source_files_properties( Thread.cpp PROPERTIES COMPILE_FLAGS "-fexceptions" )
    message(STATUS "Unhandled Exception catching enabled")
endif()

if(WARNING_REPORTING OR EXCEPTION_CATCHING)
    target_sources(${TARGET} PRIVATE CallsignTLS.cpp)
endif()

if(DEADLOCK_DETECTION)
    target_compile_definitions(${TARGET} PUBLIC __CORE_CRITICAL_SECTION_LOG__)
    message(STATUS "Enabled deadlock detection.")
endif()

if(NOT WCHAR_SUPPORT)
    target_compile_definitions(${TARGET} PUBLIC __CORE_NO_WCHAR_SUPPORT__)
    message(STATUS "Disabled WCHAR support.")
endif()

if (INSTANCE_ID_BITS)
    target_compile_definitions(${TARGET} PUBLIC __CORE_INSTANCE_BITS__=${INSTANCE_ID_BITS})
    message(STATUS "COMRPC instanceid defined as ${INSTANCE_ID_BITS} bits.")
endif()

if(BLUETOOTH_SUPPORT)
    target_compile_definitions(${TARGET} PUBLIC __CORE_BLUETOOTH_SUPPORT__)

    include(GetBluez5Headers)

    GetBluez5UtilHeadersFiles(Bluez5UtilHeadersFiles)

    install(FILES ${Bluez5UtilHeadersFiles}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/core/bluez5 COMPONENT ${NAMESPACE}_Development)

    GetBluez5IncludeDirs(Bluez5IncludeDirs)

    foreach(path ${Bluez5IncludeDirs})
    target_include_directories(${TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${path}>)        
    endforeach()

    CheckBluez5InclusiveLanguage(INCLUSIVE_LANGUAGE)
    
    if(NOT ${INCLUSIVE_LANGUAGE})
        #TODO: Remove NO_INCLUSIVE_LANGUAGE define when the plugins are synced to BLUEZ_HAS_NO_INCLUSIVE_LANGUAGE
        target_compile_definitions(${TARGET} PUBLIC INTERFACE NO_INCLUSIVE_LANGUAGE BLUEZ_HAS_NO_INCLUSIVE_LANGUAGE)
    endif()

    message(STATUS "Enable bluetooth support.")
endif()

# ==================================================================================

target_compile_definitions(${TARGET} PRIVATE CORE_EXPORTS)

if(BUILD_TESTS)
    target_compile_definitions(${TARGET} PRIVATE BUILD_TESTS)
endif()

target_link_libraries(${TARGET}
        PRIVATE
          CompileSettings::CompileSettings
          CompileSettingsDebug::CompileSettingsDebug
          ${CMAKE_DL_LIBS}
          Threads::Threads
        )

set_target_properties(${TARGET} PROPERTIES
          CXX_STANDARD 11
          CXX_STANDARD_REQUIRED YES
          FRAMEWORK FALSE
          PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
          SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

if (NOT BUILD_SHARED_LIBS)
    set_property(TARGET  ${TARGET}
            APPEND PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON
    )
endif()

if (APPLE)
    target_compile_definitions( ${TARGET} PRIVATE __APPLE_USE_RFC_3542)
endif()

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/core> 
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../> #core.h
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

if(LIBRT_FOUND)
    target_link_libraries(${TARGET}
            PRIVATE
              LIBRT::LIBRT
            )
endif()

if(ExecInfo_FOUND)
    target_link_libraries(${TARGET}
            PRIVATE
              ${LIBEXECINFO_LIBRARY}
            )
endif()

find_package(Systemd)
if(SYSTEMD_FOUND)
    target_link_libraries( ${TARGET}
        PRIVATE
        Systemd::Systemd
    )
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DSYSTEMD_FOUND")
endif()

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS CompileSettings ${TARGET}  EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/core COMPONENT ${NAMESPACE}_Development   
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
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
