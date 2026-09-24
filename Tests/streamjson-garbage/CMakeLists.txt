if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
add_executable(StreamJSONGarbageTest
    Module.cpp
    GarbageTest.cpp
)

set_target_properties(StreamJSONGarbageTest PROPERTIES
    CXX_STANDARD ${CXX_STD}
    CXX_STANDARD_REQUIRED YES
)

target_compile_options(StreamJSONGarbageTest PRIVATE -pthread)
target_link_options(StreamJSONGarbageTest PRIVATE -pthread)

target_link_libraries(StreamJSONGarbageTest
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS StreamJSONGarbageTest
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${NAMESPACE}_Test
)
endif() # NOT Windows
