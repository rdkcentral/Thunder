#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_stopWatch, simple_stopWatch)
{
     StopWatch stopwatch;
     uint64_t elapsed = stopwatch.Elapsed();
     uint64_t elapsed1 = elapsed;
     EXPECT_EQ(elapsed,elapsed1);
     
     uint64_t reset = stopwatch.Reset();
     uint64_t reset1 = reset;
     EXPECT_EQ(reset,reset1);
}
