add_executable(BroadcastTester BroadcastTester.cpp)

target_link_libraries(BroadcastTester 
    PRIVATE
        ${NAMESPACE}Broadcast::${NAMESPACE}Broadcast
        ${NAMESPACE}Core::${NAMESPACE}Core
)   

install(TARGETS BroadcastTester DESTINATION bin)