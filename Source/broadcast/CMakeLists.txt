set(TARGET Broadcast)

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
        Descriptors.h
        MPEGDescriptor.h
        MPEGSection.h
        MPEGTable.h
        ProgramTable.h
        TunerAdministrator.h
        Services.h
        Networks.h
        TimeDate.h
        Schedule.h
        NIT.h
        SDT.h
        TDT.h
        EIT.h
        Module.h
        )

set(PC_CFLAGS "")

find_package(Nexus REQUIRED)
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

# ===========================================================================================
# Install ARTIFACTS:
# ===========================================================================================
install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/broadcast COMPONENT devel # for mac
        INCLUDES DESTINATION include/${NAMESPACE}/broadcast # headers
)

# ===========================================================================================
# Install METADATA:
# ===========================================================================================
configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/broadcast.pc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${BROADCAST_ARTIFACT}.pc"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BROADCAST_ARTIFACT}.pc
        DESTINATION "lib/pkgconfig")

configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/broadcast.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake"
                @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Find${TARGET}.cmake
        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cmake/${NAMESPACE}"
)

HandleStandardLib()
