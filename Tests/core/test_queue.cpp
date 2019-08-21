#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/Queue.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_queue, simple_queue)
{
    QueueType<int> obj1(20);
    ASSERT_TRUE(obj1.Insert(20,300));
    ASSERT_TRUE(obj1.Insert(30,300));
    ASSERT_TRUE(obj1.Post(20));
    ASSERT_TRUE(obj1.Remove(20));
    int a_Result = 30;
    ASSERT_TRUE(obj1.Extract(a_Result,300));
    ASSERT_EQ(obj1.Length(),1);
    obj1.Enable();
    obj1.Disable();
    obj1.Flush();
}
