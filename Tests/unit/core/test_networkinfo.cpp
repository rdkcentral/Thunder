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

    // -------------------------------------------------------------------------
    // Helper: return a well-known loopback adapter name for the current platform
    // -------------------------------------------------------------------------
    static const char* LoopbackName()
    {
#if defined(__WINDOWS__)
        return "Loopback Pseudo-Interface 1";
#else
        return "lo0"; // macOS & most BSDs
                      // Linux uses "lo" – tests that need it can fall back
#endif
    }

    // Helper: find the first adapter that has at least one IPv4 address
    static bool FindAdapterWithIPv4(::Thunder::Core::AdapterIterator& out)
    {
        ::Thunder::Core::AdapterIterator it;
        while (it.Next()) {
            ::Thunder::Core::IPV4AddressIterator addrs = it.IPV4Addresses();
            if (addrs.Count() > 0) {
                out = it;
                return true;
            }
        }
        return false;
    }

    // =========================================================================
    // AdapterIterator – construction / enumeration
    // =========================================================================

    TEST(Core_NetworkInfo, AdapterIterator_DefaultConstruction)
    {
        ::Thunder::Core::AdapterIterator adapters;

        // Before calling Next(), the iterator should not be valid
        EXPECT_FALSE(adapters.IsValid());

        // There should be at least one adapter (loopback)
        EXPECT_GT(adapters.Count(), static_cast<uint16_t>(0));

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_Enumeration)
    {
        ::Thunder::Core::AdapterIterator adapters;
        uint16_t counted = 0;

        while (adapters.Next()) {
            EXPECT_TRUE(adapters.IsValid());
            // Name must not be empty
            EXPECT_FALSE(adapters.Name().empty());
            // Index must be > 0 (kernel assigns positive indices)
            EXPECT_GT(adapters.Index(), static_cast<uint16_t>(0));
            counted++;
        }

        EXPECT_EQ(counted, adapters.Count());

        // After exhausting, IsValid should be false
        EXPECT_FALSE(adapters.IsValid());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_Reset)
    {
        ::Thunder::Core::AdapterIterator adapters;

        // Advance to first adapter
        ASSERT_TRUE(adapters.Next());
        string firstName = adapters.Name();

        // Reset and re-iterate
        adapters.Reset();
        EXPECT_FALSE(adapters.IsValid());
        ASSERT_TRUE(adapters.Next());
        EXPECT_EQ(adapters.Name(), firstName);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_LookupByName_Loopback)
    {
        // Loopback should always exist on any system
        string loopback(LoopbackName());
        ::Thunder::Core::AdapterIterator adapter(loopback);

        // If the platform loopback name doesn't match, try "lo" (Linux)
        if (!adapter.IsValid()) {
            adapter = ::Thunder::Core::AdapterIterator("lo");
        }

        ASSERT_TRUE(adapter.IsValid()) << "Loopback adapter not found";
        EXPECT_TRUE(adapter.Name() == "lo0" || adapter.Name() == "lo");

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_LookupByName_NonExistent)
    {
        ::Thunder::Core::AdapterIterator adapter("this_adapter_does_not_exist_42");

        // Should not be valid – the adapter doesn't exist
        EXPECT_FALSE(adapter.IsValid());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_LookupByIndex)
    {
        // First, discover a valid index
        ::Thunder::Core::AdapterIterator first;
        ASSERT_TRUE(first.Next());
        uint16_t index = first.Index();
        string name = first.Name();

        // Now look it up by index
        ::Thunder::Core::AdapterIterator byIndex(index);
        ASSERT_TRUE(byIndex.IsValid());
        EXPECT_EQ(byIndex.Index(), index);
        EXPECT_EQ(byIndex.Name(), name);

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // AdapterIterator – copy & move semantics
    // =========================================================================

    TEST(Core_NetworkInfo, AdapterIterator_CopyConstruction)
    {
        ::Thunder::Core::AdapterIterator original;
        ASSERT_TRUE(original.Next());
        string origName = original.Name();

        ::Thunder::Core::AdapterIterator copy(original);
        ASSERT_TRUE(copy.IsValid());
        EXPECT_EQ(copy.Name(), origName);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_CopyAssignment)
    {
        ::Thunder::Core::AdapterIterator original;
        ASSERT_TRUE(original.Next());
        string origName = original.Name();

        ::Thunder::Core::AdapterIterator assigned;
        assigned = original;
        ASSERT_TRUE(assigned.IsValid());
        EXPECT_EQ(assigned.Name(), origName);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_MoveConstruction)
    {
        ::Thunder::Core::AdapterIterator original;
        ASSERT_TRUE(original.Next());
        string origName = original.Name();

        ::Thunder::Core::AdapterIterator moved(std::move(original));
        ASSERT_TRUE(moved.IsValid());
        EXPECT_EQ(moved.Name(), origName);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_MoveAssignment)
    {
        ::Thunder::Core::AdapterIterator original;
        ASSERT_TRUE(original.Next());
        string origName = original.Name();

        ::Thunder::Core::AdapterIterator assigned;
        assigned = std::move(original);
        ASSERT_TRUE(assigned.IsValid());
        EXPECT_EQ(assigned.Name(), origName);

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // AdapterIterator – interface flags (IsUp, IsRunning)
    // =========================================================================

    TEST(Core_NetworkInfo, AdapterIterator_LoopbackIsUp)
    {
        string loopback(LoopbackName());
        ::Thunder::Core::AdapterIterator adapter(loopback);
        if (!adapter.IsValid()) {
            adapter = ::Thunder::Core::AdapterIterator("lo");
        }
        ASSERT_TRUE(adapter.IsValid()) << "Loopback adapter not found";

        // Loopback should always be up and running
        EXPECT_TRUE(adapter.IsUp());
        EXPECT_TRUE(adapter.IsRunning());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_FlagsConsistency)
    {
        // For every adapter, if IsRunning() is true then IsUp() must also be true
        ::Thunder::Core::AdapterIterator adapters;
        while (adapters.Next()) {
            if (adapters.IsRunning()) {
                EXPECT_TRUE(adapters.IsUp())
                    << "Adapter " << adapters.Name() << " is Running but not Up";
            }
        }

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // AdapterIterator – MAC address
    // =========================================================================

    TEST(Core_NetworkInfo, AdapterIterator_HasMAC)
    {
        ::Thunder::Core::AdapterIterator adapters;
        bool foundWithMAC = false;
        bool foundWithoutMAC = false;

        while (adapters.Next()) {
            if (adapters.HasMAC()) {
                foundWithMAC = true;

                // String form should have content
                string mac = adapters.MACAddress(':');
                EXPECT_FALSE(mac.empty());
                // XX:XX:XX:XX:XX:XX = 17 chars
                EXPECT_EQ(mac.length(), static_cast<size_t>(17))
                    << "MAC for " << adapters.Name() << " = " << mac;
            } else {
                foundWithoutMAC = true;
            }
        }

        // At least one physical adapter should have a MAC
        EXPECT_TRUE(foundWithMAC) << "No adapter with a MAC address found";
        // Loopback / tunnels should not have a MAC
        EXPECT_TRUE(foundWithoutMAC) << "Expected at least one adapter without MAC (loopback)";

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_MACAddress_Buffer)
    {
        ::Thunder::Core::AdapterIterator adapters;

        while (adapters.Next()) {
            if (adapters.HasMAC()) {
                uint8_t mac[6];
                adapters.MACAddress(mac, sizeof(mac));

                // At least one byte should be non-zero
                bool allZero = true;
                for (int i = 0; i < 6; i++) {
                    if (mac[i] != 0) {
                        allZero = false;
                        break;
                    }
                }
                EXPECT_FALSE(allZero)
                    << "HasMAC() is true but buffer is all zeros for " << adapters.Name();
                break; // one adapter is enough
            }
        }

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterIterator_MACAddress_StringMatchesBuffer)
    {
        ::Thunder::Core::AdapterIterator adapters;

        while (adapters.Next()) {
            if (adapters.HasMAC()) {
                uint8_t mac[6];
                adapters.MACAddress(mac, sizeof(mac));
                string macStr = adapters.MACAddress(':');

                // Reconstruct string from buffer and compare
                char expected[18];
                snprintf(expected, sizeof(expected),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

                EXPECT_EQ(macStr, string(expected))
                    << "For adapter " << adapters.Name();
                break;
            }
        }

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // IPV4AddressIterator
    // =========================================================================

    TEST(Core_NetworkInfo, IPV4AddressIterator_DefaultConstruction)
    {
        ::Thunder::Core::IPV4AddressIterator it;

        EXPECT_FALSE(it.IsValid());
        EXPECT_EQ(it.Count(), static_cast<uint16_t>(0));

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV4AddressIterator_LoopbackHasAddress)
    {
        string loopback(LoopbackName());
        ::Thunder::Core::AdapterIterator adapter(loopback);
        if (!adapter.IsValid()) {
            adapter = ::Thunder::Core::AdapterIterator("lo");
        }
        ASSERT_TRUE(adapter.IsValid()) << "Loopback adapter not found";

        ::Thunder::Core::IPV4AddressIterator ipv4 = adapter.IPV4Addresses();
        EXPECT_GT(ipv4.Count(), static_cast<uint16_t>(0));

        ASSERT_TRUE(ipv4.Next());
        ::Thunder::Core::IPNode addr = ipv4.Address();
        EXPECT_EQ(addr.HostAddress(), "127.0.0.1");

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV4AddressIterator_SubnetMask)
    {
        string loopback(LoopbackName());
        ::Thunder::Core::AdapterIterator adapter(loopback);
        if (!adapter.IsValid()) {
            adapter = ::Thunder::Core::AdapterIterator("lo");
        }
        ASSERT_TRUE(adapter.IsValid());

        ::Thunder::Core::IPV4AddressIterator ipv4 = adapter.IPV4Addresses();
        ASSERT_TRUE(ipv4.Next());

        ::Thunder::Core::IPNode addr = ipv4.Address();
        // Loopback is typically /8 on all platforms
        EXPECT_EQ(addr.Mask(), static_cast<uint8_t>(8))
            << "Loopback mask should be /8, got /" << static_cast<int>(addr.Mask());

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV4AddressIterator_CopyAndMove)
    {
        ::Thunder::Core::AdapterIterator adapter;
        ASSERT_TRUE(FindAdapterWithIPv4(adapter));

        ::Thunder::Core::IPV4AddressIterator original = adapter.IPV4Addresses();
        ASSERT_TRUE(original.Next());
        ::Thunder::Core::IPNode origAddr = original.Address();

        // Copy construction — copy resets position, so Need Next() to advance
        ::Thunder::Core::IPV4AddressIterator copy(original);
        EXPECT_EQ(copy.Count(), original.Count());
        ASSERT_TRUE(copy.Next());
        EXPECT_EQ(copy.Address().HostAddress(), origAddr.HostAddress());

        // Copy assignment
        ::Thunder::Core::IPV4AddressIterator assigned;
        assigned = original;
        EXPECT_EQ(assigned.Count(), original.Count());

        // Move construction — preserves the iteration state
        ::Thunder::Core::IPV4AddressIterator moved(std::move(copy));
        EXPECT_GT(moved.Count(), static_cast<uint16_t>(0));

        // Move assignment
        ::Thunder::Core::IPV4AddressIterator moveAssigned;
        moveAssigned = std::move(assigned);
        EXPECT_GT(moveAssigned.Count(), static_cast<uint16_t>(0));

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV4AddressIterator_Reset)
    {
        ::Thunder::Core::AdapterIterator adapter;
        ASSERT_TRUE(FindAdapterWithIPv4(adapter));

        ::Thunder::Core::IPV4AddressIterator ipv4 = adapter.IPV4Addresses();
        ASSERT_TRUE(ipv4.Next());
        string firstAddr = ipv4.Address().HostAddress();

        ipv4.Reset();
        EXPECT_FALSE(ipv4.IsValid());
        ASSERT_TRUE(ipv4.Next());
        EXPECT_EQ(ipv4.Address().HostAddress(), firstAddr);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV4AddressIterator_FullIteration)
    {
        ::Thunder::Core::AdapterIterator adapter;
        ASSERT_TRUE(FindAdapterWithIPv4(adapter));

        ::Thunder::Core::IPV4AddressIterator ipv4 = adapter.IPV4Addresses();
        uint16_t counted = 0;
        while (ipv4.Next()) {
            EXPECT_TRUE(ipv4.IsValid());
            ::Thunder::Core::IPNode addr = ipv4.Address();
            EXPECT_FALSE(addr.HostAddress().empty());
            counted++;
        }
        EXPECT_EQ(counted, ipv4.Count());
        EXPECT_FALSE(ipv4.IsValid());

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // IPV6AddressIterator
    // =========================================================================

    TEST(Core_NetworkInfo, IPV6AddressIterator_DefaultConstruction)
    {
        ::Thunder::Core::IPV6AddressIterator it;

        EXPECT_FALSE(it.IsValid());
        EXPECT_EQ(it.Count(), static_cast<uint16_t>(0));

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV6AddressIterator_LoopbackHasAddress)
    {
        string loopback(LoopbackName());
        ::Thunder::Core::AdapterIterator adapter(loopback);
        if (!adapter.IsValid()) {
            adapter = ::Thunder::Core::AdapterIterator("lo");
        }
        ASSERT_TRUE(adapter.IsValid()) << "Loopback adapter not found";

        ::Thunder::Core::IPV6AddressIterator ipv6 = adapter.IPV6Addresses();
        // Loopback should have ::1
        EXPECT_GT(ipv6.Count(), static_cast<uint16_t>(0));

        bool foundLoopback = false;
        while (ipv6.Next()) {
            string addr = ipv6.Address().HostAddress();
            if (addr == "::1") {
                foundLoopback = true;
                EXPECT_EQ(ipv6.Address().Mask(), static_cast<uint8_t>(128));
            }
        }
        EXPECT_TRUE(foundLoopback) << "IPv6 loopback address ::1 not found";

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, IPV6AddressIterator_CopyAndMove)
    {
        // Find an adapter with IPv6 addresses
        ::Thunder::Core::AdapterIterator adapters;
        ::Thunder::Core::IPV6AddressIterator original;
        bool found = false;

        while (adapters.Next()) {
            ::Thunder::Core::IPV6AddressIterator ipv6 = adapters.IPV6Addresses();
            if (ipv6.Count() > 0) {
                original = ipv6;
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "No adapter with IPv6 addresses";

        ASSERT_TRUE(original.Next());
        string origAddr = original.Address().HostAddress();

        // Copy — copy constructor resets position
        ::Thunder::Core::IPV6AddressIterator copy(original);
        EXPECT_EQ(copy.Count(), original.Count());
        ASSERT_TRUE(copy.Next());

        // Move
        ::Thunder::Core::IPV6AddressIterator moved(std::move(copy));
        EXPECT_GT(moved.Count(), static_cast<uint16_t>(0));

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // AdapterIterator – cross-adapter enumeration of all addresses
    // =========================================================================

    TEST(Core_NetworkInfo, AdapterIterator_AllAdaptersHaveConsistentAddresses)
    {
        ::Thunder::Core::AdapterIterator adapters;

        while (adapters.Next()) {
            ::Thunder::Core::IPV4AddressIterator ipv4 = adapters.IPV4Addresses();
            ::Thunder::Core::IPV6AddressIterator ipv6 = adapters.IPV6Addresses();

            uint16_t ipv4Count = 0;
            while (ipv4.Next()) {
                ::Thunder::Core::IPNode addr = ipv4.Address();
                // Address should have a valid host string
                EXPECT_FALSE(addr.HostAddress().empty())
                    << "Empty IPv4 on adapter " << adapters.Name();
                ipv4Count++;
            }
            EXPECT_EQ(ipv4Count, adapters.IPV4Addresses().Count());

            uint16_t ipv6Count = 0;
            while (ipv6.Next()) {
                ::Thunder::Core::IPNode addr = ipv6.Address();
                EXPECT_FALSE(addr.HostAddress().empty())
                    << "Empty IPv6 on adapter " << adapters.Name();
                ipv6Count++;
            }
            EXPECT_EQ(ipv6Count, adapters.IPV6Addresses().Count());
        }

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // RoutingTable
    // =========================================================================

    TEST(Core_NetworkInfo, RoutingTable_IPv4)
    {
        // Should not crash or throw. On any connected machine there should
        // be at least one route (the default route or a local subnet route).
        ::Thunder::Core::RoutingTable routes(true /* ipv4 */);

        // If we got here without crash, the sysctl/Netlink parsing works.
        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, RoutingTable_IPv6)
    {
        ::Thunder::Core::RoutingTable routes(false /* ipv6 */);

        ::Thunder::Core::Singleton::Dispose();
    }

    // =========================================================================
    // AdapterObserver
    // =========================================================================

    class TestNotification : public ::Thunder::Core::AdapterObserver::INotification {
    public:
        TestNotification() : _eventCount(0) {}

        void Event(const string& /* name */) override { _eventCount++; }
        void Added(const string& /* name */, const ::Thunder::Core::IPNode& /* node */) override {}
        void Removed(const string& /* name */, const ::Thunder::Core::IPNode& /* node */) override {}

        uint32_t EventCount() const { return _eventCount; }

    private:
        uint32_t _eventCount;
    };

    TEST(Core_NetworkInfo, AdapterObserver_OpenClose)
    {
        TestNotification notification;
        ::Thunder::Core::AdapterObserver observer(&notification);

        EXPECT_EQ(observer.Open(), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(observer.Close(), ::Thunder::Core::ERROR_NONE);

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(Core_NetworkInfo, AdapterObserver_DoubleClose)
    {
        TestNotification notification;
        ::Thunder::Core::AdapterObserver observer(&notification);

        EXPECT_EQ(observer.Open(), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(observer.Close(), ::Thunder::Core::ERROR_NONE);
        // Second close should not crash
        EXPECT_EQ(observer.Close(), ::Thunder::Core::ERROR_NONE);

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder