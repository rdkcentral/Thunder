#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

TEST(Core_MeasurementType, simpleSet)
{
    Core::MeasurementType<uint8_t> uint8;
    uint8.Set(100);
    uint8.Set(200);
    uint8.Set(120);
    uint8.Set(220);
    uint8.Set(140);
    uint8.Set(240);
    ASSERT_EQ(uint8.Min(), 100);
    ASSERT_EQ(uint8.Max(), 240);
    ASSERT_EQ(uint8.Last(), 240);
    ASSERT_EQ(uint8.Average(), 170);
    ASSERT_EQ(uint8.Measurements(), 6);
    uint8.Reset();
    ASSERT_EQ(uint8.Last(), 0);
    ASSERT_EQ(uint8.Average(), 0);
    ASSERT_EQ(uint8.Measurements(), 0);
}
