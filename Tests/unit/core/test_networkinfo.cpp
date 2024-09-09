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

namespace Thunder {
namespace Tests {
namespace Core {

    TEST(test_ipv4addressiterator, simple_ipv4addressiterator)
    {
        ::Thunder::Core::AdapterIterator adapters;
        ::Thunder::Core::AdapterIterator adapters2;

        ::Thunder::Core::AdapterIterator adapter("eth0");
        ::Thunder::Core::IPV4AddressIterator result;

        ASSERT_TRUE(adapter.IsValid());

        result.Next();
        //EXPECT_EQ(adapters.Index(),adapters.Index()); TODO
        while (adapters.Next() == true) {
           if (adapters.IsValid() == true) {
               ::Thunder::Core::IPV4AddressIterator index(adapters.IPV4Addresses());
               EXPECT_EQ(index.Count(),index.Count());
               EXPECT_STREQ(adapters.Name().c_str(),adapters.Name().c_str());
               while (index.Next() == true) {
                   ::Thunder::Core::NodeId current(index.Address());
                   ::Thunder::Core::IPNode currentNode(index.Address());
                   EXPECT_EQ(adapter.Add(currentNode),::Thunder::Core::ERROR_NONE);
                   EXPECT_EQ(adapter.Gateway(currentNode,current),::Thunder::Core::ERROR_NONE);
                   EXPECT_EQ(adapter.Delete(currentNode),::Thunder::Core::ERROR_NONE);
                   EXPECT_EQ(adapter.Broadcast(current),::Thunder::Core::ERROR_NONE);
                   if ((current.IsMulticast() == false) && (current.IsLocalInterface() == false)) {
                       result = index;
                       EXPECT_STREQ(current.HostName().c_str(),current.HostName().c_str());
                       EXPECT_STREQ(current.HostAddress().c_str(),current.HostAddress().c_str());
                   }
               }
           }
        }

        ::Thunder::Core::IPV4AddressIterator ipv4addressiterator1;
        ipv4addressiterator1 = result;
        ::Thunder::Core::IPV4AddressIterator ipv4addressiterator2(result);
        ipv4addressiterator1.Reset();

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(test_ipv6addressiterator, simple_ipv6addressiterator)
    {
        ::Thunder::Core::AdapterIterator adapters;
        ::Thunder::Core::IPV6AddressIterator result;
        result.Next();
        while (adapters.Next() == true) {
            ::Thunder::Core::IPV6AddressIterator index(adapters.IPV6Addresses());
            EXPECT_EQ(index.Count(),index.Count());
            EXPECT_STREQ(adapters.Name().c_str(),adapters.Name().c_str());
            while (index.Next() == true) {
                ::Thunder::Core::NodeId current(index.Address());
                if ((current.IsMulticast() == false) && (current.IsLocalInterface() == false)) {
                    result = index;
                    EXPECT_STREQ(current.HostName().c_str(),current.HostName().c_str());
                    EXPECT_STREQ(current.HostAddress().c_str(),current.HostAddress().c_str());
                }
            }
        }
        ::Thunder::Core::IPV6AddressIterator ipv6addressiterator1;
        ipv6addressiterator1 = result;
        ::Thunder::Core::IPV6AddressIterator ipv6addressiterator2(result);
        ipv6addressiterator1.Reset();

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(DISABLED_test_adapteriterator, simple_adapteriterator)
    {
        ::Thunder::Core::AdapterIterator adapter("eth0");
        ::Thunder::Core::AdapterIterator adapter1 = adapter;
        ::Thunder::Core::AdapterIterator adapter2(adapter);
        ::Thunder::Core::AdapterIterator adapter3("test0");

        EXPECT_TRUE(adapter.IsUp());
        EXPECT_TRUE(adapter.IsRunning());
        adapter.Up(true);
        adapter.Up(false);

        EXPECT_EQ(adapter.Count(),adapter.Count());
        EXPECT_STREQ(adapter.Name().c_str(),adapter.Name().c_str());

        EXPECT_STREQ(adapter.MACAddress(':').c_str(),adapter.MACAddress(':').c_str());
        uint8_t buffer[32];
        adapter.MACAddress(buffer,32);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(DISABLED_test_adapterobserver, simple_adapterobserver)
    {
        ::Thunder::Core::AdapterObserver::INotification* callback;
        ::Thunder::Core::AdapterObserver observer(callback);

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
