set(TARGET WPEProcess)

add_executable(${TARGET}
        Process.cpp
        )

# Non-buildable interface target to carry the definitions for users
add_library(COMProcess INTERFACE)
add_library(${NAMESPACE}COMProcess::${NAMESPACE}COMProcess ALIAS COMProcess)
target_compile_definitions(COMProcess
        INTERFACE
          HOSTING_COMPROCESS=${TARGET}
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}COMProcess::${NAMESPACE}COMProcess
        PRIVATE
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
          ${NAMESPACE}Core::${NAMESPACE}Core
        )

if (WPEFRAMEWORK_PROTOCOLS)
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
        SOVERSION ${VERSION_MAJOR}
        )

install(
        TARGETS COMProcess ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)
