#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

enum class TestState {
    TEST_INIT = 0x00,
    TEST_MESSAGE = 0x01,
    TEST_READY = 0x02,
    TEST_ERROR = 0x03
};

TEST(test_statetrigger, simple_statetrigger)
{
    WPEFramework::Core::StateTrigger<TestState> state(TestState::TEST_READY);

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
