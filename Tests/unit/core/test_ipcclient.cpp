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

    TEST(Core_IPC, IPCClientConnection)
    {
        std::string connector = _T("/tmp/testserver");

        auto lambdaFunc = [connector](IPTestAdministrator & testAdmin) {
            Core::NodeId serverNode(connector.c_str());

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

            serverChannel.Cleanup();

            factory->DestroyFactories();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::NodeId clientNode(connector.c_str());
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
} // Thunder
