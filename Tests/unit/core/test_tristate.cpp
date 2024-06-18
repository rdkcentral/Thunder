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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {    

    TEST(test_TriState, simple_TriState)
    {
        Thunder::Core::TriState();
        Thunder::Core::TriState tristate1("F");
        Thunder::Core::TriState tristate2("T");
        Thunder::Core::TriState tristate3("True");
        Thunder::Core::TriState tristate4("False");
        Thunder::Core::TriState tristate5(tristate1);
        Thunder::Core::TriState tristate6 =tristate1;
        Thunder::Core::TriState tristate7(Thunder::Core::TriState::EnumState::True);

        EXPECT_EQ(tristate3.Get(),Thunder::Core::TriState::EnumState::True);
        tristate3.Set(false);
        EXPECT_EQ(tristate3.Get(),Thunder::Core::TriState::EnumState::False);
        tristate3.Set(Thunder::Core::TriState::EnumState::Unknown);
        EXPECT_EQ(tristate3.Get(),Thunder::Core::TriState::EnumState::Unknown);
    }

} // Core
} // Tests
} // Thunder
