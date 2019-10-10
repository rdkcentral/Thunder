#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    template <typename T, bool BOUNDARY_INCLUSIVE = true>
    void RangeTest(T data)
    {
        EXPECT_TRUE(data.IsValid());
        EXPECT_EQ(data.Maximum(), 100u);
        EXPECT_EQ(data.Minimum(), 1u);
        if (BOUNDARY_INCLUSIVE) {
            EXPECT_EQ(data.Range(), 100u);
            EXPECT_TRUE(data.InRange(1));
            EXPECT_TRUE(data.InRange(100));
        } else {
            EXPECT_EQ(data.Range(), 98u);
            EXPECT_FALSE(data.InRange(1));
            EXPECT_FALSE(data.InRange(100));
        }
    }

    TEST(Core_RangeType, BoundaryExclusive)
    {
        Core::RangeType<uint8_t, false, false> data(1u, 100u);
        RangeTest<Core::RangeType<uint8_t, false, false>, false>(data);
    }

    TEST(Core_RangeType, BoundaryInclusive)
    {
        Core::RangeType<uint8_t, true, true> data(1u, 100u);
        RangeTest<Core::RangeType<uint8_t, true, true>>(data);
    }

    TEST(Core_RangeType, EqualBoundary)
    {
        Core::RangeType<uint8_t, true, true> withBoundary(2u, 2u);
        EXPECT_EQ(withBoundary.Range(), 1u);

        Core::RangeType<uint8_t, false, false> withoutBoundary(2u, 2u);
        EXPECT_EQ(withoutBoundary.Range(), 0u);
    }
} // Tests
} // WPEFramework
