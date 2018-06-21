set(TARGET Interfaces)

add_library(${TARGET} SHARED
        ProxyStubs.cpp
        )

set(PUBLIC_HEADERS
        IBluetooth.h
        IBrowser.h
        ICapture.h
        ICommand.h
        IComposition.h
        IContentDecryption.h
        IDictionary.h
        IDRM.h
        IElement.h
        IGuide.h
        IIPNetwork.h
        IKeyHandler.h
        IMemory.h
        INetflix.h
        IPower.h
        IProvisioning.h
        IResourceCenter.h
        IRPCLink.h
        IStreaming.h
        ISwitchBoard.h
        ITimeSync.h
        ITVPlatform.h
        IVoiceHandler.h
        IWebDriver.h
        IWebServer.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}::Core
          ${NAMESPACE}::Tracing
          ${NAMESPACE}::Cryptalgo
        )

if (WPEFRAMEWORK_PROTOCOLS)
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}::Protocols
            )
else()
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}::WebSocket
              ${NAMESPACE}::COM
            )
endif ()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${PLUGINS_INTERFACES_ARTIFACT}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/wpeframework/proxystubs COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/wpeframework/proxystubs COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/wpeframework/proxystubs COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/interfaces COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/interfaces      # headers
)

HandleStandardLib()
