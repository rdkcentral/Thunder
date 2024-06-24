project(LocalTracer
        VERSION 0.0.1
        DESCRIPTION "Library to get tracing locally from e.g. your test code on the stdout."
        LANGUAGES CXX)

cmake_minimum_required(VERSION 3.15)

set(MODULE_NAME ${NAMESPACE}LocalTracer)

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

option(EXAMPLE_LOCALTRACER "Include an example app." OFF)

add_library(${MODULE_NAME} INTERFACE)

target_link_libraries(${MODULE_NAME} INTERFACE
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

target_include_directories(${MODULE_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}>)

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_11)

install(TARGETS ${MODULE_NAME} EXPORT ${MODULE_NAME}Targets)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/localtracer/localtracer.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/localtracer COMPONENT ${NAMESPACE}_Development)

include(HeaderOnlyInstall)

HeaderOnlyInstallPackageConfig(TARGET ${MODULE_NAME}
    NAME ${MODULE_NAME}
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Header only library to locally print traces coming from Messaging without the need of running Thunder.")

HeaderOnlyInstallCMakeConfig(TARGET ${MODULE_NAME} TREAT_AS_NORMAL)

add_subdirectory(example)
