set(TARGET ${NAMESPACE}Broadcast)

find_package(NXCLIENT QUIET)

add_library(${TARGET} SHARED 
        ProgramTable.cpp
        Definitions.cpp
        TunerAdministrator.cpp
        Module.cpp
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

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
        )


if(NXCLIENT_FOUND)
    find_package(NEXUS REQUIRED)

     target_sources(${TARGET} PRIVATE implementations/NexusTuner.cpp)

     target_link_libraries(${TARGET}
        PRIVATE
            NEXUS::NEXUS
            NXCLIENT::NXCLIENT
        )
else()
    target_sources(${TARGET} PRIVATE implementations/LinuxDVBTuner.cpp)
endif()


set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
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
InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "Broadcast library" )

InstallCMakeConfig(
        TARGETS ${TARGET})


add_subdirectory(test)
