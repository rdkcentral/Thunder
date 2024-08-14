#find_package(${NAMESPACE}Core REQUIRED)
#find_package(${NAMESPACE}Cryptalgo REQUIRED)
#find_package(${NAMESPACE}WebSocket REQUIRED)
#find_package(${NAMESPACE}Tracing REQUIRED)

add_executable(httpsclient_test
    Module.cpp
    main.cpp
)

target_link_libraries(httpsclient_test
    PRIVATE
        ${NAMESPACE}Core
        ${NAMESPACE}Cryptalgo
        ${NAMESPACE}Tracing
        ${NAMESPACE}WebSocket
)

install(TARGETS httpsclient_test DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
