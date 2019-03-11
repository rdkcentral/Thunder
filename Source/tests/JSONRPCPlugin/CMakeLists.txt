set(PLUGIN_NAME SystemdConnector)
set(MODULE_NAME WPEFramework${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(Systemd REQUIRED)

add_library(${MODULE_NAME} SHARED 
    SystemdConnector.cpp
    Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME} 
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${SYSTEMD_LIBRARIES})

install(TARGETS ${MODULE_NAME} 
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})