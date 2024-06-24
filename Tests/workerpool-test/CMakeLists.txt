
add_executable(WorkerPoolTest
    Module.cpp
    WorkerPoolTest.cpp
)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

target_link_libraries(WorkerPoolTest
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS WorkerPoolTest DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
