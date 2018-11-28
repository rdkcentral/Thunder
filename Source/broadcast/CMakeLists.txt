set(TARGET Broadcast)

find_package(Nexus REQUIRED)

option(WPEFRAMEWORK_BROADCAST_SI_PARSING
        "Add SI parsing and collecting functionality." OFF)

set(SOURCE_FILES
        ProgramTable.cpp
        Definitions.cpp
        TunerAdministrator.cpp
        Module.cpp
        implementations/NexusTuner.cpp
        )

set(PUBLIC_HEADERS
        broadcast.h
        Definitions.h
        MPEGDescriptor.h
        MPEGSection.h
        MPEGTable.h
        ProgramTable.h
        TunerAdministrator.h
        ServiceAdministrator.h
        NIT.h
        SDT.h
        Module.h
        )

set (CFLAGS_VAR "-I${CMAKE_INSTALL_PREFIX}/include/WPEFramework")
set (LIBS_VAR "-l${BROADCAST_ARTIFACT}")

if(WPEFRAMEWORK_BROADCAST_SI_PARSING)
    target_compile_definitions(Support INTERFACE INCLUDE_SI_PARSING)
    set(SOURCE_FILES ${SOURCE_FILES} 
        ServiceAdministrator.cpp
    )
    set(CFLAGS_VAR "${CFLAGS_VAR} -DINCLUDE_SI_PARSING")
    message(STATUS "Enabled SI Parsing inclusion.")
endif()

add_library(${TARGET} SHARED ${SOURCE_FILES})

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

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/broadcast.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/broadcast.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/broadcast.pc
        DESTINATION lib/pkgconfig
)

