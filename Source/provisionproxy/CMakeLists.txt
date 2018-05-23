set(TARGET ProvisionProxy)

find_package(OpenSSL REQUIRED)
find_package(libprovision REQUIRED)

# Construct a library object
add_library(${TARGET} STATIC
        ipclink.cpp
        )

set(PUBLIC_HEADERS
        AccesProvision.h
        IPCProvision.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}::Core
          Metrological::libprovision
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${PROVISIONPROXY_ARTIFACT}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/provision COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/provision      # headers
)

HandleStandardLib()


configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/provisionproxy.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/provisionproxy.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/provisionproxy.pc
        DESTINATION lib/pkgconfig
)