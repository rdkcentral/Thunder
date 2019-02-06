
# ${TARGET} mentioned here is the name to be used by other CMake projects.
add_library(Marshalling SHARED
        ProxyStubs.cpp
        )

add_library(Interfaces SHARED
        Enumerations.cpp
        )

set(PUBLIC_HEADERS
        IAVNClient.h
        IBluetooth.h
        IBrowser.h
        ICapture.h
        ICommand.h
        IComposition.h
        IContentDecryption.h
        IDictionary.h
        IDRM.h
        IDsgccClient.h
        IExternal.h
        IGuide.h
        IIPNetwork.h
        IKeyHandler.h
        IMemory.h
        INetflix.h
        IPlayGiga.h
        IPower.h
        IProvisioning.h
        IRPCLink.h
        IStreaming.h
        ISwitchBoard.h
        ITimeSync.h
        ITVControl.h
        ITVPlatform.h
        IVoiceHandler.h
        IRtspClient.h
        IWebDriver.h
        IWebServer.h
        Module.h
        )

target_link_libraries(Marshalling
        PRIVATE
          ${NAMESPACE}::Core
          ${NAMESPACE}::Tracing
        )

target_link_libraries(Interfaces
        PRIVATE
          ${NAMESPACE}::Core
          ${NAMESPACE}::Tracing
        )

if (WPEFRAMEWORK_PROTOCOLS)
    target_link_libraries(Marshalling
            PRIVATE
              ${NAMESPACE}::Protocols
            )
else()
    target_link_libraries(Marshalling
            PRIVATE
              ${NAMESPACE}::WebSocket
              ${NAMESPACE}::COM
            )
endif ()

set_target_properties(Marshalling PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME "${NAMESPACE}Marshalling"
        )

set_target_properties(Interfaces PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        OUTPUT_NAME "${NAMESPACE}Interfaces"
        )


install(
        TARGETS Interfaces EXPORT InterfacesTargets  # for downstream dependencies
        ARCHIVE DESTINATION lib/ COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/ COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/ COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/interfaces COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/interfaces      # headers
)

install(
        TARGETS Marshalling EXPORT MarshallingTargets  # for downstream dependencies
        LIBRARY DESTINATION lib/wpeframework/proxystubs COMPONENT libs      # shared lib
)

# package config
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/interfaces.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}Interfaces.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${NAMESPACE}Interfaces.pc
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig"
)

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/interfaces.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindInterfaces.cmake"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FindInterfaces.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake/${NAMESPACE}"
)

include (CMakePackageConfigHelpers)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/InterfacesConfigVersion.cmake
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion
        )

add_library(${NAMESPACE}::Interfaces ALIAS Interfaces)

install(EXPORT InterfacesTargets
        FILE InterfacesConfig.cmake
        NAMESPACE ${NAMESPACE}::
        DESTINATION include/cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/InterfacesConfigVersion.cmake
        DESTINATION include/cmake )
