set(TARGET RPCProcess)

add_executable(${TARGET}
        Process.cpp
        )

# Non-buildable interface target to carry the definitions for users
add_library(COMProcess INTERFACE)
add_library(${NAMESPACE}::COMProcess ALIAS COMProcess)
target_compile_definitions(COMProcess
        INTERFACE
          ARTIFACT_COMPROCESS=${WPEPROCESS_ARTIFACT}
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}::COMProcess
        PRIVATE
          ${NAMESPACE}::Core
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
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${WPEPROCESS_ARTIFACT}
        )

install(
        TARGETS COMProcess ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE} COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}      # headers
)
