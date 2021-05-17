
# find_package(${NAMESPACE}Core REQUIRED)
# find_package(${NAMESPACE}Cryptalgo REQUIRED)
# find_package(${NAMESPACE}WebSocket REQUIRED)

add_executable(httpsclient_test
    main.cpp
)

target_link_libraries(httpsclient_test
    PRIVATE
        ${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo
        ${NAMESPACE}WebSocket)

install(TARGETS httpsclient_test DESTINATION bin)
