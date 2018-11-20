set(TARGET Broadcast)

find_package(Nexus REQUIRED)

add_library(${TARGET} SHARED
        ProgramTable.cpp
        NexusTuner.cpp
        Module.cpp
        )

set(PUBLIC_HEADERS
        broadcast.h
        Definitions.h
        MPEGDescriptor.h
        MPEGSection.h
        MPEGTable.h
        ProgramTable.h
        NIT.h
        SDT.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}::Core
          NEXUS::NEXUS
          NEXUS::NXCLIENT
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${BROADCAST_ARTIFACT}
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
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/broadcast COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/broadcast # headers
)

HandleStandardLib()

set (CFLAGS_VAR "-I${CMAKE_INSTALL_PREFIX}/include/WPEFramework")
set (LIBS_VAR "-l${BROADCAST_ARTIFACT}")

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/broadcast.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/broadcast.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/broadcast.pc
        DESTINATION lib/pkgconfig
)

