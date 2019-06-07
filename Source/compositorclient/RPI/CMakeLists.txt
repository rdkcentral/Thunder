set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

find_package(EGL REQUIRED)

add_library(${TARGET} SHARED Implementation.cpp)

target_link_libraries(${TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        ${EGL_LIBRARIES}
)

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
)

target_include_directories(${TARGET}
    PUBLIC
        $<INSTALL_INTERFACE:include/${NAMESPACE}>
    PRIVATE
        ${EGL_INCLUDE_DIRS})

target_compile_definitions(${TARGET}
    PRIVATE
        ${EGL_DEFINITIONS})

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
)

InstallPackageConfig(
        TARGETS ${TARGET}
        DESCRIPTION "A compositor implementation wrapper for RPI and key handling.")

InstallCMakeConfig(TARGETS ${TARGET})
