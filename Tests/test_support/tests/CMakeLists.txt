# Smoke test for thunder_test_support
# Boots the embedded server and exercises JSON-RPC calls against the
# built-in Controller plugin. No external plugin .so files needed.

find_package(GTest REQUIRED)

add_executable(thunder_test_runtime_smoke
    SmokeTest.cpp
    Module.cpp
)

target_link_libraries(thunder_test_runtime_smoke
    PRIVATE
        thunder_test_support
        GTest::GTest
        GTest::Main
)
