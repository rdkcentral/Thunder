#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/Queue.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_queue, simple_queue)
{
    QueueType<int> obj1(20);
    bool ins= obj1.Insert(20,300);
    ASSERT_TRUE(ins);
    bool ins1= obj1.Insert(30,300);
    ASSERT_TRUE(ins1);
    bool post= obj1.Post(20);
    ASSERT_TRUE(post);
    bool remove= obj1.Remove(20);
    ASSERT_TRUE(remove);
    int a_Result = 30;
    bool extract= obj1.Extract(a_Result,300);
    ASSERT_TRUE(extract);
    obj1.Enable();
    obj1.Disable();
    obj1.Flush();
}
