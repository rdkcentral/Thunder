set(TARGET WPEFramework)

add_executable(${TARGET}
        Controller.cpp
        PluginHost.cpp
        PluginServer.cpp
        Probe.cpp
        SystemInfo.cpp
        )

target_compile_definitions(${TARGET}
        PRIVATE
          NAMESPACE=${NAMESPACE}
          APPLICATION_NAME=${TARGET}
          THREADPOOL_COUNT=3
          RPCPOOL_COUNT=3
        )

if (TREE_REFERENCE)
    target_compile_definitions(${TARGET}
            PRIVATE
              -DTREE_REFERENCE=${TREE_REFERENCE}
            )
endif()

target_link_libraries(${TARGET}
        PRIVATE
          ${CMAKE_DL_LIBS}
	  ${LIBEXECINFO_LIBRARY}
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Cryptalgo::${NAMESPACE}Cryptalgo
          ${NAMESPACE}Plugins::${NAMESPACE}Plugins
          ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess  # For COM executable define
        )

if (PROTOCOLS)
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TARGET}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

include(GenericConfig.cmake)

