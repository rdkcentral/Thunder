#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(test_rectangle, simple_rectangle)
{
    Rectangle();
    Rectangle r1(2,3,2,3); 
    Rectangle r2(2,4,2,4);
    EXPECT_FALSE(r1 == r2);
    EXPECT_TRUE(r1 != r2);
 
    Rectangle r3(2,3,2,3); 
    EXPECT_TRUE(r1 == r3);
    EXPECT_FALSE(r1 != r3);
     
    Rectangle r4 = r1 & r2;
    Rectangle r5(2,4,2,2);
    EXPECT_TRUE(r4 == r5);

    Rectangle r6 = r1 | r2;
    Rectangle r7(2,3,2,5);
    EXPECT_TRUE(r6 == r7);
    
    Rectangle r8 = r1.combine(r2);
    EXPECT_TRUE(r8 == r7);

    EXPECT_TRUE(r1.Contains(2,3));
    EXPECT_TRUE(r1.Overlaps(r2));
}
