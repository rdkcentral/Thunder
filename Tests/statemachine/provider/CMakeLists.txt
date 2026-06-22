set(PLUGIN_TARGET ThunderTestStreamingProvider)

add_library(${PLUGIN_TARGET} SHARED
    StreamingProvider.cpp
    Module.cpp
)

target_compile_definitions(${PLUGIN_TARGET}
    PRIVATE
        MODULE_NAME=StreamingProvider
)

if(BUILD_REFERENCE)
    target_compile_definitions(${PLUGIN_TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${PLUGIN_TARGET}
    PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}COM::${NAMESPACE}COM
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
)

add_custom_command(TARGET ThunderTestStreamingProvider POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PLUGIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E create_symlink 
        "$<TARGET_FILE:ThunderTestStreamingProvider>" 
        "${PLUGIN_DIR}/$<TARGET_FILE_NAME:ThunderTestStreamingProvider>"
    COMMENT "Creating symlink for ThunderTestStreamingProvider"
)
