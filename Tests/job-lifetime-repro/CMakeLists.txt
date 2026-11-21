
add_executable(JobLifetimeReproFixed
    Module.cpp
    JobLifetimeRepro.cpp
)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

target_link_libraries(JobLifetimeReproFixed
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS JobLifetimeReproFixed DESTINATION bin)

add_executable(JobLifetimeReproBuggy
    Module.cpp
    JobLifetimeRepro.cpp
)

target_compile_definitions(JobLifetimeReproBuggy
    PRIVATE
        BUGGY
)

target_link_libraries(JobLifetimeReproBuggy
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS JobLifetimeReproBuggy DESTINATION bin)

add_executable(JobLifetimeReproCrossThread
    Module.cpp
    JobLifetimeReproCrossThread.cpp
)

target_link_libraries(JobLifetimeReproCrossThread
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS JobLifetimeReproCrossThread DESTINATION bin)

# Manual reproduction tool for the IDLE-window revoke race -- NOT an automated
# regression test. Reproducing the race requires a temporary, hand-applied
# delay in Source/core/ThreadPool.h's Minion::Process() (documented at that
# exact spot, and in this test's own header comment); without it, the window
# is essentially never hit. Intentionally not add_test()-registered.
add_executable(JobLifetimeReproIdleWindow
    Module.cpp
    JobLifetimeReproIdleWindow.cpp
)

target_link_libraries(JobLifetimeReproIdleWindow
    PRIVATE
        ${NAMESPACE}Core
)

install(TARGETS JobLifetimeReproIdleWindow DESTINATION bin)

enable_testing()

add_test(NAME JobLifetimeReproFixed COMMAND JobLifetimeReproFixed)
set_tests_properties(JobLifetimeReproFixed PROPERTIES TIMEOUT 30)

add_test(NAME JobLifetimeReproCrossThread COMMAND JobLifetimeReproCrossThread)
set_tests_properties(JobLifetimeReproCrossThread PROPERTIES TIMEOUT 30)
