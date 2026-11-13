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

// Cross-thread reproduction mirroring the ACTUAL field-reported flow more
// faithfully than JobLifetimeRepro.cpp:
//
//   RevokeAndBlock() is called from the MAIN job's Dispatch() (mirrors
//   ActivateJob, triggered synchronously by IShell::Activate() delivering the
//   Activated notification), targeting a ResultJob (mirrors ActivateResultJob)
//   that is CONCURRENTLY EXECUTING on a DIFFERENT worker thread. This exercises
//   ThreadPool::Revoke()'s genuine cross-thread blocking-wait path
//   (Minion::Completed()), NOT the same-thread shortcut used by
//   JobLifetimeRepro.cpp's single-thread reentrant scenario.
//
//   Just like the real ActivateJob, MainJob holds its OWN reference
//   (_resultjob) to the result job wrapper -- independent from Controller's
//   own _resultJob reference -- for the entire duration of its Dispatch().
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

// Mirrors PluginInitializerService::ActivateResultJob
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
        printf("[ResultJob] Dispatch() starting (thread %p)\n", reinterpret_cast<void*>(pthread_self()));
        fflush(stdout);
        _startedSignal.SetEvent();
        // simulate the real work done inside ActivationResultNotification() (lock
        // protected plugin-state handling) so the job is genuinely still EXECUTING
        // at the moment the MainJob thread calls RevokeAndBlock()
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        printf("[ResultJob] Dispatch() finishing\n");
        fflush(stdout);
    }

private:
    Core::Event& _startedSignal;
};

using ResultJobProxyType = Core::ProxyType<RevokeAndBlockJobType<ResultJob>>;

// Mirrors PluginInitializerService::ActivateJob: a plain IDispatch holding its
// OWN reference to the result job wrapper, exactly like the real code -- this
// sibling reference is what should keep the wrapper alive while MainJob runs.
class MainJob : public Core::IDispatch {
public:
    MainJob(Controller& controller, const ResultJobProxyType& resultjob, Core::Event& startedSignal)
        : _controller(controller)
        , _resultjob(resultjob)
        , _startedSignal(startedSignal)
        , _active(false)
    {
    }
    ~MainJob() override = default;

    MainJob(const MainJob&) = delete;
    MainJob& operator=(const MainJob&) = delete;

    void Submit(const Core::ProxyType<Core::IDispatch>& job)
    {
        if (_active == false) {
            _active = true;
            Core::IWorkerPool::Instance().Submit(job);
        }
    }

private:
    void Dispatch() override;

private:
    Controller& _controller;
    ResultJobProxyType _resultjob;
    Core::Event& _startedSignal;
    std::atomic_bool _active;
};

using MainJobProxyType = Core::ProxyType<MainJob>;

// Mirrors PluginInitializerService::PluginStarter
class Controller {
public:
    Controller()
        : _resultJob()
        , _mainJob()
        , _startedSignal(false, true)
        , _done(false)
    {
    }

    void Start()
    {
        _resultJob = ResultJobProxyType::Create(_startedSignal);
        _mainJob = MainJobProxyType::Create(*this, _resultJob, _startedSignal);

        printf("[controller] submitting ResultJob and MainJob as 2 independent jobs\n");
        fflush(stdout);

        _resultJob->Submit();
        _mainJob->Submit(Core::ProxyType<Core::IDispatch>(_mainJob));
    }

    // Mirrors PluginStarter::Activated() -> SetInactive(), invoked synchronously
    // from WITHIN MainJob::Dispatch() (i.e. from ActivateJob's call to
    // IShell::Activate()) -- on a DIFFERENT thread than the one currently
    // executing ResultJob::Dispatch().
    void OnActivatedMidFlight()
    {
        printf("[controller] OnActivatedMidFlight() cross-thread notification (thread %p)\n",
            reinterpret_cast<void*>(pthread_self()));
        fflush(stdout);
        if (_resultJob.IsValid() == true) {
            printf("[controller] calling RevokeAndBlock() -- expected to BLOCK until ResultJob::Dispatch() on the other thread completes\n");
            fflush(stdout);
            _resultJob->RevokeAndBlock();
            printf("[controller] RevokeAndBlock() returned\n");
            fflush(stdout);
            _resultJob = ResultJobProxyType();
        }
    }

    void SignalDone()
    {
        _done = true;
    }
    bool Done() const
    {
        return _done.load();
    }

private:
    ResultJobProxyType _resultJob;
    MainJobProxyType _mainJob;
    Core::Event _startedSignal;
    std::atomic<bool> _done;
};

void MainJob::Dispatch()
{
    printf("[MainJob] Dispatch() starting (thread %p)\n", reinterpret_cast<void*>(pthread_self()));
    fflush(stdout);
    _active = false;

    // wait until ResultJob has genuinely started executing on the OTHER thread
    // before delivering the reentrant cross-thread notification -- guarantees
    // real concurrency instead of a best-effort race
    _startedSignal.Lock();

    _controller.OnActivatedMidFlight();

    // mirrors ActivateJob::Dispatch()'s tail call -- a no-op here since
    // RevokeAndBlock() already set _blocked == true
    _resultjob->Submit();

    _controller.SignalDone();
}

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

        while (controller.Done() == false) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    printf("main: finished without an observed crash\n");
    fflush(stdout);

    Core::IWorkerPool::Assign(nullptr);
    pool.Stop();
    return 0;
}
