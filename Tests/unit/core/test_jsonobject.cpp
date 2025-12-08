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

#ifdef __APPLE__
#include <time.h>
#endif

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(JSONOBJECT, FirstKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("name"));

        EXPECT_TRUE(container.HasLabel("Name"));

        /* void */ container.Remove("Name");

        EXPECT_FALSE(container.HasLabel("Name"));
    }

    TEST(JSONOBJECT, SecondKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("age"));

        EXPECT_TRUE(container.HasLabel("Age"));

        /* void */ container.Remove("Age");

        EXPECT_FALSE(container.HasLabel("Age"));
    }

    TEST(JSONOBJECT, ThirdKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("gender"));

        EXPECT_TRUE(container.HasLabel("Gender"));

        /* void */ container.Remove("Gender");

        EXPECT_FALSE(container.HasLabel("Gender"));
    }

    TEST(JSONOBJECT, IdenticalKeyValueRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": "Age", "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("age"));

        EXPECT_TRUE(container.HasLabel("Age"));

        /* void */ container.Remove("Age");

        EXPECT_FALSE(container.HasLabel("Age"));
    }

} // Core
} // Tests
} // Thunder
