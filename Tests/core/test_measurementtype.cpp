#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    TEST(Core_MeasurementType, simpleSet)
    {
        Core::MeasurementType<uint8_t> data;
        data.Set(100);
        data.Set(200);
        data.Set(120);
        data.Set(220);
        data.Set(140);
        data.Set(240);
        EXPECT_EQ(data.Min(), 100u);
        EXPECT_EQ(data.Max(), 240u);
        EXPECT_EQ(data.Last(), 240u);
        EXPECT_EQ(data.Average(), 170u);
        EXPECT_EQ(data.Measurements(), 6u);
        data.Reset();
        EXPECT_EQ(data.Last(), 0u);
        EXPECT_EQ(data.Average(), 0u);
        EXPECT_EQ(data.Measurements(), 0u);
    }
}
}
