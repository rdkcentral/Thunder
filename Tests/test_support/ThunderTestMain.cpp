// ==========================================================================
// ThunderTestMain — GTest main() for plugin integration tests.
//
// When a test binary loads external plugin shared libraries (.so) via the
// ThunderTestRuntime, those libraries may register static objects whose
// destruction order at process exit conflicts with Thunder's own
// singletons (WorkerPool, ResourceMonitor, etc.).
//
// This main() calls _Exit() after RUN_ALL_TESTS() to skip static
// destruction entirely, matching the real Thunder daemon which calls
// exit(0) after its CloseDown() sequence.
//
// Link against thunder_test_main instead of GTest::Main:
//   target_link_libraries(my_test PRIVATE
//       thunder_test_support::thunder_test_support
//       thunder_test_main::thunder_test_main
//       GTest::GTest)
// ==========================================================================

#include <gtest/gtest.h>
#include <cstdlib>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    _Exit(result);
}
