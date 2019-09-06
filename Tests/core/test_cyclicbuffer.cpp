#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    const char g_bufferName[] = "cyclicbuffer01";

    TEST(Core_CyclicBuffer, WithoutOverwrite)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            uint32_t result;
            string data;
            uint32_t bufferSize = 10;
            uint8_t loadBuffer[bufferSize + 1];
            Core::CyclicBuffer buffer(g_bufferName, bufferSize, false);

            testAdmin.Sync("setup server");

            testAdmin.Sync("setup client");

            EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

            data = "abcdefghi";
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            testAdmin.Sync("server wrote");

            testAdmin.Sync("client read");

            testAdmin.Sync("client wrote");

            result = buffer.Peek(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 5u);
            EXPECT_STREQ((char*)loadBuffer, "efghi");

            testAdmin.Sync("server peek");

            result = buffer.Read(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 5u);
            EXPECT_STREQ((char*)loadBuffer, "efghi");

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
            Core::CyclicBuffer buffer(g_bufferName, bufferSize, false);

            testAdmin.Sync("setup client");

            testAdmin.Sync("server wrote");

            result = buffer.Read(loadBuffer, 4);
            loadBuffer[result] = '\0';
            EXPECT_STREQ((char*)loadBuffer, "abcd");

            testAdmin.Sync("client read");

            data = "klmnopq";
            result = buffer.Reserve(data.size());
            EXPECT_EQ(result, Core::ERROR_INVALID_INPUT_LENGTH);
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, 0u);

            testAdmin.Sync("client wrote");

            testAdmin.Sync("server peek");

            testAdmin.Sync("server read");
        }
        Core::Singleton::Dispose();
    }

    TEST(Core_CyclicBuffer, WithOverwrite)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            uint32_t result;
            string data;
            uint32_t bufferSize = 10;
            uint8_t loadBuffer[bufferSize + 1];
            Core::CyclicBuffer buffer(g_bufferName, bufferSize, true);

            testAdmin.Sync("setup server");

            testAdmin.Sync("setup client");

            EXPECT_EQ(buffer.Read(loadBuffer, buffer.Used()), 0u);

            data = "abcdefghi";
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            testAdmin.Sync("server wrote");

            testAdmin.Sync("client read");

            testAdmin.Sync("client wrote");

            result = buffer.Peek(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 9u);
            EXPECT_STREQ((char*)loadBuffer, "ijklmnopq");

            testAdmin.Sync("server peek");

            result = buffer.Read(loadBuffer, buffer.Used());
            loadBuffer[result] = '\0';
            EXPECT_EQ(result, 9u);
            EXPECT_STREQ((char*)loadBuffer, "ijklmnopq");

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
            Core::CyclicBuffer buffer(g_bufferName, bufferSize, true);

            testAdmin.Sync("setup client");

            testAdmin.Sync("server wrote");

            result = buffer.Read(loadBuffer, 4);
            loadBuffer[result] = '\0';
            EXPECT_STREQ((char*)loadBuffer, "abcd");

            testAdmin.Sync("client read");

            data = "j";
            result = buffer.Reserve(8);
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());
            data = "klmnopq";
            result = buffer.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
            EXPECT_EQ(result, data.size());

            testAdmin.Sync("client wrote");

            testAdmin.Sync("server peek");

            testAdmin.Sync("server read");
        }
        Core::Singleton::Dispose();
    }
}
}
