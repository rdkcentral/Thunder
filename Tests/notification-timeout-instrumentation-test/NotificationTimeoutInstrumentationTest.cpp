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

//
// IPlugin::INotification timeout-instrumentation reproducer
// ============================================================
// Demonstrates the special, non-upstream diagnostic instrumentation added to:
//   - Source/com/Communicator.h                    (records the announcing client's PID, keyed
//                                                     by channel LinkId(), at Announce() time)
//   - Source/com/IUnknown.cpp                       (detects an IPC timeout while invoking an
//                                                     IPlugin::INotification proxy, logs the
//                                                     offending iterator index/pid, then calls
//                                                     INTENTIONAL_CRASH)
//   - Source/com/NotificationTimeoutDebug.h          (thread-local correlation state + PID map)
//   - Source/WPEFramework/PluginServer.h             (ServiceMap registration/notification-loop
//                                                     bookkeeping this reproducer mirrors by hand)
//
// This does NOT stand up a full PluginHost/ServiceMap - that would require an entire plugin
// hosting environment (Config, IShell, JSON-RPC, etc). Instead it drives the same generic,
// lower-level COM-RPC primitives ServiceMap's real registration/notification path ultimately
// relies on:
//   - RPC::Communicator / RPC::CommunicatorClient perform a real Announce() handshake between
//     two processes, exercising the PID-capture code in Communicator.h for real.
//   - RPC::CommunicatorClient::Offer<INTERFACE>() hands a locally-implemented
//     IPlugin::INotification sink to the server. This is the same generic "expose an interface
//     pointer to the other side" mechanism a real registration ends up using - there it happens
//     implicitly, as a side effect of marshalling the INotification* parameter of a call to
//     IShell::Register(); here it is invoked directly to keep the reproducer self-contained.
//   - Calling Activated() on the resulting proxy drives a real IPC round trip through
//     ProxyStub::UnknownProxy::Invoke() - the exact function that detects the timeout.
//
// How to build (from your WPEFramework build directory):
//   cmake -DTHUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION=ON -DNOTIFICATION_TIMEOUT_INSTRUMENTATION_TEST=ON ...
//   make NotificationTimeoutInstrumentationTest
//
// What it does:
//   1. Forks a "server" process that listens on a UNIX domain socket and forks a "client"
//      process that connects to it.
//   2. The client Offer()s an IPlugin::INotification implementation whose Activated() deliberately
//      never returns (simulating a wedged/deadlocked observer process).
//   3. The server receives the offered sink (exactly as ServiceMap::Register() would receive a
//      real plugin monitor's sink), logs its registration order/count/pid the same way
//      PluginServer.h's LogNotificationClientRegistered() does, then calls Activated() on it -
//      exactly the way ServiceMap::Activated() does - after setting the same thread-local
//      RPC::NotificationTimeoutDebug state PluginServer.h's ENTER/LEAVE macros set.
//   4. Because the client never replies, UnknownProxy::Invoke() eventually sees
//      Core::ERROR_TIMEDOUT, logs "[NOTIFY-TIMEOUT-DEBUG] ..." with the iterator index/sink
//      pointer, and calls INTENTIONAL_CRASH(), which aborts the server process (SIGABRT).
//   5. This test harness (the top-level process) waits for the server process and checks that it
//      was indeed terminated by SIGABRT - i.e. that the whole feature, end-to-end, worked.
//
// NOTE: RPC::CommunicationTimeOut (Source/com/Administrator.h) is 20 seconds on non-Windows
// platforms, so this reproducer takes about that long to complete - that is genuine production
// behaviour, not a bug in the test.
//

#include "Module.h"

#ifndef THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION

#include <cstdio>

int main()
{
    fprintf(stderr, "This test requires the build to be configured with "
                     "-DTHUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION=ON.\n");
    return (77); // conventionally means "skipped"
}

#else

#include <com/NotificationTimeoutDebug.h>

#include <cstdio>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

namespace WPEFramework {
namespace Test {

    // The far end of the "connection" - implements IPlugin::INotification and deliberately hangs
    // in Activated(), simulating a wedged observer process so the server's Invoke() times out.
    class HangingNotificationSink : public PluginHost::IPlugin::INotification {
    public:
        HangingNotificationSink()
            : _block(false, true)
        {
        }
        ~HangingNotificationSink() override = default;

        void Activated(const string& callsign, PluginHost::IShell*) override
        {
            fprintf(stderr, "[Client] Activated('%s') called - deliberately hanging so the server times out...\n", callsign.c_str());
            _block.Lock(Core::infinite);
        }
        void Deactivated(const string&, PluginHost::IShell*) override
        {
        }
        void Unavailable(const string&, PluginHost::IShell*) override
        {
        }

        BEGIN_INTERFACE_MAP(HangingNotificationSink)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        Core::Event _block;
    };

    // Mimics the slice of PluginHost::Server::ServiceMap this reproducer needs: a list of
    // registered IPlugin::INotification observers, populated the same way ServiceMap::Register()
    // is (log registration order/count/pid), reached here via Communicator::Offer() instead of
    // IShell::Register() to avoid needing a full plugin-hosting environment.
    class NotificationHub : public RPC::Communicator {
    public:
        NotificationHub(const Core::NodeId& node)
            : RPC::Communicator(node, _T(""))
            , _adminLock()
            , _notifiers()
            , _registered(false, true)
        {
        }
        ~NotificationHub() override = default;

        bool WaitForRegistration(const uint32_t waitTimeMs)
        {
            return (_registered.Lock(waitTimeMs) == Core::ERROR_NONE);
        }
        PluginHost::IPlugin::INotification* FirstNotifier()
        {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return ((_notifiers.empty() == false) ? _notifiers.front() : nullptr);
        }

    private:
        void Offer(Core::IUnknown* remote, const uint32_t interfaceId) override
        {
            if (interfaceId == PluginHost::IPlugin::INotification::ID) {

                PluginHost::IPlugin::INotification* sink = remote->QueryInterface<PluginHost::IPlugin::INotification>();

                if (sink != nullptr) {

                    _adminLock.Lock();
                    const uint32_t registrationOrder = static_cast<uint32_t>(_notifiers.size());
                    _notifiers.push_back(sink);
                    const uint32_t totalClients = static_cast<uint32_t>(_notifiers.size());
                    _adminLock.Unlock();

                    // Same lookup PluginServer.h's LogNotificationClientRegistered() performs.
                    uint32_t pid = 0;
                    const ProxyStub::UnknownProxyType<PluginHost::IPlugin::INotification>* proxy =
                        dynamic_cast<const ProxyStub::UnknownProxyType<PluginHost::IPlugin::INotification>*>(sink);
                    if (proxy != nullptr) {
                        const ProxyStub::UnknownProxy* administration = proxy->Administration();
                        if (administration != nullptr) {
                            const Core::ProxyType<Core::IPCChannel>& channel = administration->Channel();
                            if (channel.IsValid() == true) {
                                pid = RPC::NotificationTimeoutDebug::LookupChannelPid(channel->LinkId());
                            }
                        }
                    }

                    fprintf(stderr, "[Server] IPlugin::INotification client registered: registrationOrder=%u, totalClients=%u, pid=%u, sinkPtr=%p\n",
                        registrationOrder, totalClients, pid, static_cast<const void*>(sink));

                    _registered.SetEvent();
                }
            }
        }

        Core::CriticalSection _adminLock;
        std::vector<PluginHost::IPlugin::INotification*> _notifiers;
        Core::Event _registered;
    };

    // Runs as the "client" (the observer process) in a forked child: connects to the server and
    // offers its INotification sink, then blocks forever (Activated() never returns on its own).
    void RunClient(const Core::NodeId& serverNode)
    {
        Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(serverNode));

        if (client->Open(RPC::CommunicationTimeOut) != Core::ERROR_NONE) {
            fprintf(stderr, "[Client] FAIL: could not connect to server\n");
            _exit(1);
        }

        Core::Sink<HangingNotificationSink> sink;

        if (client->Offer<PluginHost::IPlugin::INotification>(&sink) != Core::ERROR_NONE) {
            fprintf(stderr, "[Client] FAIL: Offer() failed\n");
            _exit(1);
        }

        fprintf(stderr, "[Client] Offered INotification sink to server, waiting to be invoked (and to hang)...\n");

        // Keep this process (and the connection) alive until the parent test harness kills it.
        for (;;) {
            ::sleep(60);
        }
    }

    // Runs as the "server" (the plugin host, in spirit): accepts the client's registration and
    // then invokes Activated() on it, exactly like ServiceMap::Activated() would.
    void RunServer(const Core::NodeId& serverNode)
    {
        NotificationHub hub(serverNode);

        if (hub.Open(5000) != Core::ERROR_NONE) {
            fprintf(stderr, "[Server] FAIL: could not open listening socket\n");
            _exit(1);
        }

        fprintf(stderr, "[Server] Listening on %s, waiting for the client to register...\n", serverNode.HostName().c_str());

        if (hub.WaitForRegistration(10000) == false) {
            fprintf(stderr, "[Server] FAIL: client never registered\n");
            _exit(1);
        }

        PluginHost::IPlugin::INotification* client = hub.FirstNotifier();
        ASSERT(client != nullptr);

        fprintf(stderr, "[Server] Calling Activated() - this will block for up to %u ms before the timeout instrumentation fires...\n",
            static_cast<uint32_t>(RPC::CommunicationTimeOut));

        // Mirrors PluginServer.h's NOTIFICATION_TIMEOUT_DEBUG_ENTER/LEAVE bookkeeping around this
        // exact call, at registration order/iterator position 0 (the only registered client here).
        RPC::NotificationTimeoutDebug::CurrentIteratorIndex = 0;
        RPC::NotificationTimeoutDebug::CurrentSinkPointer = static_cast<const void*>(client);

        client->Activated(_T("NotificationTimeoutInstrumentationTest"), nullptr);

        // Unreachable: the call above should have caused INTENTIONAL_CRASH() to abort this process.
        fprintf(stderr, "[Server] FAIL: Activated() returned instead of timing out - instrumentation did not trigger\n");
        _exit(1);
    }

} // namespace Test
} // namespace WPEFramework

int main()
{
    using namespace WPEFramework;

    const Core::NodeId serverNode(_T("/tmp/notification-timeout-instrumentation-test.sock"));
    ::unlink(serverNode.HostName().c_str());

    pid_t serverPid = fork();

    if (serverPid == 0) {
        Test::RunServer(serverNode);
        _exit(0); // unreachable
    }

    // Give the server a moment to create and start listening on the socket before the client
    // tries to connect to it.
    for (uint8_t attempt = 0; (attempt < 50) && (::access(serverNode.HostName().c_str(), F_OK) != 0); attempt++) {
        ::usleep(100000); // 100 ms
    }

    pid_t clientPid = fork();

    if (clientPid == 0) {
        Test::RunClient(serverNode);
        _exit(0); // unreachable
    }

    // --- test harness (original process) ---
    int status = 0;
    waitpid(serverPid, &status, 0);

    // The client never finishes on its own - clean it up now that the server side is done.
    kill(clientPid, SIGKILL);
    int clientStatus = 0;
    waitpid(clientPid, &clientStatus, 0);

    if ((WIFSIGNALED(status) != 0) && (WTERMSIG(status) == SIGABRT)) {
        fprintf(stdout, "[Main] PASS: server process was terminated by SIGABRT via INTENTIONAL_CRASH, as expected\n");
        return (0);
    }

    fprintf(stdout, "[Main] FAIL: server process ended unexpectedly (status=%d)\n", status);
    return (1);
}

#endif // THUNDER_NOTIFICATION_TIMEOUT_INSTRUMENTATION
