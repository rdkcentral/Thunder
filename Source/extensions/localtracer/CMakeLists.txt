project(LocalTracer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.7)

set(MODULE_NAME ${NAMESPACE}LocalTracer)

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

option(EXAMPLE_LOCALTRACER "Include an example app." OFF)

add_library(${MODULE_NAME} INTERFACE)

target_link_libraries(${MODULE_NAME} INTERFACE
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}Messaging::${NAMESPACE}Messaging)

target_include_directories(${MODULE_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include/${NAMESPACE}>)

target_compile_features(${MODULE_NAME} INTERFACE cxx_std_11)

install(TARGETS ${MODULE_NAME} EXPORT ${MODULE_NAME}Targets)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/localtracer/localtracer.h
        DESTINATION include/${NAMESPACE}/localtracer)

include(HeaderOnlyInstall)

HeaderOnlyInstallPackageConfig(TARGET ${MODULE_NAME}
    NAME ${MODULE_NAME}
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Header only library to locally print traces coming from Messaging without the need of running Thunder.")

HeaderOnlyInstallCMakeConfig(TARGET ${MODULE_NAME} TREAT_AS_NORMAL)

add_subdirectory(example)
