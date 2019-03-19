set(TargetMarshalling ${NAMESPACE}Marshallings)
set(TargetInterfaces ${NAMESPACE}Interfaces)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

add_library(${TargetMarshalling} SHARED
        ProxyStubs.cpp
        ProxyStubs_AVNClient.cpp
        ProxyStubs_Browser.cpp
        ProxyStubs_Composition.cpp
        ProxyStubs_ContentDecryption.cpp
        ProxyStubs_DsgccClient.cpp
        ProxyStubs_Guide.cpp
        ProxyStubs_Netflix.cpp
        ProxyStubs_Packager.cpp
        ProxyStubs_PlayGiga.cpp
        ProxyStubs_Provisioning.cpp
        ProxyStubs_RPCLink.cpp
        ProxyStubs_RtspClient.cpp
        ProxyStubs_Streaming.cpp
        ProxyStubs_TestController.cpp
        ProxyStubs_TVControl.cpp
        ProxyStubs_WebDriver.cpp
        ProxyStubs_WebServer.cpp
        ProxyStubs_TestUtility.cpp
        )

add_library(${TargetInterfaces} SHARED
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
        Ids.h
        IDictionary.h
        IDRM.h
        IDsgccClient.h
        IExternal.h
        IGuide.h
        IIPNetwork.h
        IKeyHandler.h
        IMemory.h
        INetflix.h
        IPackager.h
        IPlayGiga.h
        IPower.h
        IProvisioning.h
        IRPCLink.h
        IStreaming.h
        ISwitchBoard.h
        ISystemCommands.h
        ITimeSync.h
        ITVControl.h
        ITVPlatform.h
        IVoiceHandler.h
        IRtspClient.h
        IWebDriver.h
        IWebServer.h
        ITestController.h
        ITestUtility.h
        Module.h
        )

target_link_libraries(${TargetMarshalling}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        )

target_link_libraries(${TargetInterfaces}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        )

if (PROTOCOLS)
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

set_target_properties(${TargetMarshalling} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

set_target_properties(${TargetInterfaces} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        )

target_include_directories(${TargetInterfaces}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TargetInterfaces} EXPORT ${TargetInterfaces}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/ COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/ COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/ COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/interfaces COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

install(
        TARGETS ${TargetMarshalling} EXPORT ${TargetMarshalling}Targets  # for downstream dependencies
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
)

InstallPackageConfig(
        TARGETS ${TargetInterfaces}
        DESCRIPTION "Share lookup tables associated with interface enums."
        )

InstallCMakeConfig(TARGETS ${TargetInterfaces})
