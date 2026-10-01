
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
