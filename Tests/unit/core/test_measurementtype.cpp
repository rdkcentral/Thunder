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
} // Tests
} // Thunder
