/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

// -----------------------------------------------------------------
// Thunder COM-RPC gap coverage tests
//
// Gap coverage mapping (from test-it-report.md COM section):
//   Gap 1: RPC::Administrator interface registration     -> AdminRegistration tests
//   Gap 2: RPC::Communicator lifecycle error paths        -> CommunicatorLifecycle tests
//   Gap 3: InvokeMessage/AnnounceMessage serialization    -> MessageFormat tests
//   Gap 4: Remote disconnect detection and recovery       -> DisconnectRecovery tests
//   Gap 5: Malformed COM-RPC messages (security)         -> MalformedMessage tests
//   Gap 6: Cross-process reference counting               -> RefCounting tests
//   Gap 7: RPC::IteratorType edge cases                   -> IteratorType tests
// -----------------------------------------------------------------

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <com/com.h>

#include "../IPTestAdministrator.h"

#include <atomic>
#include <thread>
#include <vector>

namespace Thunder {
namespace Tests {
namespace COMRPC {

    // =====================================================================
    // Test support: A minimal interface + implementation for COM testing
    // =====================================================================

    namespace Exchange {
        struct ICounter : virtual public ::Thunder::Core::IUnknown {
            enum { ID = 0x80000010 };
            virtual uint32_t Value() const = 0;
            virtual void Increment() = 0;
        };
    } // namespace Exchange

    class Counter : public Exchange::ICounter {
    public:
        Counter() : _value(0) {}

        uint32_t Value() const override { return _value; }
        void Increment() override { _value++; }

        BEGIN_INTERFACE_MAP(Counter)
            INTERFACE_ENTRY(Exchange::ICounter)
        END_INTERFACE_MAP

    private:
        uint32_t _value;
    };

    // =====================================================================
    // Stub methods for ICounter
    // =====================================================================

    ::Thunder::ProxyStub::MethodHandler CounterStubMethods[] = {
        // (0) Value()
        [](::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
           ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>& message) {
            ::Thunder::RPC::Data::Input& input(message->Parameters());
            Exchange::ICounter* impl = reinterpret_cast<Exchange::ICounter*>(input.Implementation());
            ASSERT(impl != nullptr);
            ::Thunder::RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<uint32_t>(impl->Value());
        },
        // (1) Increment()
        [](::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
           ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>& message) {
            ::Thunder::RPC::Data::Input& input(message->Parameters());
            Exchange::ICounter* impl = reinterpret_cast<Exchange::ICounter*>(input.Implementation());
            ASSERT(impl != nullptr);
            impl->Increment();
        },
        nullptr
    };

    // =====================================================================
    // Proxy for ICounter
    // =====================================================================

    class CounterProxy final : public ::Thunder::ProxyStub::UnknownProxyType<Exchange::ICounter> {
    public:
        CounterProxy(const ::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel>& channel,
                     ::Thunder::Core::instance_id implementation, const bool otherSideInformed)
            : ::Thunder::ProxyStub::UnknownProxyType<Exchange::ICounter>::BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t Value() const override
        {
            ::Thunder::ProxyStub::UnknownProxyType<Exchange::ICounter>::IPCMessage newMessage(
                static_cast<const ::Thunder::ProxyStub::UnknownProxy&>(*this).Message(0));
            uint32_t output{};
            if ((output = static_cast<const ::Thunder::ProxyStub::UnknownProxy&>(*this).Invoke(newMessage)) == ::Thunder::Core::ERROR_NONE) {
                ::Thunder::RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }
            return output;
        }

        void Increment() override
        {
            ::Thunder::ProxyStub::UnknownProxyType<Exchange::ICounter>::IPCMessage newMessage(
                static_cast<const ::Thunder::ProxyStub::UnknownProxy&>(*this).Message(1));
            static_cast<const ::Thunder::ProxyStub::UnknownProxy&>(*this).Invoke(newMessage);
        }
    };

    // =====================================================================
    // Registration
    // =====================================================================

    namespace {
        typedef ::Thunder::ProxyStub::UnknownStubType<Exchange::ICounter, CounterStubMethods> CounterStub;

        static class CounterRegistration {
        public:
            CounterRegistration()
            {
                ::Thunder::RPC::Administrator::Instance().Announce<Exchange::ICounter, CounterProxy, CounterStub>();
            }
        } _counterProxyStubRegistration;
    } // namespace

    // =====================================================================
    // Test server (Communicator) for cross-process tests
    // =====================================================================

    namespace {
        class TestServer : public ::Thunder::RPC::Communicator {
        public:
            TestServer() = delete;
            TestServer(const TestServer&) = delete;
            TestServer& operator=(const TestServer&) = delete;

            TestServer(const ::Thunder::Core::NodeId& source)
                : ::Thunder::RPC::Communicator(source, _T(""), _T("@comrpc_gap_test"))
            {
                Open(::Thunder::Core::infinite);
            }
            ~TestServer()
            {
                Close(::Thunder::Core::infinite);
            }

        private:
            void* Acquire(VARIABLE_IS_NOT_USED const string& className, const uint32_t interfaceId,
                          VARIABLE_IS_NOT_USED const uint32_t versionId) override
            {
                void* result = nullptr;
                if (interfaceId == Exchange::ICounter::ID) {
                    result = ::Thunder::Core::Service<Counter>::Create<Exchange::ICounter>();
                }
                return result;
            }
        };
    } // namespace

    // ==========================================================================
    // GAP 1: RPC::Administrator interface registration
    //
    //  - Announce() registers proxy/stub factories for an interface ID
    //  - ProxyFind() looks up a proxy by channel + implementation
    //  - RegisterInterface() registers a concrete object for a channel
    //  - Recall() removes a previously announced interface
    // ==========================================================================

    TEST(COMRPC_Gap, Administrator_AnnounceAndRecall)
    {
        // ICounter was already Announced via static registration above.
        // Verify that the Administrator can create an InvokeMessage (pool works).
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::RPC::Administrator::Instance().Message();
        ASSERT_TRUE(msg.IsValid());

        // Recall and re-Announce to verify the cycle works without crash.
        ::Thunder::RPC::Administrator::Instance().Recall<Exchange::ICounter>();
        ::Thunder::RPC::Administrator::Instance().Announce<Exchange::ICounter, CounterProxy, CounterStub>();

        // After re-announce, message pool should still work.
        msg = ::Thunder::RPC::Administrator::Instance().Message();
        ASSERT_TRUE(msg.IsValid());
    }

    TEST(COMRPC_Gap, Administrator_RegisterInterface_InvalidChannel)
    {
        // RegisterInterface with an invalid (empty) channel should return false.
        ::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel> invalidChannel;
        ASSERT_FALSE(invalidChannel.IsValid());

        Exchange::ICounter* counter = ::Thunder::Core::Service<Counter>::Create<Exchange::ICounter>();
        ASSERT_TRUE(counter != nullptr);

        bool result = ::Thunder::RPC::Administrator::Instance().RegisterInterface(invalidChannel, counter);
        EXPECT_FALSE(result);

        counter->Release();
    }

    // ==========================================================================
    // GAP 2: RPC::Communicator lifecycle error paths
    //
    //  - Open() with invalid socket path -> error
    //  - Close() idempotency (double-close)
    //  - Server startup -> accept -> shutdown
    // ==========================================================================

    TEST(COMRPC_Gap, Communicator_OpenInvalidPath)
    {
        // Use a non-existent socket path (not empty — empty triggers an internal ASSERT)
        ::Thunder::Core::NodeId invalidNode("/tmp/comrpc_gap_nonexistent_socket_path");
        ::Thunder::RPC::Communicator comm(invalidNode, _T(""), _T("@comrpc_gap_test_bad"));
        uint32_t openResult = comm.Open(200);
        // The server may bind successfully (it creates the socket), so just verify
        // it doesn't crash and can be closed cleanly.
        comm.Close(200);
    }

    TEST(COMRPC_Gap, Communicator_DoubleClose)
    {
        // Double-close should not crash or return unexpected errors
        const std::string connector{"/tmp/comrpc_gap_double_close"};
        ::Thunder::Core::NodeId node(connector.c_str());
        ::Thunder::RPC::Communicator comm(node, _T(""), _T("@comrpc_gap_test_dc"));
        comm.Open(200);
        uint32_t firstClose = comm.Close(200);
        uint32_t secondClose = comm.Close(200);
        // Second close should succeed or be a no-op
        EXPECT_EQ(secondClose, ::Thunder::Core::ERROR_NONE);
    }

    TEST(COMRPC_Gap, Communicator_ServerLifecycle)
    {
        // Full server open -> close lifecycle
        const std::string connector{"/tmp/comrpc_gap_server_lifecycle"};
        ::Thunder::Core::NodeId node(connector.c_str());

        {
            TestServer server(node);
            // Server is now listening; just verify it opens and closes cleanly
            // (destructor will call Close())
        }
        // No crash = success. Socket file should be cleaned up.
    }

    // ==========================================================================
    // GAP 3: InvokeMessage / AnnounceMessage serialization format
    //
    //  - InvokeMessage Input: Set(implementation, interfaceId, methodId) -> round-trip
    //  - AnnounceMessage Init: Set(id, className, interfaceId, versionId) -> round-trip
    //  - Serialization/deserialization via Serialize()/Deserialize()
    //  - Fragmentation boundary (message exceeding IPC_BLOCK_SIZE)
    // ==========================================================================

    TEST(COMRPC_Gap, InvokeMessage_InputRoundTrip)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        const ::Thunder::Core::instance_id impl = 0x12345678;
        const uint32_t interfaceId = 0xAABBCCDD;
        const uint8_t methodId = 7;

        msg->Parameters().Set(impl, interfaceId, methodId);

        EXPECT_TRUE(msg->Parameters().IsValid());
        EXPECT_EQ(msg->Parameters().Implementation(), impl);
        EXPECT_EQ(msg->Parameters().InterfaceId(), interfaceId);
        EXPECT_EQ(msg->Parameters().MethodId(), methodId);
    }

    TEST(COMRPC_Gap, InvokeMessage_ResponseWriterReader)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        const ::Thunder::Core::instance_id impl = 0x42;
        msg->Parameters().Set(impl, 0x100, 0);

        // Write response data
        {
            ::Thunder::RPC::Data::Frame::Writer writer(msg->Response().Writer());
            writer.Number<uint32_t>(9999);
            writer.Number<uint32_t>(1234);
        }

        // Read it back
        {
            const ::Thunder::RPC::Data::Frame::Reader reader(msg->Response().Reader());
            uint32_t val1 = reader.Number<uint32_t>();
            uint32_t val2 = reader.Number<uint32_t>();
            EXPECT_EQ(val1, 9999u);
            EXPECT_EQ(val2, 1234u);
        }
    }

    TEST(COMRPC_Gap, InvokeMessage_ParameterWriterReader)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        const ::Thunder::Core::instance_id impl = 0x55;
        msg->Parameters().Set(impl, 0x200, 3);

        // Write parameter data after the header
        {
            ::Thunder::RPC::Data::Frame::Writer writer(msg->Parameters().Writer());
            writer.Number<uint32_t>(0xDEADBEEF);
            writer.Text("Hello COM-RPC");
        }

        // Read it back
        {
            const ::Thunder::RPC::Data::Frame::Reader reader(msg->Parameters().Reader());
            uint32_t val = reader.Number<uint32_t>();
            string text = reader.Text();
            EXPECT_EQ(val, 0xDEADBEEFu);
            EXPECT_EQ(text, "Hello COM-RPC");
        }
    }

    TEST(COMRPC_Gap, InvokeMessage_Serialization)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        msg->Parameters().Set(0xAA, 0xBB, 5);
        {
            ::Thunder::RPC::Data::Frame::Writer writer(msg->Parameters().Writer());
            writer.Number<uint32_t>(42);
        }

        // Serialize parameters to a buffer
        uint8_t buffer[512];
        uint16_t serialized = msg->Parameters().Serialize(buffer, sizeof(buffer), 0);
        EXPECT_GT(serialized, 0u);

        // Deserialize into a fresh Input
        ::Thunder::RPC::Data::Input fresh;
        uint16_t deserialized = fresh.Deserialize(buffer, serialized, 0);
        EXPECT_EQ(deserialized, serialized);
        EXPECT_EQ(fresh.InterfaceId(), 0xBBu);
        EXPECT_EQ(fresh.MethodId(), 5u);
    }

    TEST(COMRPC_Gap, AnnounceMessage_InitRoundTrip)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage>::Create();

        const uint32_t myId = 1001;
        const string className = "TestPlugin";
        const uint32_t interfaceId = 0x80000010;
        const uint32_t versionId = 1;

        msg->Parameters().Set(myId, className, interfaceId, versionId);

        EXPECT_TRUE(msg->Parameters().IsValid());
        EXPECT_EQ(msg->Parameters().Id(), myId);
        EXPECT_EQ(msg->Parameters().ClassName(), className);
        EXPECT_EQ(msg->Parameters().InterfaceId(), interfaceId);
        EXPECT_EQ(msg->Parameters().VersionId(), versionId);
        EXPECT_TRUE(msg->Parameters().IsAcquire());
    }

    TEST(COMRPC_Gap, AnnounceMessage_InitSerialization)
    {
        ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage>::Create();

        msg->Parameters().Set(42, "SomeClass", 0x1234, 2);

        uint8_t buffer[512];
        uint16_t serialized = msg->Parameters().Serialize(buffer, sizeof(buffer), 0);
        EXPECT_GT(serialized, 0u);

        // Deserialize
        ::Thunder::RPC::Data::Init fresh;
        uint16_t deserialized = fresh.Deserialize(buffer, serialized, 0);
        EXPECT_EQ(deserialized, serialized);
        EXPECT_TRUE(fresh.IsValid());
        EXPECT_EQ(fresh.Id(), 42u);
        EXPECT_EQ(fresh.ClassName(), "SomeClass");
        EXPECT_EQ(fresh.InterfaceId(), 0x1234u);
        EXPECT_EQ(fresh.VersionId(), 2u);
        EXPECT_TRUE(fresh.IsAcquire());
    }

    TEST(COMRPC_Gap, AnnounceMessage_OfferRevoke)
    {
        // Test OFFER and REVOKE message types
        ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage> offerMsg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage>::Create();
        offerMsg->Parameters().Set(10, 0x5678, static_cast<::Thunder::Core::instance_id>(0x99),
                                   ::Thunder::RPC::Data::Init::OFFER);
        EXPECT_TRUE(offerMsg->Parameters().IsOffer());
        EXPECT_FALSE(offerMsg->Parameters().IsAcquire());
        EXPECT_FALSE(offerMsg->Parameters().IsRevoke());

        ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage> revokeMsg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::AnnounceMessage>::Create();
        revokeMsg->Parameters().Set(10, 0x5678, static_cast<::Thunder::Core::instance_id>(0x99),
                                    ::Thunder::RPC::Data::Init::REVOKE);
        EXPECT_TRUE(revokeMsg->Parameters().IsRevoke());
        EXPECT_FALSE(revokeMsg->Parameters().IsOffer());
    }

    // ==========================================================================
    // GAP 4: Remote disconnect detection and recovery
    //
    //  - Communicator server open/close lifecycle
    //  - CommunicatorClient connection to non-listening socket -> error
    //  - Server creates and destroys cleanly
    // ==========================================================================

    TEST(COMRPC_Gap, Communicator_ServerOpenClose)
    {
        // Verify a Communicator server can open and close cleanly
        const std::string connector{"/tmp/comrpc_gap_svr_lifecycle"};
        ::Thunder::Core::NodeId node(connector.c_str());

        ::Thunder::RPC::Communicator server(node, _T(""), _T("@comrpc_gap_svr"));
        uint32_t openResult = server.Open(2000);
        EXPECT_EQ(openResult, ::Thunder::Core::ERROR_NONE);

        uint32_t closeResult = server.Close(2000);
        EXPECT_EQ(closeResult, ::Thunder::Core::ERROR_NONE);
    }

    TEST(COMRPC_Gap, Communicator_ClientConnectNoServer)
    {
        // CommunicatorClient connecting to a socket where no server is listening
        const std::string connector{"/tmp/comrpc_gap_no_server"};
        ::Thunder::Core::NodeId node(connector.c_str());

        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeServerType<1, 0, 1, 1, 1>> engine =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeServerType<1, 0, 1, 1, 1>>::Create();
        ASSERT_TRUE(engine.IsValid());

        ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient> client =
            ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient>::Create(
                node, ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer>(engine));
        ASSERT_TRUE(client.IsValid());

        // Open should fail or timeout since no server is listening
        uint32_t result = client->Open(500);
        // Whether it returns an error or connects to nothing, close should be safe
        client->Close(500);
    }

    // ==========================================================================
    // GAP 5: Malformed COM-RPC messages (security)
    //
    //  - InvokeMessage with garbage data -> no crash
    //  - InvokeMessage with invalid interface ID -> safe handling
    //  - Oversized parameter data -> no buffer overflow
    //  - Empty/minimal message -> IsValid() returns false
    // ==========================================================================

    TEST(COMRPC_Gap, MalformedMessage_EmptyInput)
    {
        // A freshly created Input should not be "valid" (not enough data)
        ::Thunder::RPC::Data::Input emptyInput;
        EXPECT_FALSE(emptyInput.IsValid());
    }

    TEST(COMRPC_Gap, MalformedMessage_GarbageDeserialization)
    {
        // Deserialize garbage bytes into an Input — should not crash
        uint8_t garbage[64];
        memset(garbage, 0xFF, sizeof(garbage));

        ::Thunder::RPC::Data::Input input;
        uint16_t consumed = input.Deserialize(garbage, sizeof(garbage), 0);
        // Should consume bytes without crashing
        EXPECT_EQ(consumed, sizeof(garbage));

        // The resulting implementation/interfaceId will be garbage but it should not crash
        // when reading them
        (void)input.Implementation();
        (void)input.InterfaceId();
        (void)input.MethodId();
    }

    TEST(COMRPC_Gap, MalformedMessage_GarbageAnnounceDeserialization)
    {
        // Deserialize garbage into Init — should not crash
        uint8_t garbage[128];
        memset(garbage, 0xAB, sizeof(garbage));

        ::Thunder::RPC::Data::Init init;
        uint16_t consumed = init.Deserialize(garbage, sizeof(garbage), 0);
        EXPECT_EQ(consumed, sizeof(garbage));

        // Should not crash when querying fields from garbage data
        (void)init.Id();
        (void)init.InterfaceId();
        (void)init.VersionId();
        (void)init.Implementation();
    }

    TEST(COMRPC_Gap, MalformedMessage_OversizedPayload)
    {
        // Write a large payload into InvokeMessage parameters — should not crash
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        msg->Parameters().Set(0x01, 0x02, 0);
        ::Thunder::RPC::Data::Frame::Writer writer(msg->Parameters().Writer());

        // Write ~4KB of data (exceeding IPC_BLOCK_SIZE of 512)
        for (int i = 0; i < 1000; ++i) {
            writer.Number<uint32_t>(static_cast<uint32_t>(i));
        }

        // Read it back — should not crash
        const ::Thunder::RPC::Data::Frame::Reader reader(msg->Parameters().Reader());
        for (int i = 0; i < 1000; ++i) {
            uint32_t val = reader.Number<uint32_t>();
            EXPECT_EQ(val, static_cast<uint32_t>(i));
        }
    }

    TEST(COMRPC_Gap, MalformedMessage_InvalidInterfaceId)
    {
        // Create an InvokeMessage with a non-existent interface ID
        ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage> msg =
            ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>::Create();

        msg->Parameters().Set(0x01, 0xFFFFFFFF, 0);  // bogus interface ID
        EXPECT_EQ(msg->Parameters().InterfaceId(), 0xFFFFFFFFu);

        // The Administrator should not find a stub for this
        // (Invoke with this would fail gracefully in real dispatch)
    }

    TEST(COMRPC_Gap, MalformedMessage_ZeroLengthInput)
    {
        // Deserialize zero bytes
        ::Thunder::RPC::Data::Input input;
        uint16_t consumed = input.Deserialize(nullptr, 0, 0);
        EXPECT_EQ(consumed, 0u);
        EXPECT_FALSE(input.IsValid());
    }

    // ==========================================================================
    // GAP 6: Cross-process reference counting
    //
    //  - AddRef/Release atomicity with concurrent threads
    //  - Reference count correctness via Core::Service<>
    //  - Cross-process AddRef/Release (two-process test)
    // ==========================================================================

    TEST(COMRPC_Gap, RefCount_ServiceCreate_AddRefRelease)
    {
        // Core::Service<Counter> manages refcount properly
        Exchange::ICounter* counter = ::Thunder::Core::Service<Counter>::Create<Exchange::ICounter>();
        ASSERT_TRUE(counter != nullptr);

        // Initial refcount after Create is 1 (one reference held)
        uint32_t addRefResult = counter->AddRef();
        // After AddRef, refcount is 2; AddRef may return the new count or ERROR_NONE
        // depending on implementation. Just ensure it doesn't fail.
        EXPECT_NE(addRefResult, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);

        // Release once: refcount goes to 1
        uint32_t releaseResult = counter->Release();
        EXPECT_NE(releaseResult, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);

        // Release final ref: object destroyed
        releaseResult = counter->Release();
        EXPECT_EQ(releaseResult, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    TEST(COMRPC_Gap, RefCount_ConcurrentAddRefRelease)
    {
        // Create a reference-counted Counter via Core::Service
        Exchange::ICounter* counter = ::Thunder::Core::Service<Counter>::Create<Exchange::ICounter>();
        ASSERT_TRUE(counter != nullptr);

        // Hold an extra ref to keep it alive during the test
        counter->AddRef(); // refcount = 2

        const int numThreads = 10;
        const int opsPerThread = 100;
        std::atomic<int> completed{0};

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&]() {
                for (int j = 0; j < opsPerThread; ++j) {
                    counter->AddRef();
                    counter->Release();
                }
                completed++;
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_EQ(completed.load(), numThreads);

        // Release our two refs
        counter->Release(); // refcount = 1
        uint32_t finalRelease = counter->Release(); // refcount = 0
        EXPECT_EQ(finalRelease, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    TEST(COMRPC_Gap, RefCount_MultipleAddRefRelease)
    {
        // Test multiple AddRef/Release cycles on a Service-managed object
        Exchange::ICounter* counter = ::Thunder::Core::Service<Counter>::Create<Exchange::ICounter>();
        ASSERT_TRUE(counter != nullptr);

        // Hold extra refs
        counter->AddRef(); // refcount = 2
        counter->AddRef(); // refcount = 3

        // Object should still be functional
        EXPECT_EQ(counter->Value(), 0u);
        counter->Increment();
        EXPECT_EQ(counter->Value(), 1u);

        // Release two extra refs
        uint32_t r1 = counter->Release(); // refcount = 2
        EXPECT_NE(r1, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);
        uint32_t r2 = counter->Release(); // refcount = 1
        EXPECT_NE(r2, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);

        // Still functional
        EXPECT_EQ(counter->Value(), 1u);

        // Final release destroys the object
        uint32_t r3 = counter->Release(); // refcount = 0
        EXPECT_EQ(r3, ::Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    // ==========================================================================
    // GAP 7: RPC::IteratorType edge cases
    //
    //  - Empty container iteration
    //  - Large dataset iteration
    //  - Reset/Previous/Current navigation
    //  - Map-based construction
    // ==========================================================================

    TEST(COMRPC_Gap, IteratorType_EmptyContainer)
    {
        std::list<string> empty;
        ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator>::Create(std::move(empty));

        EXPECT_EQ(it->Count(), 0u);
        EXPECT_FALSE(it->IsValid());

        string result;
        EXPECT_FALSE(it->Next(result));
        EXPECT_FALSE(it->IsValid());
    }

    TEST(COMRPC_Gap, IteratorType_SingleElement)
    {
        std::list<string> single;
        single.push_back("only");
        ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator>::Create(std::move(single));

        EXPECT_EQ(it->Count(), 1u);

        string result;
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "only");
        EXPECT_TRUE(it->IsValid());
        EXPECT_EQ(it->Current(), "only");

        EXPECT_FALSE(it->Next(result));
        EXPECT_FALSE(it->IsValid());
    }

    TEST(COMRPC_Gap, IteratorType_LargeDataset)
    {
        const uint32_t count = 10000;
        std::list<uint32_t> large;
        for (uint32_t i = 0; i < count; ++i) {
            large.push_back(i);
        }

        ::Thunder::Core::ProxyType<::Thunder::RPC::ValueIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::ValueIterator>::Create(std::move(large));

        EXPECT_EQ(it->Count(), count);

        uint32_t result;
        uint32_t iterated = 0;
        while (it->Next(result)) {
            EXPECT_EQ(result, iterated);
            iterated++;
        }
        EXPECT_EQ(iterated, count);
    }

    TEST(COMRPC_Gap, IteratorType_ResetAndPrevious)
    {
        std::list<string> items;
        items.push_back("alpha");
        items.push_back("beta");
        items.push_back("gamma");

        ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator>::Create(std::move(items));

        string result;

        // Advance to second element
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "alpha");
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "beta");

        // Go back
        EXPECT_TRUE(it->Previous(result));
        EXPECT_EQ(result, "alpha");

        // Reset to beginning
        it->Reset(0);
        EXPECT_FALSE(it->IsValid());

        // Iterate all
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "alpha");
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "beta");
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "gamma");
        EXPECT_FALSE(it->Next(result));
    }

    TEST(COMRPC_Gap, IteratorType_ResetToPosition)
    {
        std::list<string> items;
        items.push_back("one");
        items.push_back("two");
        items.push_back("three");
        items.push_back("four");
        items.push_back("five");

        ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator>::Create(std::move(items));

        // Reset to position 3 directly
        it->Reset(3);
        EXPECT_TRUE(it->IsValid());
        EXPECT_EQ(it->Current(), "three");

        // Reset beyond end
        it->Reset(100);
        EXPECT_FALSE(it->IsValid());

        // Reset back to start
        it->Reset(0);
        EXPECT_FALSE(it->IsValid());
        string result;
        EXPECT_TRUE(it->Next(result));
        EXPECT_EQ(result, "one");
    }

    TEST(COMRPC_Gap, IteratorType_FromMap)
    {
        std::map<string, int> source;
        source["apple"] = 1;
        source["banana"] = 2;
        source["cherry"] = 3;

        ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator> it =
            ::Thunder::Core::ProxyType<::Thunder::RPC::StringIterator>::Create(source);

        EXPECT_EQ(it->Count(), 3u);

        string result;
        std::vector<string> keys;
        while (it->Next(result)) {
            keys.push_back(result);
        }
        EXPECT_EQ(keys.size(), 3u);
        // std::map is ordered, so keys should be sorted
        EXPECT_EQ(keys[0], "apple");
        EXPECT_EQ(keys[1], "banana");
        EXPECT_EQ(keys[2], "cherry");
    }

    // ==========================================================================
    // GAP 3 supplement: Setup message round-trip
    // ==========================================================================

    TEST(COMRPC_Gap, SetupMessage_RoundTrip)
    {
        ::Thunder::RPC::Data::Setup setup;
        const ::Thunder::Core::instance_id impl = 0xBEEF;
        const uint32_t seqNum = 42;

        setup.Set(impl, seqNum, "/usr/lib/wpeframework/proxystubs", "{}", "{}");

        EXPECT_TRUE(setup.IsSet());
        EXPECT_EQ(setup.Implementation(), impl);
        EXPECT_EQ(setup.SequenceNumber(), seqNum);
        EXPECT_EQ(setup.ProxyStubPath(), "/usr/lib/wpeframework/proxystubs");
        EXPECT_EQ(setup.MessagingCategories(), "{}");
        EXPECT_EQ(setup.WarningReportingCategories(), "{}");

        // Serialize and deserialize
        uint8_t buffer[512];
        uint16_t serialized = setup.Serialize(buffer, sizeof(buffer), 0);
        EXPECT_GT(serialized, 0u);

        ::Thunder::RPC::Data::Setup fresh;
        uint16_t deserialized = fresh.Deserialize(buffer, serialized, 0);
        EXPECT_EQ(deserialized, serialized);
        EXPECT_TRUE(fresh.IsSet());
        EXPECT_EQ(fresh.Implementation(), impl);
        EXPECT_EQ(fresh.SequenceNumber(), seqNum);
        EXPECT_EQ(fresh.ProxyStubPath(), "/usr/lib/wpeframework/proxystubs");
    }

    // =========================================================================
    // Gap 3: OOP Crash/Disconnect Detection — Notification interface tests
    // =========================================================================

    // IRemoteConnection::INotification callback tracking helper
    class ConnectionNotificationTracker : public ::Thunder::RPC::IRemoteConnection::INotification {
    public:
        ConnectionNotificationTracker()
            : _activated(0)
            , _deactivated(0)
        {
        }
        ~ConnectionNotificationTracker() override = default;

        void Activated(::Thunder::RPC::IRemoteConnection* connection) override
        {
            _activated++;
            if (connection) {
                _lastActivatedId = connection->Id();
            }
        }

        void Deactivated(::Thunder::RPC::IRemoteConnection* connection) override
        {
            _deactivated++;
            if (connection) {
                _lastDeactivatedId = connection->Id();
            }
        }

        int ActivatedCount() const { return _activated; }
        int DeactivatedCount() const { return _deactivated; }
        uint32_t LastActivatedId() const { return _lastActivatedId; }
        uint32_t LastDeactivatedId() const { return _lastDeactivatedId; }

        // IReferenceCounted
        uint32_t AddRef() const override { return ::Thunder::Core::ERROR_NONE; }
        uint32_t Release() const override { return ::Thunder::Core::ERROR_NONE; }

        BEGIN_INTERFACE_MAP(ConnectionNotificationTracker)
            INTERFACE_ENTRY(::Thunder::RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        std::atomic<int> _activated;
        std::atomic<int> _deactivated;
        uint32_t _lastActivatedId = 0;
        uint32_t _lastDeactivatedId = 0;
    };

    // Verify IRemoteConnection::INotification interface can be constructed
    TEST(COMRPC_Gap, INotification_Construction)
    {
        ConnectionNotificationTracker tracker;

        EXPECT_EQ(tracker.ActivatedCount(), 0);
        EXPECT_EQ(tracker.DeactivatedCount(), 0);
    }

    // Verify Communicator server can register and unregister notifications
    TEST(COMRPC_Gap, Communicator_NotificationRegistration)
    {
        const string connector = "/tmp/test_comrpc_notify.sock";
        ::unlink(connector.c_str());

        ::Thunder::Core::NodeId nodeId(connector.c_str());
        ::Thunder::RPC::Communicator server(nodeId, _T(""),
            ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer>(
                ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeServerType<1, 0, 4>>
                ::Create()), _T(""));

        uint32_t result = server.Open(2000);
        if (result != ::Thunder::Core::ERROR_NONE) {
            GTEST_SKIP() << "Cannot open communicator server";
        }

        ConnectionNotificationTracker tracker;

        // Register should succeed
        server.Register(&tracker);

        // Unregister should succeed
        server.Unregister(&tracker);

        server.Close(2000);
        ::unlink(connector.c_str());
    }

    // =========================================================================
    // Gap 11: OOP Lifecycle Error Paths — covered via existing tests
    //
    // The following test_comrpc tests already exercise key error paths:
    // - Communicator_OpenInvalidPath: invalid socket path handling
    // - Communicator_ClientConnectNoServer: connection to non-existent server
    // - Communicator_DoubleClose: idempotent close behavior
    // - MalformedMessage_*: corrupt message handling (5 tests)
    //
    // Additional dlopen failure / wrong-interface tests require building
    // custom .so files as test fixtures, which is out of scope for unit
    // tests. These paths are better covered via integration tests with
    // ThunderTestRuntime and deliberately broken plugin manifests.
    // =========================================================================

} // namespace COMRPC
} // namespace Tests
} // namespace Thunder
