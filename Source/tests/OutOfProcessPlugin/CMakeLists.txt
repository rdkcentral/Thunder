project(OutOfProcessTest)

cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig)
pkg_check_modules(PLUGINS plugins)

get_filename_component(INSTALL_PREFIX ${PLUGINS_INCLUDEDIR} NAME)

list(APPEND CMAKE_MODULE_PATH "$ENV{STAGING_DIR}${CMAKE_INSTALL_PREFIX}/include/${INSTALL_PREFIX}/cmake/")
include(config)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(PLUGIN_LIBS ${PLUGINS_LIBRARIES} ${PROVISION_LIBRARIES})    

set(PLUGIN_SOURCES 
    OutOfProcessTest.cpp
    OutOfProcessImplementation.cpp
    Module.cpp)

if (BUILD_REFERENCE)
    add_definitions (-DBUILD_REFERENCE=${BUILD_REFERENCE})
endif()

# add the library
add_library(${PROJECT_NAME} SHARED ${PLUGIN_SOURCES})
include_directories(${PLUGINS_INCLUDEDIR})
target_link_libraries(${PROJECT_NAME} ${PLUGIN_LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${INSTALL_PREFIX}/plugins)

#---------------------------------------------------------------------------------------------------------------
# JSON CONFIG SECTION
#---------------------------------------------------------------------------------------------------------------
map() 
    kv(locator "lib${PROJECT_NAME}.so")
    kv(classname "${PROJECT_NAME}")
    key(configuration)
    map()
        kv(test true)
    end()
end()
ans(PLUGIN_CONFIG)

json_write("${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.json" ${PLUGIN_CONFIG})
install(FILES "${PROJECT_NAME}.json" DESTINATION ${CMAKE_INSTALL_PREFIX}/../etc/${INSTALL_PREFIX}/plugins/)
