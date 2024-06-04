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

using namespace Thunder;
using namespace Thunder::Core;

enum class TestState {
    TEST_INIT = 0x00,
    TEST_MESSAGE = 0x01,
    TEST_READY = 0x02,
    TEST_ERROR = 0x03
};

TEST(test_statetrigger, simple_statetrigger)
{
    Thunder::Core::StateTrigger<TestState> state(TestState::TEST_READY);

    EXPECT_EQ(state.GetState(),TestState::TEST_READY) << "State not equal to TEST_READY.";
    state.SetState(TestState::TEST_MESSAGE);
    EXPECT_NE(state.GetState(),TestState::TEST_READY) << "State is TEST_MESSAGE.";
    EXPECT_TRUE(state.GetState() == TestState::TEST_MESSAGE) << "State is not TEST_MESSAGE";
    EXPECT_FALSE(state.GetState() != TestState::TEST_MESSAGE) << "State is TEST_MESSAGE";
    state = TestState::TEST_READY;

    uint64_t timeOut(Core::Time::Now().Add(5).Ticks());
    uint64_t now(Core::Time::Now().Ticks());

    state.WaitState(static_cast<uint32_t>(TestState::TEST_INIT),static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));

    timeOut = Core::Time::Now().Add(3).Ticks();
    now = Core::Time::Now().Ticks();

    state.WaitStateClear(static_cast<uint32_t>(TestState::TEST_READY),static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
}
