if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
add_executable(IntentionalCrashTest
    Module.cpp
    IntentionalCrashTest.cpp
)

set_target_properties(IntentionalCrashTest PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(IntentionalCrashTest
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS IntentionalCrashTest
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${NAMESPACE}_Test
)
endif() # NOT Windows
