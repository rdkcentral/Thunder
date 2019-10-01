set(TARGET  ${NAMESPACE}ProcessContainers)

# Construct a library object
add_library(${TARGET} SHARED
        ProcessContainer.cpp
        LXCImplementation.cpp
        Module.cpp
        )

set(PUBLIC_HEADERS
        ProcessContainer.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
        ${NAMESPACE}Core::${NAMESPACE}Core
        )

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
          $<INSTALL_INTERFACE:include>
        )

find_package(LXC REQUIRED)

target_link_libraries(${TARGET}
        PRIVATE
        LXC::LXC
        CompileSettingsDebug::CompileSettingsDebug
        )

target_include_directories( ${TARGET}
        PRIVATE
        LXC::LXC
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/processcontainers COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/processcontainers    # headers
)

InstallCMakeConfig(
        TARGETS ${TARGET})

InstallPackageConfig(
        TARGETS ${TARGET} 
        DESCRIPTION "Enable ease of development for Process Container support." )
