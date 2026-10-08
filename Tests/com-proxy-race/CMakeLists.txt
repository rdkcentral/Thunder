add_executable(ComProxyRaceBaseline
    Module.cpp
    ComProxyRace.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

target_link_libraries(ComProxyRaceBaseline
    PRIVATE
        ${NAMESPACE}Core
        ${NAMESPACE}COM
        ${NAMESPACE}Messaging
)

install(TARGETS ComProxyRaceBaseline DESTINATION bin)

add_executable(ComProxyRaceBuggy
    Module.cpp
    ComProxyRace.cpp
)

target_compile_definitions(ComProxyRaceBuggy
    PRIVATE
        COM_PROXY_RACE_REPRO_DELAY
)

target_link_libraries(ComProxyRaceBuggy
    PRIVATE
        ${NAMESPACE}Core
        ${NAMESPACE}COM
        ${NAMESPACE}Messaging
)

install(TARGETS ComProxyRaceBuggy DESTINATION bin)
