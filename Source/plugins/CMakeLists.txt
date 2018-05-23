set(TARGET Plugins)

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

add_library(${TARGET} SHARED
        Channel.cpp
        Logging.cpp
        MetaData.cpp
        Module.cpp
        Service.cpp
        Shell.cpp
        StateControl.cpp
        SubSystem.cpp
        VirtualInput.cpp
        )

set(PUBLIC_HEADERS
        Channel.h
        Config.h
        Configuration.h
        IPlugin.h
        IRPCIterator.h
        IShell.h
        IStateControl.h
        ISubSystem.h
        Logging.h
        MetaData.h
        plugins.h
        Request.h
        Service.h
        VirtualInput.h
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
        OUTPUT_NAME  ${PLUGINS_ARTIFACT}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/plugins COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/plugins      # headers
)

HandleStandardLib()

# package config
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${PLUGINS_ARTIFACT}.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PLUGINS_ARTIFACT}.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGINS_ARTIFACT}.pc
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig"
)