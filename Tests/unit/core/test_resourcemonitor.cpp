/*
 * If not stated otherwise in this file or this component's license file the
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

#include "../Module.h"
#include <core/ResourceMonitor.h>

#ifdef THUNDER_CORE_RESOURCE_MONITOR_WATCHDOG

namespace WPEFramework {
namespace Tests {
namespace Core {

    TEST(Core_ResourceMonitorWatchDog, ArmsAndResetsCallbackDeadline)
    {
        ::WPEFramework::Core::ResourceMonitorWatchDog watchable(100);
        EXPECT_FALSE(watchable.Expired(::WPEFramework::Core::WatchDog::Ticks()));
        watchable.Arm();
        const uint64_t afterTimeout = ::WPEFramework::Core::WatchDog::Ticks()
            + (2 * 100 * ::WPEFramework::Core::Time::MicroSecondsPerMilliSecond);
        EXPECT_TRUE(watchable.Expired(afterTimeout));
        watchable.Reset();
        EXPECT_FALSE(watchable.Expired(afterTimeout));
    }

}
}
}

#endif
