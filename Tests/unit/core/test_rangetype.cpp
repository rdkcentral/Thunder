/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace Thunder {
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
} // Thunder
