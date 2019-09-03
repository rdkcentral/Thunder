#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

const char g_bufferName1[] = "cyclicbuffer01";
const char g_bufferName2[] = "cyclicbuffer02";

void CleanUpCyclicBuffer()
{
   // TODO: shouldn't this be done producer-side?
   char systemCmd[1024];
   sprintf(systemCmd, "rm %s %s", g_bufferName1, g_bufferName2);
   system(systemCmd);
}

TEST(Core_CyclicBuffer, simpleSet)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
        CleanUpCyclicBuffer();

        uint32_t result;
        string data;
        uint32_t bufferSize = 10;
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer bufferWithOverwrite(g_bufferName1, bufferSize, true);
        Core::CyclicBuffer bufferWithoutOverwrite(g_bufferName2, bufferSize, false);

        testAdmin.Sync("setup server");

        testAdmin.Sync("setup client");

        ASSERT_EQ(bufferWithOverwrite.Read(loadBuffer, bufferWithOverwrite.Used()), 0);
        ASSERT_EQ(bufferWithoutOverwrite.Read(loadBuffer, bufferWithoutOverwrite.Used()), 0);

        data = "abcdefghi";
        result = bufferWithOverwrite.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        ASSERT_EQ(result, data.size());
        result = bufferWithoutOverwrite.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        ASSERT_EQ(result, data.size());

        testAdmin.Sync("server wrote");

        testAdmin.Sync("client read");

        testAdmin.Sync("client wrote");

        result = bufferWithOverwrite.Peek(loadBuffer, bufferWithOverwrite.Used());
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "ijklmnopq");
        result = bufferWithoutOverwrite.Peek(loadBuffer, bufferWithoutOverwrite.Used());
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("server peek");

        result = bufferWithOverwrite.Read(loadBuffer, bufferWithOverwrite.Used());
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "ijklmnopq");
        result = bufferWithoutOverwrite.Read(loadBuffer, bufferWithoutOverwrite.Used());
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "efghi");

        testAdmin.Sync("server read");
    };

    // This side (tested) acts as client
    IPTestAdministrator testAdmin(otherSide);
    
    {
        testAdmin.Sync("setup server");

        uint32_t result;
        string data;
        uint32_t bufferSize = 10;
        uint8_t loadBuffer[bufferSize + 1];
        Core::CyclicBuffer bufferWithOverwrite(g_bufferName1, bufferSize, true);
        Core::CyclicBuffer bufferWithoutOverwrite(g_bufferName2, bufferSize, false);

        testAdmin.Sync("setup client");

        testAdmin.Sync("server wrote");

        result = bufferWithOverwrite.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "abcd");
        result = bufferWithoutOverwrite.Read(loadBuffer, 4);
        loadBuffer[result] = '\0';
        ASSERT_STREQ((char*)loadBuffer, "abcd");

        testAdmin.Sync("client read");

        data = "j";
        result = bufferWithOverwrite.Reserve(8);
        result = bufferWithOverwrite.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        ASSERT_EQ(result, data.size());
        data = "klmnopq";
        result = bufferWithOverwrite.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        ASSERT_EQ(result, data.size());
        result = bufferWithoutOverwrite.Reserve(data.size());
        ASSERT_EQ(result, Core::ERROR_INVALID_INPUT_LENGTH);
        result = bufferWithoutOverwrite.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        ASSERT_EQ(result, 0);

        testAdmin.Sync("client wrote");

        testAdmin.Sync("server peek");

        testAdmin.Sync("server read");
    }
    Core::Singleton::Dispose();
}
