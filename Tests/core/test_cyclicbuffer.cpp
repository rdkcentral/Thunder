#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

const char g_bufferName[] = "cyclicbuffer01";
const char g_defaultText[] = "Default text.";

void CleanUpCyclicBuffer()
{
   // TODO: shouldn't this be done producer-side?
   char systemCmd[1024];
   sprintf(systemCmd, "rm -f %s", g_bufferName);
   system(systemCmd);
}

TEST(Core_CyclicBuffer, simpleSet)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        CleanUpCyclicBuffer();
        Core::CyclicBuffer buffer(g_bufferName, 256, true);
        testAdmin.Sync("setup server");
        testAdmin.Sync("setup client");
        buffer.Write(reinterpret_cast<const uint8_t*>(g_defaultText), sizeof(g_defaultText));
        testAdmin.Sync("server wrote");
        testAdmin.Sync("client read");
    };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);
    
    {
        testAdmin.Sync("setup server");

        Core::CyclicBuffer buffer(g_bufferName, 0, true);
        testAdmin.Sync("setup client");
        testAdmin.Sync("server wrote");
        uint8_t loadBuffer[128];
        uint32_t result = buffer.Read(loadBuffer, buffer.Used());
        ASSERT_EQ(result, sizeof(g_defaultText));
        ASSERT_STREQ(g_defaultText, (const char*)loadBuffer);
    }
    testAdmin.Sync("client read");
    Core::Singleton::Dispose();
}
