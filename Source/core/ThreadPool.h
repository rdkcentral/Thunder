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
 
#pragma once

#include "Thread.h"
#include "ResourceMonitor.h"

namespace WPEFramework {

namespace Core {

    class EXTERNAL ThreadPool {
    public:
        typedef Core::QueueType< Core::ProxyType<IDispatch> > MessageQueue;

        struct IDispatcher {
            virtual ~IDispatcher() = default;

            virtual void Initialize() = 0;
            virtual void Deinitialize() = 0;
            virtual void Dispatch(Core::IDispatchType<void>*) = 0;
        };

        template<typename IMPLEMENTATION>
        class JobType {
        private:
            enum state : uint8_t {
                IDLE,
                SUBMITTED
            };

            class Worker : public Core::IDispatch {
            public:
                Worker() = delete;
                Worker(const Worker&) = delete;
                Worker& operator=(const Worker&) = delete;

                Worker(JobType<IMPLEMENTATION>& parent) : _parent(parent) {
                }
                ~Worker() override {
                }

            private:
                void Dispatch() override {
                    _parent.Dispatch();
                }

            private:
                 JobType<IMPLEMENTATION>& _parent;
            };

        public:
            JobType(const JobType<IMPLEMENTATION>& copy) = delete;
            JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>& RHS) = delete;

            #ifdef __WINDOWS__
            #pragma warning(disable: 4355)
            #endif
            template <typename... Args>
            JobType(Args&&... args)
                : _implementation(args...)
                , _state(IDLE)
                , _job(*this)
            {
                _job.AddRef();
            }
            #ifdef __WINDOWS__
            #pragma warning(default: 4355)
            #endif

            ~JobType()
            {
                ASSERT (_state == IDLE);
                _job.CompositRelease();
            }

        public:
            Core::ProxyType<Core::IDispatch> Aquire() {

                state expected = IDLE;
                Core::ProxyType<Core::IDispatch> result;

                if (_state.compare_exchange_strong(expected, SUBMITTED) == true) {
                    result = Core::ProxyType<Core::IDispatch>(&_job, &_job);
                }

                return (result);
            }
            Core::ProxyType<Core::IDispatch> Reset() {

                state expected = SUBMITTED;
                _state.compare_exchange_strong(expected, IDLE);

                return (Core::ProxyType<Core::IDispatch>(&_job, &_job));
            }
            operator IMPLEMENTATION& () {
                return (_implementation);
            }
            operator const IMPLEMENTATION& () const {
                return (_implementation);
            }

        protected:
             Core::ProxyType<Core::IDispatch> Forced() {

                state expected = IDLE;
                _state.compare_exchange_strong(expected, SUBMITTED);

                return (Core::ProxyType<Core::IDispatch>(&_job, &_job));
            }
            

        private:
            void Dispatch()
            {
                state expected = SUBMITTED;
                if (_state.compare_exchange_strong(expected, IDLE) == true) {
                    _implementation.Dispatch();
                }
            }

        private:
            IMPLEMENTATION _implementation;
            std::atomic<state> _state;
            ProxyObject<Worker> _job;
        };

        class EXTERNAL Minion {
        public:
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

            Minion(MessageQueue& queue, IDispatcher* dispatcher)
                : _dispatcher(dispatcher)
                , _queue(queue)
                , _adminLock()
                , _signal(false, true)
                , _interestCount(0)
                , _currentRequest()
                , _runs(0)
            {
		ASSERT(dispatcher != nullptr);
            }
            ~Minion() = default;

        public:
            uint32_t Runs() const {
                return (_runs);
            }
            bool IsActive() const {
                return (_currentRequest.IsValid());
            }
            uint32_t Completed (const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime) {
                uint32_t result = Core::ERROR_NONE;

                _adminLock.Lock();
                _interestCount++;

                if (_currentRequest != job) {
                    _adminLock.Unlock();
                }
                else {
                    _adminLock.Unlock();
                    result = _signal.Lock(waitTime);
                }

                _interestCount--;

                return(result);
            }
            void Process()
            {
		_dispatcher->Initialize();

                while (_queue.Extract(_currentRequest, Core::infinite) == true) {

                    ASSERT(_currentRequest.IsValid() == true);

                    _runs++;

                    Core::IDispatch* request = &(*_currentRequest);
                    _dispatcher->Dispatch(request);
                    _currentRequest.Release();

                    // if someone is observing this run, (WaitForCompletion) make sure that
                    // thread, sees that his object was running and is now completed.
                    _adminLock.Lock();
                    if (_interestCount > 0) {

                        _signal.SetEvent();

                        while (_interestCount > 0) {
                            std::this_thread::yield();
                        }

                        _signal.ResetEvent();
                    }
                    _adminLock.Unlock();
                }

		_dispatcher->Deinitialize();
            }

        private:
            IDispatcher* _dispatcher;
            MessageQueue& _queue;
            Core::CriticalSection _adminLock;
            Core::Event _signal;
            std::atomic<uint32_t> _interestCount;
            Core::ProxyType<Core::IDispatch> _currentRequest;
            uint32_t _runs;
        };

    private:
        class EXTERNAL Executor : public Core::Thread {
        public:
            Executor() = delete;
            Executor(const Executor&) = delete;
            Executor& operator=(const Executor&) = delete;

            Executor(MessageQueue& queue, IDispatcher* dispatcher, const uint32_t stackSize, const TCHAR* name)
                : Core::Thread(stackSize == 0 ? Core::Thread::DefaultStackSize() : stackSize, name)
                , _minion(queue, dispatcher)
            {
            }
            ~Executor() override
            {
                Thread::Stop();
                Wait(Core::Thread::STOPPED, Core::infinite);
            }

        public:
            uint32_t Runs() const {
                return (_minion.Runs());
            }
            bool IsActive() const {
                return (_minion.IsActive());
            }
            void Run () {
                Core::Thread::Run();
            }
            void Stop () {
                Core::Thread::Wait(Core::Thread::STOPPED|Core::Thread::BLOCKED, Core::infinite);
            }
            Minion& Me() {
                return (_minion);
            }

        private:
            uint32_t Worker() override
            {
                _minion.Process();
                Core::Thread::Block();
                return (Core::infinite);
            }

        private:
            Minion _minion;
        };

    public:
        ThreadPool(const ThreadPool& a_Copy) = delete;
        ThreadPool& operator=(const ThreadPool& a_RHS) = delete;

        ThreadPool(const uint8_t count, const uint32_t stackSize, const uint32_t queueSize, IDispatcher* dispatcher) 
            : _queue(queueSize)
        {
            const TCHAR* name = _T("WorkerPool::Thread");
            for (uint8_t index = 0; index < count; index++) {
                _units.emplace_back(_queue, dispatcher, stackSize, name);
            }
        }
        ~ThreadPool() {
            Stop();
            _units.clear();
        }

    public:
        uint8_t Count() const
        {
            return (static_cast<uint8_t>(_units.size()));
        }
        uint32_t Pending() const
        {
            return (_queue.Length());
        }
        void Runs(const uint8_t length, uint32_t* counters) const 
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while ((count < length) && (ptr != _units.cend())) { 
                counters[count] = ptr->Runs();
                ptr++; 
                count++; 
            }
        }
        uint8_t Active() const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while (ptr != _units.cend()) 
            { 
                if (ptr->IsActive() == true) {
                    count++;
                }
                ptr++; 
            }

            return (count);
        }
        ::ThreadId Id(const uint8_t index) const
        {
            uint8_t count = 0;
            std::list<Executor>::const_iterator ptr = _units.cbegin();
            while ((index != count) && (ptr != _units.cend())) { ptr++; count++; }

            ASSERT (ptr != _units.cend());

            return (ptr != _units.cend() ? ptr->Id() : 0);
        }
        void Submit(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            if (Core::Thread::ThreadId() == ResourceMonitor::Instance().Id()) {
                _queue.Post(job);
            }
            else {
                _queue.Insert(job, waitTime);
            }

        }
        void Post(const Core::ProxyType<IDispatch>& job)
        {
            _queue.Post(job);
        }
        uint32_t Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime)
        {
            uint32_t result = Core::ERROR_NONE;

            _queue.Remove(job);

            // Check if it is currently being executed and wait till it is done.
            std::list<Executor>::iterator index = _units.begin();

            while (index != _units.end()) {
                uint32_t outcome = index->Me().Completed(job, waitTime);
                if (outcome != Core::ERROR_NONE) {
                    result = outcome;
                }
                index++;
            }

            return (result);
        }
        MessageQueue& Queue() {
            return (_queue);
        }
        void Run()
        {
            _queue.Enable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Run();
                index++;
            }
        }
        void Stop()
        {
            _queue.Disable();
            std::list<Executor>::iterator index = _units.begin();
            while (index != _units.end()) {
                index->Stop();
                index++;
            }
        }

    private:
        MessageQueue _queue;
        std::list<Executor> _units;
    };

}
} // namespace Core
