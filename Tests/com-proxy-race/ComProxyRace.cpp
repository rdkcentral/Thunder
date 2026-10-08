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

// Repro for a genuine (non-deterministic) UnknownProxy teardown/resurrection
// race, replacing an earlier, invalidated version of this test.
//
// EARLIER (RETRACTED) THEORY: two concurrent Complete() calls on a proxy that
// had only been AddRef()'d once both reach the "terminal" branch of
// Release(). That is NOT a race: UnknownProxy::Release()'s decrement and
// terminal-branch decision are fully serialized under its own _adminLock, so
// a correctly-paired AddRef/Release sequence can never have two callers both
// observe refCount<=1.
//
// FIRST CORRECTED ATTEMPT (ALSO INVALIDATED): a resurrector thread calling
// UnknownProxyType::QueryInterface() directly on the raw proxy pointer,
// racing a releaser thread's Release(). Measured empirically to crash almost
// immediately in BOTH the delayed and non-delayed variant, because a direct
// pointer call bypasses the real safety net entirely: in production, nothing
// ever reaches a proxy's QueryInterface() without first finding it via
// RPC::Administrator::ProxyFind(), which only succeeds while the proxy is
// still listed in _channelProxyMap. Calling QueryInterface() directly let the
// test "resurrect" a proxy at literally any point, including long after it
// was already deleted -- not a faithful model of the real exposure.
//
// CURRENT (FAITHFUL) DESIGN: the proxy is registered the real way, via
// RPC::Administrator::ProxyInstance(channel, impl, outbound, ...), using a
// minimal non-functional Core::IPCChannel so it lands in _channelProxyMap.
// The resurrector thread looks it up the real way too, via
// RPC::Administrator::ProxyFind(channel, impl) -- which holds the
// Administrator's single *global* _adminLock for its entire
// lookup-then-QueryInterface() call. RPC::Administrator::UnregisterUnknownProxy()
// (called from UnknownProxy::Release()'s terminal branch, AFTER that proxy's
// own *per-proxy* _adminLock has already been released) takes the SAME
// global lock to erase the entry. So the real, narrow, genuinely racy window
// is just the gap between:
//   - UnknownProxy::Release() unlocking its own per-proxy lock, and
//   - it then acquiring the Administrator's global lock inside
//     UnregisterUnknownProxy() to remove the entry.
// If ProxyFind() (on another thread) acquires the global lock first during
// that gap, it still finds the (soon to be deleted) entry, unconditionally
// AddRef()s it via QueryInterface(), and hands back a pointer that Release()
// is unconditionally going to free regardless -- a genuine, scheduler-timed
// use-after-free, matching field crash fingerprint 7826415 (SIGSEGV in
// Core::CriticalSection::Lock from COM-RPC proxy teardown/interface-query
// code). If UnregisterUnknownProxy() gets the global lock first, ProxyFind()
// correctly finds nothing and is fully safe.
//
// Because the outcome depends on which thread wins that lock race -- not on
// any guaranteed logic defect -- a single-process loop is a poor measurement
// tool (the first crash kills the whole run). The BUGGY variant's gated delay
// (inserted into IUnknown.h, between Release()'s per-proxy unlock and its
// UnregisterUnknownProxy() call) widens the window so ProxyFind() can almost
// always win it; the BASELINE variant (same code, no delay) leaves only the
// natural, narrow gap, so it should win far less often. Logging every
// iteration (rather than embedding process-forking machinery in this binary)
// is enough: run each variant for many iterations/trials from the shell and
// compare how reliably -- and how early -- each one crashes.

#include "Module.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

using namespace Thunder;

namespace {

    // Minimal, non-functional IPCChannel. Administrator::ProxyInstance() only
    // registers a proxy in _channelProxyMap when channel.IsValid() == true, so
    // we need a real (but otherwise inert) channel object; our proxy is built
    // "inbound" (outbound=false) and nothing here ever calls Invoke(), so none
    // of the actual transport methods below are ever exercised.
    class FakeChannel : public Core::IPCChannel {
    public:
        FakeChannel() : Core::IPCChannel() {}
        ~FakeChannel() override = default;

        uint32_t Id() const override { return 0xC0FFEE; }
        string Origin() const override { return _T("ComProxyRaceTest"); }
        uint32_t ReportResponse(Core::ProxyType<Core::IIPC>&) override { return Core::ERROR_NONE; }
        bool IsOpen() const override { return false; }
        bool IsClosed() const override { return true; }

    private:
        uint32_t Execute(const Core::ProxyType<Core::IIPC>&, Core::IDispatchType<Core::IIPC>*) override { return Core::ERROR_UNAVAILABLE; }
        uint32_t Execute(const Core::ProxyType<Core::IIPC>&, const uint32_t) override { return Core::ERROR_UNAVAILABLE; }
    };

    using TestProxy = ProxyStub::UnknownProxyType<Core::IUnknown>;

    ProxyStub::MethodHandler TestStubMethods[] = { nullptr };
    typedef ProxyStub::UnknownStubType<Core::IUnknown, TestStubMethods> TestStub;

    class Registration {
    public:
        Registration()
        {
            RPC::Administrator::Instance().Announce<Core::IUnknown, TestProxy, TestStub>();
        }
    } _registration;

    constexpr Core::instance_id TestImplementation = static_cast<Core::instance_id>(0x1234);

    void RunOneIteration(uint32_t iteration)
    {
        Core::ProxyType<Core::IPCChannel> channel(Core::ProxyType<FakeChannel>::Create());

        Core::IUnknown* owned = nullptr;
        RPC::Administrator::Instance().ProxyInstance<Core::IUnknown>(channel, TestImplementation, false, owned);

        // ProxyInstance() already left us holding one legitimate external
        // reference in `owned` (refCount 2: one for the Administrator's own
        // bookkeeping slot, one for this call's internal QueryInterface()).

        std::atomic<bool> go{ false };

        // Thread A: the real, matching Release() for the reference above.
        // Expected to tear the proxy down (refCount 2 -> 1 -> terminal branch).
        std::thread releaser([&]() {
            while (go.load(std::memory_order_acquire) == false) {
                // spin, waiting for the simultaneous start signal
            }
            fprintf(stderr, "Releaser releasing\n");
            owned->Release();
        });

        // Thread B: a concurrent lookup via the real Administrator::ProxyFind(),
        // exactly as RPC::Administrator::ProxyInstance()'s "already have a proxy
        // for this channel+impl" path or any other channel-driven lookup would do.
        std::thread resurrector([&]() {
            while (go.load(std::memory_order_acquire) == false) {
                // spin, waiting for the simultaneous start signal
            }
            void* found = nullptr;
            RPC::Administrator::Instance().ProxyFind(channel, TestImplementation, Core::IUnknown::ID, found);
            if (found != nullptr) {
                // Mirrors a caller that has no idea Thread A is mid-teardown,
                // using and then releasing what it believes is a valid reference.
		fprintf(stderr, "Resurrector releasing\n");
                reinterpret_cast<Core::IUnknown*>(found)->Release();
            }
        });

        go.store(true, std::memory_order_release);

        releaser.join();
        resurrector.join();

        fprintf(stderr, "[ComProxyRace] iteration %u completed without crashing\n", iteration);
        fflush(stderr);
    }

} // namespace

int main(int argc, char** argv)
{
    uint32_t iterations = 500;

    if (argc > 1) {
        iterations = static_cast<uint32_t>(atoi(argv[1]));
    }

#ifdef COM_PROXY_RACE_REPRO_DELAY
    fprintf(stderr, "[ComProxyRace] BUGGY variant: artificial delay widens the resurrection window in Release()\n");
#else
    fprintf(stderr, "[ComProxyRace] BASELINE variant: no artificial delay (natural scheduling timing only)\n");
#endif

    for (uint32_t iteration = 0; iteration < iterations; iteration++) {
        RunOneIteration(iteration);
    }

    Core::Singleton::Dispose();

    fprintf(stderr, "[ComProxyRace] Completed %u iterations without crashing.\n", iterations);

    return 0;
}
