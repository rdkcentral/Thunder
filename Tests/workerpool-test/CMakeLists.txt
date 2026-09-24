
add_executable(WorkerPoolTest
    Module.cpp
    WorkerPoolTest.cpp
)

target_compile_options(WorkerPoolTest PRIVATE -pthread)
target_link_options(WorkerPoolTest PRIVATE -pthread)
set_target_properties(WorkerPoolTest PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)

target_link_libraries(WorkerPoolTest
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS WorkerPoolTest DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
