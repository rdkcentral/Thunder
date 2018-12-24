set(TARGET Core)

option(WPEFRAMEWORK_DEADLOCK_DETECTION
        "Enable deadlock detection tooling." OFF)
option(WPEFRAMEWORK_WCHAR_SUPPORT
        "Enable support for WCHAR in ccpsdk." OFF)

find_package(Threads REQUIRED)
find_package(LIBRT)

add_library(${TARGET} SHARED
        CyclicBuffer.cpp
        DataElement.cpp
        DataElementFile.cpp
        FileSystem.cpp
        ISO639.cpp
        JSON.cpp
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
        XGetopt.cpp
        ResourceMonitor.cpp
        )

#TODO: Remove all non public headers from this list,
# All the interface headers are here, these will be installed to staging
set(PUBLIC_HEADERS
        ASN1.h
        Config.h
        core.h
        CyclicBuffer.h
        DataBuffer.h
        DataElementFile.h
        DataElement.h
        DataStorage.h
        Enumerate.h
        Factory.h
        FileSystem.h
        Frame.h
        IAction.h
        IIterator.h
        IObserver.h
        IPCChannel.h
        IPCConnector.h
        ISO639.h
        JSON.h
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
        Stream.h
        StreamJSON.h
        StreamText.h
        StreamTypeLengthValue.h
        Sync.h
        Synchronize.h
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
        Module.h
        )

#non-buildable interfce target to carry the definitions
add_library(Support INTERFACE)
add_library(${NAMESPACE}::Support ALIAS Support)
set(PC_FLAGS "-std=c++11")

if(${CONFIG_DIR} STREQUAL "debug")
    set(PC_FLAGS "${PC_FLAGS} -D_DEBUG -g0")
elseif(${CONFIG_DIR} STREQUAL "production")
    set(PC_FLAGS "${PC_FLAGS} -DNDEBUG -DPRODUCTION")
else()
    set(PC_FLAGS "${PC_FLAGS} -DNDEBUG")
endif()

if(NOT WPEFRAMEWORK_WCHAR_SUPPORT)
    set(PC_FLAGS "${PC_FLAGS} -D__NO_WCHAR_SUPPORT__")
    target_compile_definitions(Support INTERFACE __NO_WCHAR_SUPPORT__)
    message(STATUS "Disabled WCHAR support")
endif()

if(WPEFRAMEWORK_DEADLOCK_DETECTION)
    set(PC_FLAGS "${PC_FLAGS} -DCRITICAL_SECTION_LOCK_LOG")
    target_compile_definitions(Support INTERFACE CRITICAL_SECTION_LOCK_LOG)
    message(STATUS "Enabled deadlock detection.")
endif()

if (WPEFRAMEWORK_SYSTEM_PREFIX)
    target_compile_definitions(Support INTERFACE SYSTEM_PREFIX=${WPEFRAMEWORK_SYSTEM_PREFIX})
    message(STATUS "System prefix is set to: ${WPEFRAMEWORK_SYSTEM_PREFIX}")
endif()

find_package(BCMHost QUIET)

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}::Support
        PRIVATE
          ${CMAKE_DL_LIBS}
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
          OUTPUT_NAME ${CORE_ARTIFACT}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../> #core.h
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

if(LIBRT_FOUND)
    target_link_libraries(${TARGET}
            PRIVATE
              LIBRT::LIBRT
            )
endif()

# Hardware specific
if(${WPEFRAMEWORK_PLATFORM} STREQUAL "INTELCE")
    target_include_directories(${TARGET}
            PRIVATE
              ${INTELCE_OSAL_LIBRARIES}
              ${INTELCE_GDL_LIBRARIES})

    target_link_libraries(${TARGET}
            PRIVATE
              ${INTELCE_OSAL_INCLUDE_DIRS}
              ${INTELCE_GDL_INCLUDE_DIRS}
            )
endif()
find_package(BCMHost)
if (BCM_HOST_FOUND)
    target_compile_definitions(${TARGET}
            PRIVATE
             -DPLATFORM_RPI
            )
    target_include_directories(${TARGET}
            PRIVATE
              ${BCM_HOST_INCLUDE_DIRS})

    target_link_libraries(${TARGET}
            PRIVATE
            ${BCM_HOST_LIBRARIES}
            -lvchostif
            )
endif (BCM_HOST_FOUND)

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS Support ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/core COMPONENT devel   # for mac 
        INCLUDES DESTINATION include/${NAMESPACE}/core      # headers
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/core.pc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${CORE_ARTIFACT}.pc"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CORE_ARTIFACT}.pc
        DESTINATION "lib/pkgconfig")

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/core.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake/${NAMESPACE}"
)

HandleStandardLib()
