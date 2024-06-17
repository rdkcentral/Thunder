/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(Core_IPC, IPCClientConnection)
    {
        std::string connector = _T("/tmp/testserver");

        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Thunder::Core::NodeId serverNode(connector.c_str());

            uint32_t error;

            Thunder::Core::ProxyType<Thunder::Core::FactoryType<Thunder::Core::IIPC, uint32_t> > factory(Thunder::Core::ProxyType<Thunder::Core::FactoryType<Thunder::Core::IIPC, uint32_t> >::Create());
            Thunder::Core::IPCChannelServerType<Thunder::Core::Void, false> serverChannel(serverNode, 512, factory);
            error = serverChannel.Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Thunder::Core::ERROR_NONE);

            testAdmin.Sync("setup server");
            testAdmin.Sync("setup client");
            testAdmin.Sync("done testing");

            error = serverChannel.Close(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Thunder::Core::ERROR_NONE);

            serverChannel.Cleanup();

            factory->DestroyFactories();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Thunder::Core::NodeId clientNode(connector.c_str());
            uint32_t error;

            testAdmin.Sync("setup server");

            Thunder::Core::ProxyType<Thunder::Core::FactoryType<Thunder::Core::IIPC, uint32_t> > factory(Thunder::Core::ProxyType<Thunder::Core::FactoryType<Thunder::Core::IIPC, uint32_t> >::Create());
            Thunder::Core::IPCChannelClientType<Thunder::Core::Void, false, false> clientChannel(clientNode, 512, factory);
            error = clientChannel.Source().Open(1000); // Wait for 1 Second.
            EXPECT_EQ(error, Thunder::Core::ERROR_NONE);
            testAdmin.Sync("setup client");

            error = clientChannel.Close(1000);
            EXPECT_EQ(error, Thunder::Core::ERROR_NONE);
            factory->DestroyFactories();
            Thunder::Core::Singleton::Dispose();
        }
        testAdmin.Sync("done testing");
    }

} // Core
} // Tests
} // Thunder
