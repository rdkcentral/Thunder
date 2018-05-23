#
# virtual input monitor
#

add_executable(vk_monitor vk_monitor.c)

target_link_libraries(vk_monitor
        PRIVATE
        WPEFramework::WPEFrameworkVirtualInput)


install(
        TARGETS vk_monitor EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/${MODULE} COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${MODULE}      # headers
)