/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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
#include <limits>
#include <random>
#include "Module.h"

using namespace Thunder;

class WorkerPoolTestAdministrator
{
    public:
        struct TestJobMetaData {
            const uint32_t   JobId;
            uint32_t         RepeatCnt;
            bool             bIsRevoked;

            bool operator==(const TestJobMetaData& other) const
            {
                return (JobId == other.JobId);
            }
        };

        class TesterTask : public Core::IDispatch {
        public:
            TesterTask(const TesterTask&) = delete;
            TesterTask& operator=(const TesterTask&) = delete;

            explicit TesterTask(WorkerPoolTestAdministrator& parent)
                : _parent(parent)
                , _jobMetaData{_parent.getNextJobId(), _parent.getRandomValue(0, 5), false}
            {
            }
            ~TesterTask() override = default;

            void Dispatch() override {
                _parent.UpdateJobDetails(_jobMetaData.JobId);
            }

            const uint32_t JobId() {
                return _jobMetaData.JobId;
            }

            bool Revoked() {
                return _jobMetaData.bIsRevoked;
            }

            void Revoked(bool bRevoked) {
                _jobMetaData.bIsRevoked = bRevoked;
            }

            uint32_t RepeatCount() {
                return _jobMetaData.RepeatCnt;
            }

            void RepeatCount(uint32_t repeatCnt) {
                _jobMetaData.RepeatCnt = repeatCnt;
            }
        private:
            WorkerPoolTestAdministrator& _parent;
            WorkerPoolTestAdministrator::TestJobMetaData _jobMetaData;

        };

        class MonitorTask : public Core::IDispatch
        {
        public:
            MonitorTask(const MonitorTask&) = delete;
            MonitorTask& operator=(const MonitorTask&) = delete;

            explicit MonitorTask(WorkerPoolTestAdministrator& parent)
                : _parent(parent) {

                }

            ~MonitorTask() override = default;

            void Dispatch() override {
                _parent.AnalyseTestTask();
            }

        private:
            WorkerPoolTestAdministrator& _parent;

        };

    private:
        class WorkerPoolImplementation : public Core::WorkerPool {

        private:
            class Dispatcher : public Core::ThreadPool::IDispatcher {
            public:
                Dispatcher(const Dispatcher&) = delete;
                Dispatcher& operator=(const Dispatcher&) = delete;

                Dispatcher() = default;
                ~Dispatcher() override = default;

            private:
                void Initialize() override { }
                void Deinitialize() override { }
                void Dispatch(Core::IDispatch* job) override {
                    ASSERT(job != nullptr);
                    job->Dispatch();
                }

            };

        public:
            WorkerPoolImplementation() = delete;
            WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
            WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

            WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
                : WorkerPool(threads, stackSize, queueSize, &_dispatcher)
                , _dispatcher(){ }

            ~WorkerPoolImplementation()
            {
                Core::WorkerPool::Stop();
            }

            void Run()
            {
                Core::WorkerPool::Run();
                Core::WorkerPool::Join();

            }

            void Stop()
            {
                Core::WorkerPool::Stop();
            }
        private:
            Dispatcher _dispatcher;
        };

    public:
        WorkerPoolTestAdministrator(const WorkerPoolTestAdministrator&) = delete;
        WorkerPoolTestAdministrator& operator=(const WorkerPoolTestAdministrator&) = delete;

        WorkerPoolTestAdministrator()
            : _workerPool()
            , _jobIdGenerator(0)
            , _lock()
            , _bAllJobSubmitted(false)
            , _adminEvent(false, true)
        {
        }

        ~WorkerPoolTestAdministrator()
        {
            Core::IWorkerPool::Assign(nullptr);
            if (_workerPool.IsValid() == true) {
                _workerPool.Release();
            }
        }

        void ResetAndActivateWorkerPool() {
            if (_workerPool.IsValid() == true) {
                Core::IWorkerPool::Assign(nullptr);
                _workerPool->Stop();
                _workerPool.Release();
            }
            _workerPool = Core::ProxyType<WorkerPoolTestAdministrator::WorkerPoolImplementation>::Create(getRandomValue(3,6), Core::Thread::DefaultStackSize(), getRandomValue(8,16));
            Core::WorkerPool::Assign(&(*_workerPool));

            // clear _activeJobList
            _activeJobList.clear();

            std::thread(
                [this] ()
            {
                _workerPool->Run();
            }
            ).detach();
        }

        void Initialize() {
            ResetAndActivateWorkerPool();
        }

        void Start() {
            if (_workerPool.IsValid() == true) {
                _workerPool->Run();
            }
        }

        void Stop() {
            if (_workerPool.IsValid() == true) {
                _workerPool->Stop();
            }
        }

        void Notify() {
            _adminEvent.SetEvent();
        }

        uint32_t WaitForTestCompletion(uint32_t waitTime) {
            return _adminEvent.Lock(waitTime);
        }

        void ResetEvent() {
            _adminEvent.ResetEvent();
        }

        uint32_t getRandomValue(const uint32_t min, const uint32_t max) {
            static bool once = false;
            if (!once) {
                srand(time(0));
                once = true;
            }

            return 1+((rand() % max) + min-1);
        }

        const uint32_t getNextJobId() {
            _lock.Lock();
            _jobIdGenerator = (_jobIdGenerator+1)%std::numeric_limits<unsigned int>::max();
            _lock.Unlock();
            return _jobIdGenerator;
        }

        void SubmitTestJob() {

            Core::ProxyType<Core::IDispatch> TestJob = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TesterTask>::Create(*this));
            uint32_t JobId = static_cast<TesterTask&>(*TestJob).JobId();

            _lock.Lock();
            _activeJobList[JobId] = TestJob;
            TestJob.AddRef();
            _lock.Unlock();

            _workerPool->Submit(TestJob);
        }

        void UpdateJobDetails(const uint32_t jobId) {
            _lock.Lock();
            if (_activeJobList.count(jobId) > 0 ) {
                uint32_t repeatCnt =  static_cast<TesterTask&>(*_activeJobList[jobId]).RepeatCount();
                bool isRevoked = static_cast<TesterTask&>(*_activeJobList[jobId]).Revoked();
                if (repeatCnt > 0 && !isRevoked) {
                   _lock.Unlock();
                    _workerPool->Schedule(Core::Time::Now().Add(100), _activeJobList[jobId]);
                    _lock.Lock();
                    if (_activeJobList.count(jobId) > 0) {
                        static_cast<TesterTask&>(*_activeJobList[jobId]).RepeatCount(repeatCnt-1);
                        printf("%s[ %d ] JobId = %d has been Scheduled for next run (%d)\n", __FUNCTION__, __LINE__, jobId, repeatCnt);
                    }
                }
                else{
                    if (!isRevoked)
                        _activeJobList.erase(jobId);
                }
            }
            _lock.Unlock();
        }

        void RevokeRandomJobs() {
            // Randomly revoke a few jobs from _activeJobList.
            int nJobsToRevoke = getRandomValue(0, 8);

            _lock.Lock();

            nJobsToRevoke = std::min(nJobsToRevoke, static_cast<int>(_activeJobList.size()));

            for (int i = 0; i < nJobsToRevoke; i++) {
                if (_activeJobList.size() < 5)
                    break;

                auto random_it = std::next(std::begin(_activeJobList), getRandomValue(1, _activeJobList.size()-1));
                if (random_it != _activeJobList.end() && (_activeJobList.count(random_it->first) > 0) ) {
                    printf("%s[ %d ] Revoking JobId = %d \n", __FUNCTION__, __LINE__, random_it->first);
                    static_cast<TesterTask&>(*_activeJobList[random_it->first]).Revoked(true);
                    _lock.Unlock();
                    _workerPool->Revoke(random_it->second);
                    _lock.Lock();
                    if (_activeJobList.count(random_it->first) > 0)
                        _activeJobList.erase(random_it);
                }
            }
            _lock.Unlock();
        }

        void RescheduleRandomJobs() {
            // Randomly revoke a few jobs from _activeJobList.
            int nJobsToRevoke = getRandomValue(0, 10);

            _lock.Lock();

            nJobsToRevoke = std::min(nJobsToRevoke, static_cast<int>(_activeJobList.size()));

            for (int i = 0; i < nJobsToRevoke; i++) {
                if (_activeJobList.size() < 5)
                    break;

                auto random_it = std::next(std::begin(_activeJobList), getRandomValue(1, _activeJobList.size()-1));
                bool isJobRevoked = static_cast<TesterTask&>(*_activeJobList[random_it->first]).Revoked();

                if (random_it != _activeJobList.end() && (_activeJobList.count(random_it->first) > 0) && !isJobRevoked) {
                    printf("%s[ %d ] JobId = %d \n", __FUNCTION__, __LINE__, random_it->first);
                    int nRepeatCnt = static_cast<TesterTask&>(*_activeJobList[random_it->first]).RepeatCount();
                    static_cast<TesterTask&>(*_activeJobList[random_it->first]).RepeatCount(nRepeatCnt+1);

                    _lock.Unlock();
                    _workerPool->Reschedule(Core::Time::Now().Add(/*getRandomValue(0,100)*/100), random_it->second);
                    _lock.Lock();
                }
            }

            _lock.Unlock();
        }

        void GenerateAndTestAutomatedJobs() {

            int nInitialJobCount = getRandomValue(1000, 3000);

            printf("%s[ %d ] Staring %d  Test Jobs \n", __FUNCTION__, __LINE__, nInitialJobCount);

            // Submit the first half of test job from separate thread.
            for (int i =0; i < nInitialJobCount; i++) {
                std::thread(
                    [=] ()
                {
                    SubmitTestJob();
                }
                ).detach();

                // Submit the next half of test job from testAdministrators context.
                SubmitTestJob();
            }
            _bAllJobSubmitted = true;

        }

        void GenerateTestReport() {
            if (_activeJobList.empty())
                printf("\n\n*******\n  Good news !!!  All Jobs Completed Successfully \n\n*******\n");

            else {
                printf(" The Following Jobs FAILED to complete ->  ");

                _lock.Lock();
                for (auto& p : _activeJobList) {
                    printf(" %d ", p.first);
                }
                _lock.Unlock();

                printf("\n");
            }
        }

        void AnalyseTestTask() {
            if (_activeJobList.empty() && _bAllJobSubmitted)
                Notify();
            else {
                printf(" \nThe Following Jobs are still pending ->  ");

                _lock.Lock();
                for (auto& p : _activeJobList){
                    printf(" %d ", p.first);
                }
                _lock.Unlock();

                printf("\n\n");

                if (_activeJobList.size() > 50) {
                    RescheduleRandomJobs();
                    RevokeRandomJobs();
                }
                _workerPool->Schedule(Core::Time::Now(), _monitorJob);
            }
        }

        void StartMonitorJob() {
            _monitorJob = Core::ProxyType<Core::IDispatch>(Core::ProxyType<MonitorTask>::Create(*this));
            _workerPool->Schedule(Core::Time::Now().Add(100), _monitorJob);

        }
    private:
        Core::ProxyType<WorkerPoolImplementation> _workerPool;
        uint32_t _jobIdGenerator;
        Core::CriticalSection _lock;
        bool _bAllJobSubmitted;
        Core::Event  _adminEvent;
        std::unordered_map<uint32_t, Core::ProxyType<Core::IDispatch>> _activeJobList;
        Core::ProxyType<Core::IDispatch> _monitorJob;
};

#ifdef __WINDOWS__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
    WorkerPoolTestAdministrator TestAdministrator;

    int nTestCount = TestAdministrator.getRandomValue(1, 5);
    for (int i =0; i < nTestCount; i++) {
        TestAdministrator.ResetAndActivateWorkerPool();
        TestAdministrator.GenerateAndTestAutomatedJobs();
        TestAdministrator.StartMonitorJob();
        if (Core::ERROR_NONE != TestAdministrator.WaitForTestCompletion(200*1000))
            printf("\n\n ********\n Test could not finish in 200 Seconds..\n\n ********\n");
        TestAdministrator.ResetEvent();
        TestAdministrator.GenerateTestReport();
        TestAdministrator.Stop();

        printf("..Test Run %d out of %d Completed\n", i+1, nTestCount);
        if (i != nTestCount-1) {
            printf("\nStarting next Run in 1 Second..\n");
            SleepMs(1000);
        }
    }
}
