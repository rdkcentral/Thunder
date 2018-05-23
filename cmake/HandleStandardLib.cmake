include (CMakePackageConfigHelpers)

macro(HandleStandardLib)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion
        )

string(REPLACE ${NAMESPACE} "" TARGET_STRIPED ${TARGET})

add_library(${NAMESPACE}::${TARGET_STRIPED} ALIAS ${TARGET})

install(EXPORT ${TARGET}Targets
        FILE ${TARGET}Config.cmake
        NAMESPACE ${NAMESPACE}::
        DESTINATION include/cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}ConfigVersion.cmake
        DESTINATION include/cmake )
endmacro()