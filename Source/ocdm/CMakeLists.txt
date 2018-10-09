set(TARGET ocdm)

add_library(${TARGET} SHARED
        open_cdm.cpp
        ProxyStubs.cpp
        )

set(PUBLIC_HEADERS
        DataExchange.h
        IOCDM.h
        open_cdm.h
        Module.h
        )

target_link_libraries(${TARGET}
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
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${CDMI_ARTIFACT}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/ocdm COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/ocdm      # headers
)

#FIXME: This needs to become a sysmlink.
install(
        FILES ${PUBLIC_HEADERS}
        DESTINATION include/opencdm
)

HandleStandardLib()

set (CFLAGS_VAR "-I${CMAKE_INSTALL_PREFIX}/include/opencdm")
set (LIBS_VAR "-l${TARGET}")

if (WPEFRAMEWORK_CDMI_BCM_NEXUS_SVP)
    set(CFLAGS_VAR "${CFLAGS_VAR} -I${CMAKE_INSTALL_PREFIX}/include/refsw -DUSE_OPENCDM_BCM_NEXUS_SVP ")
    set(LIBS_VAR "${LIBS_VAR} -lb_secbuf -lbrcmsvpmeta")
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/ocdm.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ocdm.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/ocdm.pc
        DESTINATION lib/pkgconfig
)
