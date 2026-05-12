set(TARGET thunder_test_runtime_smoke)

find_package(GTest REQUIRED)

add_executable(${TARGET}
    SmokeTest.cpp
    Module.cpp
)

target_compile_definitions(${TARGET}
    PRIVATE
        MODULE_NAME=SmokeTest
)

if(BUILD_REFERENCE)
    target_compile_definitions(${TARGET} PRIVATE BUILD_REFERENCE=${BUILD_REFERENCE})
endif()

target_link_libraries(${TARGET}
    PRIVATE
        thunder_test_support
        GTest::GTest
        GTest::Main
)

add_test(NAME ${TARGET} COMMAND ${TARGET})