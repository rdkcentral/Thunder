# CMakeLists.txt for binder COMRPC integration tests
# Enabled only when BINDER_COMRPC=ON and a binder kernel is available.

if(NOT BINDER_COMRPC)
    message(STATUS "Binder tests: skipped (BINDER_COMRPC is OFF)")
    return()
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(BINDER_TEST_LIBS
    ${NAMESPACE}Core
    ${NAMESPACE}COM
    ${NAMESPACE}Messaging
)

# ============================================================
# Helper: add a single-source test executable
# ============================================================
function(add_binder_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE ${BINDER_TEST_LIBS})
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Source/core
        ${CMAKE_SOURCE_DIR}/Source/com
    )
endfunction()

add_binder_test(test_binder_transport)
add_binder_test(test_binder_svcmgr)
add_binder_test(test_binder_announce)
add_binder_test(test_binder_invoke)
add_binder_test(test_binder_multi_client)
add_binder_test(test_binder_svcmgr_fail)

install(TARGETS
    test_binder_transport
    test_binder_svcmgr
    test_binder_announce
    test_binder_invoke
    test_binder_multi_client
    test_binder_svcmgr_fail
    DESTINATION ${CMAKE_INSTALL_BINDIR}/binder-tests
)
