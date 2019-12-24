#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    string g_connector = _T("/tmp/testserver");

    TEST(Core_IPC, IPCClientConnection)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            Core::NodeId serverNode(g_connector.c_str());
            uint32_t error;

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());
            Core::IPCChannelServerType<Core::Void, false> serverChannel(serverNode, 512, factory);
            error = serverChannel.Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = serverChannel.Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);

            factory->DestroyFactories();
        };
        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId clientNode(g_connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> > factory(Core::ProxyType<Core::FactoryType<Core::IIPC, uint32_t> >::Create());
            Core::IPCChannelClientType<Core::Void, false, false> clientChannel(clientNode, 512, factory);
            error = clientChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Core::ERROR_NONE);
            testAdmin.Sync("setup client");

            error = clientChannel.Close(1000);
            EXPECT_EQ(error, Core::ERROR_NONE);
            factory->DestroyFactories();
            Core::Singleton::Dispose();
        }
        testAdmin.Sync("done testing");
    }
} // Tests
} // WPEFramework
