set(TARGET Plugins)

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

add_library(${TARGET} SHARED
        Channel.cpp
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

if(WPEFRAMEWORK_PROTOCOLS)
    set(PROTOCOL_WRAPPER "-l${PROTOCOLS_ARTIFACT}")
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}::Protocols
            )
else()
    set(PROTOCOL_WRAPPER "-l${COM_ARTIFACT} -l{WEBSOCKET_ARTIFACT}")
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}::WebSocket
              ${NAMESPACE}::COM
            )
endif()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME  ${PLUGINS_ARTIFACT}
        )

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/plugins COMPONENT devel # for mac
        INCLUDES DESTINATION include/${NAMESPACE}/plugins      # headers
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/plugins.pc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${PLUGINS_ARTIFACT}.pc"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUGINS_ARTIFACT}.pc
        DESTINATION "lib/pkgconfig")

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/plugins.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake/${NAMESPACE}"
)

HandleStandardLib()
