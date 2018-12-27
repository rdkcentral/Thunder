#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/SharedBuffer.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

const uint32_t g_bufferSize = 8 * 1024;
const uint16_t g_administrationSize = 64;
const char g_bufferName[] = "testbuffer01";

void CleanUpBuffer()
{
   // TODO: shouldn't this be done producer-side?
   char systemCmd[1024];
   sprintf(systemCmd, "rm -f %s", g_bufferName);
   system(systemCmd);
   sprintf(systemCmd, "rm -f %s.admin", g_bufferName);
   system(systemCmd);
}

TEST(Core_SharedBuffer, simpleSet)
{
   IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
      CleanUpBuffer();

      SharedBuffer buff01(g_bufferName, g_bufferSize, g_administrationSize);
      buff01.RequestProduce(Core::infinite);
      
      testAdmin.Sync("setup producer");

      testAdmin.Sync("setup consumer");
      
      uint8_t * buffer = buff01.Buffer();
      uint64_t bufferSize = buff01.Size();
      ASSERT_EQ(bufferSize, g_bufferSize);

      buffer[0] = 42;
      buffer[1] = 43;
      buffer[2] = 44;

      buff01.Produced();
      
      testAdmin.Sync("consumer done");
   };

   // This side (tested) acts as client (consumer).
   IPTestAdministrator testAdmin(otherSide);

   {
      // In extra scope, to make sure "buff01" is destructed before producer.
      bool result = testAdmin.Sync("setup producer");
      ASSERT_TRUE(result);

      SharedBuffer buff01(g_bufferName);

      testAdmin.Sync("setup consumer");

      buff01.RequestConsume(Core::infinite);
      
      uint8_t * buffer = buff01.Buffer();
      uint64_t bufferSize = buff01.Size();
      ASSERT_EQ(bufferSize, g_bufferSize);

      EXPECT_EQ(buffer[0], 42);
      EXPECT_EQ(buffer[1], 43);
      EXPECT_EQ(buffer[2], 44);

      buff01.Consumed();
   }

   testAdmin.Sync("consumer done");

   Core::Singleton::Dispose();
}
