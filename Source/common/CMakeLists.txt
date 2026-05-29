set(TARGET ${NAMESPACE}Common)

add_library(${TARGET}
        Channel.cpp
        JSONRPC.cpp
        Metadata.cpp
        Module.cpp
        Service.cpp
        Shell.cpp
        StateControl.cpp
        SubSystem.cpp
        VirtualInput.cpp
        System.cpp
        )

set(PUBLIC_HEADERS
        Channel.h
        common.h
        Config.h
        Configuration.h
        JSONRPC.h
        Metadata.h
        Request.h
        Service.h
        StateControl.h
        SubSystem.h
        System.h
        VirtualInput.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}Plugins::${NAMESPACE}Plugins
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}Messaging::${NAMESPACE}Messaging
          ${NAMESPACE}COM::${NAMESPACE}COM
          ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
        PRIVATE
          CompileSettingsDebug::CompileSettingsDebug
        )

if (WARNING_REPORTING)
    target_link_libraries(${TARGET}
           PUBLIC
              ${NAMESPACE}WarningReporting::${NAMESPACE}WarningReporting
            )
endif()

target_include_directories(${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../plugins>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/plugins>
        )

target_compile_definitions(${TARGET} PRIVATE COMMON_EXPORTS)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD ${CXX_STD}
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}"
        SOVERSION ${VERSION_MAJOR}
        )

if(HUMAN_VERSIONED_BINARIES)
set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        )
endif()

install(
        TARGETS ${TARGET} EXPORT ${TARGET}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${NAMESPACE}_Runtime NAMELINK_COMPONENT ${NAMESPACE}_Development
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Runtime
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/common COMPONENT ${NAMESPACE}_Development
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "Basic library with common plugin host implementation support.")

InstallCMakeConfig(TARGETS ${TARGET})
