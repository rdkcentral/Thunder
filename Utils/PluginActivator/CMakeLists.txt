cmake_minimum_required(VERSION 3.10.3)

project( PluginActivator )

set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

add_executable(${PROJECT_NAME}
    source/Module.cpp
    source/main.cpp
    source/COMRPCStarter.cpp
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
    -Wall
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}COM::${NAMESPACE}COM
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
