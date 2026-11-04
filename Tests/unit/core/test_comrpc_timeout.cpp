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
// COM-RPC IPC timeout behaviour (UnknownProxy::Invoke)
//
// A proxy owned by the COM-RPC server side (Thunder) is reached over a Communicator::Client
// channel, a proxy owned by a client is reached over a CommunicatorClient channel. When an
// invoke times out:
//
//   - server side proxy: aborts if built with COMRPC_ABORT_ON_TIMEOUT, otherwise the socket
//     to the remote process is shut down.
//   - client side proxy: returns the timeout, never aborts and never terminates the process.
//
// Every scenario runs as a gtest death test in "threadsafe" mode, so the scenario starts in a
// freshly executed, still single threaded process and the peer process can be forked safely
// before any Thunder thread exists.
// -----------------------------------------------------------------

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <com/com.h>

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>

#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace Thunder {
namespace Tests {
namespace COMRPCTimeout {

    namespace {

        constexpr uint32_t BlockTimeMs = 1500; // time the remote implementation is blocked
        constexpr uint32_t InvokeTimeoutMs = 200; // time the proxy is willing to wait, shorter than BlockTimeMs
        constexpr uint32_t SetupTimeoutMs = 5000;
        constexpr uint32_t PeerClosedTimeoutMs = 3000;

        // Exit codes used by the scenarios, anything other than 0 is reported as a failure by the death test.
        enum ExitCode : int {
            SUCCESS = 0,
            NO_CONNECTION = 10,
            NO_INTERFACE = 11,
            UNEXPECTED_RESULT = 12,
            PEER_NOT_CLOSED = 13,
            PEER_FAILED = 14
        };

        // =====================================================================
        // Test interface: a single method that blocks in the remote process
        // =====================================================================

        namespace Exchange {
            struct IBlocker : virtual public ::Thunder::Core::IUnknown {
                enum { ID = 0x80000020 };
                virtual uint32_t Block(const uint32_t durationMs) = 0;
            };
        }

        class Blocker : public Exchange::IBlocker {
        public:
            uint32_t Block(const uint32_t durationMs) override
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
                return (::Thunder::Core::ERROR_NONE);
            }

            BEGIN_INTERFACE_MAP(Blocker)
                INTERFACE_ENTRY(Exchange::IBlocker)
            END_INTERFACE_MAP
        };

        ::Thunder::ProxyStub::MethodHandler BlockerStubMethods[] = {
            // (0) Block()
            [](::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
               ::Thunder::Core::ProxyType<::Thunder::RPC::InvokeMessage>& message) {
                ::Thunder::RPC::Data::Input& input(message->Parameters());
                Exchange::IBlocker* impl = reinterpret_cast<Exchange::IBlocker*>(input.Implementation());
                ASSERT(impl != nullptr);
                ::Thunder::RPC::Data::Frame::Reader parameters(input.Reader());
                const uint32_t durationMs = parameters.Number<uint32_t>();
                ::Thunder::RPC::Data::Frame::Writer writer(message->Response().Writer());
                writer.Number<uint32_t>(impl->Block(durationMs));
            },
            nullptr
        };

        class BlockerProxy final : public ::Thunder::ProxyStub::UnknownProxyType<Exchange::IBlocker> {
        public:
            BlockerProxy(const ::Thunder::Core::ProxyType<::Thunder::Core::IPCChannel>& channel,
                         ::Thunder::Core::instance_id implementation, const bool otherSideInformed)
                : ::Thunder::ProxyStub::UnknownProxyType<Exchange::IBlocker>::BaseClass(channel, implementation, otherSideInformed)
            {
            }

            uint32_t Block(const uint32_t durationMs) override
            {
                const ::Thunder::ProxyStub::UnknownProxy& base(*this);

                ::Thunder::ProxyStub::UnknownProxyType<Exchange::IBlocker>::IPCMessage newMessage(base.Message(0));
                ::Thunder::RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
                writer.Number<uint32_t>(durationMs);

                uint32_t result = base.Invoke(newMessage, InvokeTimeoutMs);
                if (result == ::Thunder::Core::ERROR_NONE) {
                    ::Thunder::RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                    result = reader.Number<uint32_t>();
                }
                return (result);
            }
        };

        typedef ::Thunder::ProxyStub::UnknownStubType<Exchange::IBlocker, BlockerStubMethods> BlockerStub;

        static class BlockerRegistration {
        public:
            BlockerRegistration()
            {
                ::Thunder::RPC::Administrator::Instance().Announce<Exchange::IBlocker, BlockerProxy, BlockerStub>();
            }
        } _blockerProxyStubRegistration;

        // =====================================================================
        // COM-RPC server: offers a Blocker on request and records the proxies clients offer to it
        // =====================================================================

        class TimeoutServer : public ::Thunder::RPC::Communicator {
        public:
            TimeoutServer() = delete;
            TimeoutServer(const TimeoutServer&) = delete;
            TimeoutServer& operator=(const TimeoutServer&) = delete;

            TimeoutServer(const ::Thunder::Core::NodeId& source)
                : ::Thunder::RPC::Communicator(source, _T(""), _T("@comrpc_timeout_test"))
                , _lock()
                , _offered()
                , _blocker(nullptr)
            {
                Open(::Thunder::Core::infinite);
            }
            ~TimeoutServer() override
            {
                if (_blocker != nullptr) {
                    _blocker->Release();
                }
                Close(::Thunder::Core::infinite);
            }

            // Proxy of the Blocker a client offered to us, a proxy that lives on the server side.
            Exchange::IBlocker* WaitForOffer(const uint32_t waitTimeMs)
            {
                std::unique_lock<std::mutex> guard(_lock);
                _offered.wait_for(guard, std::chrono::milliseconds(waitTimeMs), [this]() { return (_blocker != nullptr); });
                return (_blocker);
            }

        private:
            void* Acquire(VARIABLE_IS_NOT_USED const string& className, const uint32_t interfaceId,
                          VARIABLE_IS_NOT_USED const uint32_t versionId) override
            {
                return ((interfaceId == Exchange::IBlocker::ID) ? ::Thunder::Core::Service<Blocker>::Create<Exchange::IBlocker>() : nullptr);
            }
            void Offer(::Thunder::Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                if (interfaceId == Exchange::IBlocker::ID) {
                    std::lock_guard<std::mutex> guard(_lock);
                    if (_blocker == nullptr) {
                        _blocker = remote->QueryInterface<Exchange::IBlocker>();
                        _offered.notify_all();
                    }
                }
            }

            std::mutex _lock;
            std::condition_variable _offered;
            Exchange::IBlocker* _blocker;
        };

        using ClientEngine = ::Thunder::RPC::InvokeServerType<1, 0, 1, 1, 1>;

        // Connects to the server, the peer process might not be listening yet.
        bool Connect(::Thunder::RPC::CommunicatorClient& client)
        {
            for (uint8_t attempt = 0; attempt < 50; ++attempt) {
                if (client.Open(200) == ::Thunder::Core::ERROR_NONE) {
                    return (true);
                }
                client.Close(200);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return (false);
        }

        // Waits for the connection to be closed by the remote side.
        bool WaitForPeerClose(const ::Thunder::RPC::CommunicatorClient& client, const uint32_t waitTimeMs)
        {
            for (uint32_t waited = 0; (client.IsOpen() == true) && (waited < waitTimeMs); waited += 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            return (client.IsOpen() == false);
        }

        // Peer processes must never outlive the process under test, also not when that one aborts.
        void DieWithParent()
        {
            ::prctl(PR_SET_PDEATHSIG, SIGKILL);
        }

        ::Thunder::Core::NodeId Prepare(const std::string& connector)
        {
            ::unlink(connector.c_str());
            return (::Thunder::Core::NodeId(connector.c_str()));
        }

        int ExitStatus(const pid_t child, const bool kill)
        {
            if (kill == true) {
                ::kill(child, SIGKILL);
            }

            int status = 0;
            ::waitpid(child, &status, 0);

            return ((WIFEXITED(status) != 0) ? WEXITSTATUS(status) : PEER_FAILED);
        }

        // =====================================================================
        // Scenario: a client offers a Blocker to the server, the server (owner of
        // a Communicator::Client channel) invokes it and times out.
        // =====================================================================

        // Client process, it must see the connection to the server being closed.
        int OfferBlockerAndWaitForClose(const ::Thunder::Core::NodeId& node)
        {
            ::Thunder::Core::ProxyType<ClientEngine> engine(::Thunder::Core::ProxyType<ClientEngine>::Create());
            ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient> client(
                ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient>::Create(node, ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer>(engine)));

            int result = NO_CONNECTION;

            if (Connect(*client) == true) {
                Exchange::IBlocker* blocker = ::Thunder::Core::Service<Blocker>::Create<Exchange::IBlocker>();

                result = (client->Offer<Exchange::IBlocker>(blocker, ~0, SetupTimeoutMs) == ::Thunder::Core::ERROR_NONE) ? SUCCESS : NO_INTERFACE;
                blocker->Release();

                if (result == SUCCESS) {
                    result = (WaitForPeerClose(*client, PeerClosedTimeoutMs) == true) ? SUCCESS : PEER_NOT_CLOSED;
                }
            }

            client->Close(200);
            return (result);
        }

        // Server process. With COMRPC_ABORT_ON_TIMEOUT the process dies in Block(), without it
        // the socket to the client must have been shut down and the process exits with SUCCESS.
        [[noreturn]] void ServerSideTimeoutScenario(const std::string& connector)
        {
            const ::Thunder::Core::NodeId node(Prepare(connector));

            const pid_t client = ::fork();
            if (client == 0) {
                DieWithParent();
                ::_exit(OfferBlockerAndWaitForClose(node));
            }

            int result = NO_INTERFACE;
            {
                TimeoutServer server(node);

                Exchange::IBlocker* blocker = server.WaitForOffer(SetupTimeoutMs);

                if (blocker != nullptr) {
                    const uint32_t timedOut = blocker->Block(BlockTimeMs);

                    // Only reached when the process has not been aborted.
                    result = ((timedOut & ~COM_ERROR) == ::Thunder::Core::ERROR_TIMEDOUT) ? SUCCESS : UNEXPECTED_RESULT;
                }
            }

            // The client only sees the connection closing in time if the server shut it down on the timeout,
            // this process is still alive at this point.
            const int clientResult = ExitStatus(client, (result != SUCCESS));

            ::_exit((result != SUCCESS) ? result : clientResult);
        }

        // =====================================================================
        // Scenario: the server hosts a Blocker, a client (owner of a CommunicatorClient
        // channel) acquires it, invokes it and times out.
        // =====================================================================

        [[noreturn]] void ServeUntilKilled(const ::Thunder::Core::NodeId& node)
        {
            TimeoutServer server(node);

            std::this_thread::sleep_for(std::chrono::seconds(30));
            ::_exit(SUCCESS);
        }

        [[noreturn]] void ClientSideTimeoutScenario(const std::string& connector)
        {
            const ::Thunder::Core::NodeId node(Prepare(connector));

            const pid_t server = ::fork();
            if (server == 0) {
                DieWithParent();
                ServeUntilKilled(node);
            }

            int result = NO_CONNECTION;
            {
                ::Thunder::Core::ProxyType<ClientEngine> engine(::Thunder::Core::ProxyType<ClientEngine>::Create());
                ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient> client(
                    ::Thunder::Core::ProxyType<::Thunder::RPC::CommunicatorClient>::Create(node, ::Thunder::Core::ProxyType<::Thunder::Core::IIPCServer>(engine)));

                if (Connect(*client) == true) {
                    Exchange::IBlocker* blocker = client->Acquire<Exchange::IBlocker>(SetupTimeoutMs, _T("Blocker"), ~0);

                    result = NO_INTERFACE;

                    if (blocker != nullptr) {
                        const uint32_t timedOut = blocker->Block(BlockTimeMs);

                        // Only reached when the process has not been aborted.
                        result = ((timedOut & ~COM_ERROR) == ::Thunder::Core::ERROR_TIMEDOUT) ? SUCCESS : UNEXPECTED_RESULT;

                        blocker->Release();
                    }
                }

                client->Close(200);
            }

            ExitStatus(server, true);

            ::_exit(result);
        }

        void UseThreadsafeDeathTests()
        {
#ifdef GTEST_FLAG_SET
            GTEST_FLAG_SET(death_test_style, "threadsafe");
#else
            ::testing::FLAGS_gtest_death_test_style = "threadsafe";
#endif
        }

    } // namespace

    // ==========================================================================
    // Server side proxy (Communicator::Client channel)
    // ==========================================================================

    TEST(COMRPC_TimeoutDeathTest, ServerSideProxy_AbortsOnTimeoutWhenEnabled)
    {
#ifdef COMRPC_ABORT_ON_TIMEOUT
        UseThreadsafeDeathTests();
        EXPECT_EXIT(ServerSideTimeoutScenario("/tmp/comrpc_timeout_server_abort"), ::testing::KilledBySignal(SIGABRT), "");
#else
        GTEST_SKIP() << "Requires a build with COMRPC_ABORT_ON_TIMEOUT";
#endif
    }

    TEST(COMRPC_TimeoutDeathTest, ServerSideProxy_ShutsDownSocketOnTimeoutWhenDisabled)
    {
#ifndef COMRPC_ABORT_ON_TIMEOUT
        UseThreadsafeDeathTests();
        EXPECT_EXIT(ServerSideTimeoutScenario("/tmp/comrpc_timeout_server_shutdown"), ::testing::ExitedWithCode(SUCCESS), "");
#else
        GTEST_SKIP() << "Requires a build without COMRPC_ABORT_ON_TIMEOUT";
#endif
    }

    // ==========================================================================
    // Client side proxy (CommunicatorClient channel), same outcome with and without the option
    // ==========================================================================

    TEST(COMRPC_TimeoutDeathTest, ClientSideProxy_ReturnsTimeoutAndDoesNotAbort)
    {
        UseThreadsafeDeathTests();
        EXPECT_EXIT(ClientSideTimeoutScenario("/tmp/comrpc_timeout_client"), ::testing::ExitedWithCode(SUCCESS), "");
    }

} // namespace COMRPCTimeout
} // namespace Tests
} // namespace Thunder
