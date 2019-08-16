#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/StateTrigger.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

time_t g_time =time(NULL)+1;

TEST(test_statetrigger, simple_statetrigger)
{
    enum testState {
        TEST_INIT = 0x00,
        TEST_MESSAGE = 0x01,
        TEST_READY = 0x02,
        TEST_ERROR = 0x03,
    };

    WPEFramework::Core::StateTrigger<testState> state(TEST_READY);
    ASSERT_EQ(state.GetState(),TEST_READY) << "State not equal to TEST_READY.";
    state.SetState(TEST_MESSAGE);
    ASSERT_NE(state.GetState(),TEST_READY) << "State is TEST_MESSAGE.";
    ASSERT_TRUE(state.GetState() == TEST_MESSAGE) << "State is not TEST_MESSAGE";
    ASSERT_FALSE(state.GetState() != TEST_MESSAGE) << "State is TEST_MESSAGE";
    state = TEST_READY;
    state.WaitState(TEST_READY,g_time);

    state.WaitStateClear(TEST_INIT,g_time+5);
}
