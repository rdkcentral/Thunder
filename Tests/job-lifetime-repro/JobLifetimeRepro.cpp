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

// Standalone, PluginInitializerService-independent reproduction of the
// double-free / use-after-free caused by a job reentrantly releasing the
// LAST reference to its own enclosing RevokeAndBlockJobType-style wrapper
// from inside its own Dispatch().
//
// Two targets are built from this single source (see CMakeLists.txt):
//   JobLifetimeReproBuggy  (-DBUGGY) : reproduces PluginStarter::SetInactive()'s
//               original pattern (immediate release of the last reference
//               while still executing inside the job's own Dispatch())
//   JobLifetimeReproFixed  (default) : the proposed fix (deferred release via
//               a follow-up job)
#include "Module.h"

#include <atomic>
#include <chrono>
#include <cstdio>
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

class Controller;

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
        printf("[wrapper %p] ~RevokeAndBlockJobType running\n", static_cast<void*>(this));
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

// Mirrors PluginInitializerService::ActivateResultJob
class ResultJob {
public:
    explicit ResultJob(Controller& controller)
        : _controller(controller)
    {
    }
    ~ResultJob() = default;

    ResultJob(const ResultJob&) = delete;
    ResultJob& operator=(const ResultJob&) = delete;

private:
    friend Core::ThreadPool::JobType<ResultJob>;
    void Dispatch();

private:
    Controller& _controller;
};

using ResultJobProxyType = Core::ProxyType<RevokeAndBlockJobType<ResultJob>>;

#if !defined(BUGGY)
// The proposed fix: release the last reference from a fresh dispatch cycle,
// never from inside the job's own reentrant Dispatch().
class ReleaseJob : public Core::IDispatch {
public:
    explicit ReleaseJob(ResultJobProxyType&& job)
        : _job(std::move(job))
    {
    }
    ~ReleaseJob() override = default;

    ReleaseJob(const ReleaseJob&) = delete;
    ReleaseJob& operator=(const ReleaseJob&) = delete;

private:
    void Dispatch() override
    {
        printf("[ReleaseJob] releasing wrapper on a fresh (non-reentrant) dispatch cycle\n");
        fflush(stdout);
        _job = ResultJobProxyType();
    }

private:
    ResultJobProxyType _job;
};
#endif

// Mirrors PluginInitializerService::PluginStarter
class Controller {
public:
    Controller()
        : _resultJob()
        , _done(false)
    {
    }

    void Start()
    {
        _resultJob = ResultJobProxyType::Create(*this);
        printf("[controller] submitting job (wrapper=%p, only reference held by controller)\n",
            static_cast<void*>(&(*_resultJob)));
        fflush(stdout);
        _resultJob->Submit();
    }

    // Called reentrantly, on the SAME worker thread, from WITHIN ResultJob::Dispatch()
    // -- mirrors PluginStarter::Activated()/Failed() calling SetInactive().
    void OnResult()
    {
        printf("[controller] OnResult() running reentrantly inside Dispatch()\n");
        fflush(stdout);
        if (_resultJob.IsValid() == true) {
            _resultJob->RevokeAndBlock();
#if defined(BUGGY)
            printf("[controller] BUGGY: releasing last reference to wrapper NOW (reentrant)\n");
            fflush(stdout);
            _resultJob = ResultJobProxyType();
            printf("[controller] release call returned (if you see this, corruption went undetected so far)\n");
            fflush(stdout);
#else
            printf("[controller] FIXED: deferring release to a follow-up job\n");
            fflush(stdout);
            Core::ProxyType<Core::IDispatch> release(Core::ProxyType<ReleaseJob>::Create(std::move(_resultJob)));
            Core::IWorkerPool::Instance().Submit(release);
#endif
        }
        _done = true;
    }

    bool Done() const
    {
        return _done.load();
    }

private:
    ResultJobProxyType _resultJob;
    std::atomic<bool> _done;
};

void ResultJob::Dispatch()
{
    _controller.OnResult();
}

} // namespace

int main()
{
    SimpleDispatcher dispatcher;
    Core::WorkerPool pool(2, 0, 8, &dispatcher);
    Core::IWorkerPool::Assign(&pool);
    pool.Run();

    {
	    printf("main: Local scope start\n");
        Controller controller;
        controller.Start();

        while (controller.Done() == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // give the Minion time to unwind back out of Dispatch()/Process()
        // after our reentrant release, exactly as it would in the real crash
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
	        printf("main: Before Local scope ends\n");
            fflush(stdout);
    }

    printf("main: finished without an observed crash\n");
    fflush(stdout);

    Core::IWorkerPool::Assign(nullptr);
    pool.Stop();
    return 0;
}
