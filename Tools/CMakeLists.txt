cmake_minimum_required(VERSION 3.3)

project(WPEFrameworkGenerators)

set(GENERATOR_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/sbin")

message("CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH}, CMAKE_SYSTEM_LIBRARY_PATH ${CMAKE_SYSTEM_LIBRARY_PATH}, CMAKE_SYSTEM_INCLUDE_PATH ${CMAKE_SYSTEM_INCLUDE_PATH} and CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}")

if(NOT DEFINED GENERIC_CMAKE_MODULE_PATH)
   find_path(GENERIC_CMAKE_MODULE_PATH CMake.cmake
       PATHS
           "/usr/share/cmake/Modules/"
           "/usr/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules/"
   )
endif()

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindProxyStubGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindProxyStubGenerator.cmake"
                @ONLY
)

configure_file( "${CMAKE_SOURCE_DIR}/cmake/FindJsonGenerator.cmake.in"
                "${CMAKE_CURRENT_BINARY_DIR}/FindJsonGenerator.cmake"
                @ONLY
)

install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/FindProxyStubGenerator.cmake"
    DESTINATION ${GENERIC_CMAKE_MODULE_PATH}
)

install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/FindJsonGenerator.cmake"
    DESTINATION ${GENERIC_CMAKE_MODULE_PATH}
)

install(DIRECTORY 
        "${CMAKE_SOURCE_DIR}/ProxyStubGenerator"
        "${CMAKE_SOURCE_DIR}/JsonGenerator"
    DESTINATION ${GENERATOR_INSTALL_PATH}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
