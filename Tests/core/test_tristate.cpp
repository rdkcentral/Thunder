#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_TriState, simple_TriState)
{
    TriState();
    TriState tristate1("F");
    TriState tristate2("T");
    TriState tristate3("True");
    TriState tristate4("False");
    TriState tristate5(tristate1);
    TriState tristate6 =tristate1;
    TriState tristate7(TriState::EnumState::True);

    EXPECT_EQ(tristate3.Get(),TriState::EnumState::True);
    tristate3.Set(false);
    EXPECT_EQ(tristate3.Get(),TriState::EnumState::False);
    tristate3.Set(TriState::EnumState::Unknown);
    EXPECT_EQ(tristate3.Get(),TriState::EnumState::Unknown);
}

