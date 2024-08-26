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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(Core_IPC, IPCClientConnection)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 1;

        const std::string connector = _T("/tmp/testserver");

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            ::Thunder::Core::NodeId serverNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            ::Thunder::Core::IPCChannelServerType<::Thunder::Core::Void, false> serverChannel(serverNode, 512, factory);

            ASSERT_EQ(serverChannel.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Only for internal factories
//            factory->DestroyFactories();

            // A server cannot 'end' its life if clients are connected
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Do not unregister any / the last handler prior the call of cleanup
            serverChannel.Cleanup();

            ASSERT_EQ(serverChannel.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            ::Thunder::Core::NodeId clientNode(connector.c_str());

            ::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> > factory(::Thunder::Core::ProxyType<::Thunder::Core::FactoryType<::Thunder::Core::IIPC, uint32_t> >::Create());

            ::Thunder::Core::IPCChannelClientType<::Thunder::Core::Void, false, false> clientChannel(clientNode, 512, factory);

            ASSERT_EQ(clientChannel.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Only for internal factories
//           factory->DestroyFactories();

            ASSERT_EQ(clientChannel.Source().Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // Signal the server it can 'end' its life
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
