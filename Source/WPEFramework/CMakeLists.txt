set(TARGET PluginHost)

add_executable(${TARGET}
        Controller.cpp
        PluginHost.cpp
        PluginServer.cpp
        Probe.cpp
        SystemInfo.cpp
        )

target_compile_definitions(${TARGET}
        PRIVATE
          INSTALL_COMMON_PREFIX=${NAMESPACE}
          APPLICATION_NAME=${PLUGINHOST_ARTIFACT}
        )

if (TREE_REFERENCE)
    target_compile_definitions(${TARGET}
            PRIVATE
              -DTREE_REFERENCE=${TREE_REFERENCE}
            )
endif()

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}::Core
          ${NAMESPACE}::Tracing
          ${NAMESPACE}::Cryptalgo
          ${NAMESPACE}::Plugins
          ${NAMESPACE}::COMProcess  # For COM executable define
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
        VERSION ${VERSION}
        OUTPUT_NAME ${PLUGINHOST_ARTIFACT}
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE} COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}      # headers
)
