cmake_minimum_required(VERSION 3.10)
project(WorkerPoolSelfResubmitTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add the test executable
add_executable(test_WorkerPool_SelfResubmit
    test_WorkerPool_SelfResubmit.cpp
)

# Link against Thunder and GTest
target_link_libraries(test_WorkerPool_SelfResubmit
    ${GTEST_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${NAMESPACE}Core::${NAMESPACE}Core
)

install(
    TARGETS test_WorkerPool_SelfResubmit
    DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)

# Add the test to CTest
enable_testing()
add_test(NAME WorkerPoolSelfResubmit COMMAND test_WorkerPool_SelfResubmit)
