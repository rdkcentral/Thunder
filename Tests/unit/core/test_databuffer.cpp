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

    TEST(Core_DataBuffer, simple)
    {
        constexpr uint32_t blocksize = 10;

        ::Thunder::Core::ScopedStorage<blocksize> storage;

PUSH_WARNING(DISABLE_WARNING_MAYBE_UNINITIALIZED)
        EXPECT_EQ(storage.Size(), blocksize);
        EXPECT_NE(storage.Buffer(), nullptr);
POP_WARNING()
    }

} // Core
} // Tests
} // Thunder
