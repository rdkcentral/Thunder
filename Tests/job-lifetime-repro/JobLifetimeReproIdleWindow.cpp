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

// Targets a DIFFERENT window than JobLifetimeReproCrossThread.cpp:
//
//   Core::ThreadPool::Minion::Process() transitions a job's state EXECUTING ->
//   IDLE (via Closure()/Resubmit()) BEFORE it actually releases its own
//   _currentRequest reference. Core::ThreadPool::JobType::RevokeRequired() (and
//   therefore RevokeAndBlock()) only consults that state -- if it is already
//   IDLE, Revoke() returns an invalid job and RevokeAndBlock() returns
//   IMMEDIATELY, without waiting, even though the Minion thread may still hold
//   a live reference and not yet have called Release().
//
//   This test exercises exactly that gap (artificially widened in
//   core/ThreadPool.h for this experiment -- see the sleep_for() inserted
//   between Closure() and _currentRequest.Release() in Minion::Process()) by
//   having BOTH outstanding references to the ResultJob wrapper dropped
//   (mirroring PluginStarter::SetInactive() fully tearing down _activateResultJob
//   and _activateJob) right after a RevokeAndBlock() call that is timed to land
//   inside the widened window.
#include "Module.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <pthread.h>
#include <thread>

using namespace Thunder;

namespace {

class SimpleDispatcher : public Core::ThreadPool::IDispatcher {
public:
    void Initialize() override { }
    void Deinitialize() override { }
    void Dispatch(Core::IDispatch* job) override
    {
        job->Dispatch();
    }
};

// Mirrors PluginInitializerService::RevokeAndBlockJobType<IMPLEMENTATION>
template <typename IMPLEMENTATION>
class RevokeAndBlockJobType {
public:
    RevokeAndBlockJobType(const RevokeAndBlockJobType<IMPLEMENTATION>&) = delete;
    RevokeAndBlockJobType<IMPLEMENTATION>& operator=(const RevokeAndBlockJobType<IMPLEMENTATION>&) = delete;

    template <typename... Args>
    explicit RevokeAndBlockJobType(Args&&... args)
        : _workerjob(std::forward<Args>(args)...)
        , _blocked(false)
        , _adminLock()
    {
    }
    ~RevokeAndBlockJobType()
    {
        printf("[wrapper %p] ~RevokeAndBlockJobType running (thread %p)\n",
            static_cast<void*>(this), reinterpret_cast<void*>(pthread_self()));
        fflush(stdout);
    }

    bool Submit()
    {
        Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
        bool submitted = false;
        if (_blocked == false) {
            submitted = _workerjob.Submit();
        }
        return submitted;
    }
    void RevokeAndBlock()
    {
        Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
        _blocked = true;
        _workerjob.Revoke();
    }

private:
    Core::IWorkerPool::JobType<IMPLEMENTATION> _workerjob;
    bool _blocked;
    Core::CriticalSection _adminLock;
};

// Mirrors PluginInitializerService::ActivateResultJob -- deliberately does
// (almost) NO work, so it reaches Closure()/IDLE very quickly and spends
// nearly all of its lifetime inside the artificially-widened IDLE window.
class ResultJob {
public:
    explicit ResultJob(Core::Event& startedSignal)
        : _startedSignal(startedSignal)
    {
    }
    ~ResultJob() = default;

    ResultJob(const ResultJob&) = delete;
    ResultJob& operator=(const ResultJob&) = delete;

private:
    friend Core::ThreadPool::JobType<ResultJob>;
    void Dispatch()
    {
        printf("[ResultJob] Dispatch() entry (thread %p) -- signalling started\n",
            reinterpret_cast<void*>(pthread_self()));
        fflush(stdout);
        _startedSignal.SetEvent();
        printf("[ResultJob] Dispatch() returning now (state -> IDLE imminent, then the\n"
               "            artificial 200ms sleep in Minion::Process() before Release())\n");
        fflush(stdout);
    }

private:
    Core::Event& _startedSignal;
};

using ResultJobProxyType = Core::ProxyType<RevokeAndBlockJobType<ResultJob>>;

// Mirrors PluginStarter: holds the "PluginStarter::_activateResultJob" reference,
// and -- just like SetInactive() -- drops BOTH outstanding references (its own,
// plus the sibling normally held by ActivateJob::_resultjob) once RevokeAndBlock()
// returns.
class Controller {
public:
    Controller()
        : _resultJob()
        , _siblingResultJob()
        , _startedSignal(false, true)
    {
    }

    void Start()
    {
        _resultJob = ResultJobProxyType::Create(_startedSignal);
        // mirrors ActivateJob's OWN sibling copy of the same wrapper
        _siblingResultJob = _resultJob;

        printf("[controller] submitting ResultJob (refcount now held by both\n"
               "             Controller and its sibling, mirroring PluginStarter +\n"
               "             ActivateJob)\n");
        fflush(stdout);
        _resultJob->Submit();
    }

    // Mirrors PluginStarter::SetInactive(): called from a DIFFERENT thread than
    // the one running ResultJob::Dispatch(), timed to land inside the widened
    // IDLE window.
    void SetInactiveStyleTeardown()
    {
        printf("[controller] calling RevokeAndBlock() (thread %p) -- if the bug is\n"
               "             real, this returns IMMEDIATELY because state is already\n"
               "             IDLE, even though the Minion hasn't released yet\n",
            reinterpret_cast<void*>(pthread_self()));
        fflush(stdout);

        auto start = std::chrono::steady_clock::now();
        _resultJob->RevokeAndBlock();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
                             .count();

        printf("[controller] RevokeAndBlock() returned after %lld ms\n", static_cast<long long>(elapsedMs));
        if (elapsedMs < 100) {
            printf("[controller] ^^ returned almost immediately -- did NOT wait for the\n"
                   "             Minion's still-pending Release(); dropping both references now\n");
        } else {
            printf("[controller] ^^ actually blocked roughly the full artificial window -- safe in this run\n");
        }
        fflush(stdout);

        // Drop BOTH references right away, exactly like SetInactive() drops
        // _activateResultJob and then (via ~ActivateJob()) _resultjob.
        _resultJob = ResultJobProxyType();
        _siblingResultJob = ResultJobProxyType();

        printf("[controller] both references dropped -- if that was the last one and\n"
               "             the Minion still holds a now-dangling reference, it will\n"
               "             crash shortly when it wakes up from the artificial sleep\n");
        fflush(stdout);
    }

    void WaitStarted()
    {
        _startedSignal.Lock();
    }

private:
    ResultJobProxyType _resultJob;
    ResultJobProxyType _siblingResultJob;
    Core::Event _startedSignal;
};

} // namespace

int main()
{
    SimpleDispatcher dispatcher;
    Core::WorkerPool pool(2, 0, 8, &dispatcher);
    Core::IWorkerPool::Assign(&pool);
    pool.Run();

    {
        Controller controller;
        controller.Start();

        // Wait until ResultJob has genuinely started (and, in practice, almost
        // certainly already returned and hit the widened IDLE window by the
        // time we wake up from the short sleep below -- Dispatch() itself does
        // essentially no work).
        controller.WaitStarted();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        controller.SetInactiveStyleTeardown();

        // Give the Minion time to wake up from its artificial sleep and attempt
        // the (possibly now use-after-free) Release() call.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    printf("main: finished without an observed crash\n");
    fflush(stdout);

    Core::IWorkerPool::Assign(nullptr);
    pool.Stop();
    return 0;
}
