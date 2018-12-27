#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>

TEST(Test_IPTestAdministrator, sync)
{
   IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
      testAdmin.Sync("str01");

      testAdmin.Sync("str02_a");
   };

   IPTestAdministrator testAdmin(otherSide);

   bool result = testAdmin.Sync("str01");
   ASSERT_TRUE(result);

   result = testAdmin.Sync("str02_b");
   ASSERT_FALSE(result);
}

