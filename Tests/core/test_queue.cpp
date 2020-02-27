#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_queue, simple_queue)
{
    QueueType<int> obj1(20);
    EXPECT_TRUE(obj1.Insert(20,300));
    EXPECT_TRUE(obj1.Insert(30,300));
    EXPECT_TRUE(obj1.Post(20));
    EXPECT_TRUE(obj1.Remove(20));
    int a_Result = 30;
    EXPECT_TRUE(obj1.Extract(a_Result,300));
    EXPECT_EQ(obj1.Length(),1u);
    obj1.Enable();
    obj1.Disable();
    obj1.Flush();
}
