#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_optional, simple_test)
{
    OptionalType<int> obj;
    int type = obj;
    EXPECT_EQ(type, 0);
    obj.Clear();
    EXPECT_FALSE(obj.IsSet());
    OptionalType<string> obj1("request");
    OptionalType<string> obj2(obj1);
    EXPECT_TRUE(obj2 == obj1);
    EXPECT_TRUE(obj2.Value() == obj1.Value());
    EXPECT_TRUE(obj1.IsSet());
    const OptionalType<string> obj3;
    OptionalType<string> obj4;
    obj4 = obj3;
    const OptionalType<int> obj5;
    const OptionalType<int> objSample = 40;
    OptionalType<int> object;
    object = 20;
    EXPECT_TRUE(objSample != object.Value());
    EXPECT_TRUE(objSample != obj5);
    int type1 = objSample;
    EXPECT_EQ(type1, 40);
}
