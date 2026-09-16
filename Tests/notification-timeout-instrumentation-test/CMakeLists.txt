if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
add_executable(NotificationTimeoutInstrumentationTest
    Module.cpp
    NotificationTimeoutInstrumentationTest.cpp
)

set_target_properties(NotificationTimeoutInstrumentationTest PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(NotificationTimeoutInstrumentationTest
    PRIVATE
        ${NAMESPACE}Core
        ${NAMESPACE}COM
        ${NAMESPACE}Plugins
        ${NAMESPACE}ProxyStubs
)

install(TARGETS NotificationTimeoutInstrumentationTest
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${NAMESPACE}_Test
)
endif() # NOT Windows
