#pragma once

#include "Thread.h"
#include "Timer.h"
#include <atomic>
#include <functional>

namespace WPEFramework {

namespace Core {

    class EXTERNAL WorkerPool {
    private:
        WorkerPool() = delete;
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

        class Job {
        public:
            Job()
                : _job()
            {
            }
            Job(const Job& copy)
                : _job(copy._job)
            {
            }
            Job(const Core::ProxyType<Core::IDispatch>& job)
                : _job(job)
            {
            }
            ~Job()
            {
            }
            Job& operator=(const Job& RHS)
            {
                _job = RHS._job;

                return (*this);
            }

        public:
            bool operator==(const Job& RHS) const
            {
                return (_job == RHS._job);
            }
            bool operator!=(const Job& RHS) const
            {
                return (!operator==(RHS));
            }
            uint64_t Timed(const uint64_t /* scheduledTime */)
            {
               WorkerPool::Instance().Submit(_job);
                _job.Release();

                // No need to reschedule, just drop it..
                return (0);
            }
            inline void Dispatch()
            {
                ASSERT(_job.IsValid() == true);
                _job->Dispatch();
                _job.Release();
            }

        private:
            Core::ProxyType<Core::IDispatch> _job;
        };

    protected:
        class Minion : public Core::Thread {
        private:
            Minion(const Minion&) = delete;
            Minion& operator=(const Minion&) = delete;

        public:
            Minion()
                : Core::Thread(Core::Thread::DefaultStackSize(), nullptr)
                , _parent(nullptr)
                , _index(0)
            {
            }
            Minion(const uint32_t stackSize)
                : Core::Thread(stackSize, nullptr)
                , _parent(nullptr)
                , _index(0)
            {
            }
            virtual ~Minion()
            {
                Stop();
                Wait(Core::Thread::STOPPED, Core::infinite);
            }

        public:
            void Set(WorkerPool& parent, const uint8_t index)
            {
                _parent = &parent;
                _index = index;
            }

        private:
            virtual uint32_t Worker() override
            {
                _parent->Process(_index);
                Block();
                return (Core::infinite);
            }

        private:
            WorkerPool* _parent;
            uint8_t _index;
        };

        typedef Core::QueueType<Job> MessageQueue;

    public:
        struct Metadata {
            uint32_t Pending;
            uint32_t Occupation;
            uint8_t Slots;
            uint32_t* Slot;
        };

    public:
	static WorkerPool& Instance() {
            ASSERT(_instance != nullptr);
            return (*_instance);
	}
	static bool IsAvailable() {
            return (_instance != nullptr);
	}
        ~WorkerPool();

    public:
        inline void Submit(const Core::ProxyType<Core::IDispatch>& job)
        {
            _handleQueue.Insert(Job(job), Core::infinite);
        }
        inline void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job)
        {
            _timer.Schedule(time, Job(job));
        }
        inline uint32_t Revoke(const Core::ProxyType<Core::IDispatch>& job, const uint32_t waitTime = Core::infinite)
        {
            Job compare(job);
            return (_timer.Revoke(compare) == true || _handleQueue.Remove(compare) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        inline const WorkerPool::Metadata& Snapshot()
        {
            _metadata.Occupation = _occupation.load();
            _metadata.Pending = _handleQueue.Length();
            return (_metadata);
        }
	void Join() {
            Process(0);
	}
        void Run()
        {
            _handleQueue.Enable();
            for (uint8_t index = 1; index < _metadata.Slots; index++) {
                Minion& minion = Index(index);
                minion.Set(*this, index);
                minion.Run();
            }
        }
        void Stop()
        {
            _handleQueue.Disable();
            for (uint8_t index = 1; index < _metadata.Slots; index++) {
                Minion& minion = Index(index);
                minion.Block();
                minion.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
            }
        }

        inline ThreadId Id(const uint8_t index) const {
            ThreadId result = 0;

            if (index == 0) {
                result = _timer.ThreadId();
            }
            else if (index == 1) {
                result = 0;
            }
            else if (index < _metadata.Slots) {
                result = const_cast<WorkerPool&>(*this).Index(index - 1).ThreadId();
            }

            return (result);
        }

    protected:
        WorkerPool(const uint8_t threadCount, uint32_t* counters);

        virtual Minion& Index(const uint8_t index) = 0;
        virtual bool Running() = 0;

        void Process(const uint8_t index)
        {
            Job newRequest;

            while ((Running() == true) && (_handleQueue.Extract(newRequest, Core::infinite) == true)) {

                _metadata.Slot[index]++;

                _occupation++;

                newRequest.Dispatch();

                _occupation--;
            }
        }

    private:
        MessageQueue _handleQueue;
        std::atomic<uint8_t> _occupation;
        Core::TimerType<Job> _timer;
        Metadata _metadata;
        static WorkerPool* _instance;
    };

    template <const uint8_t THREAD_COUNT>
    class WorkerPoolType : public WorkerPool {
    private:
        WorkerPoolType() = delete;
        WorkerPoolType(const WorkerPoolType<THREAD_COUNT>&) = delete;
        WorkerPoolType<THREAD_COUNT>& operator=(const WorkerPoolType<THREAD_COUNT>&) = delete;

    public:
        WorkerPoolType(const uint32_t stackSize)
            : WorkerPool(THREAD_COUNT, &(_counters[0]))
            , _minions()
        {
        }
        virtual ~WorkerPoolType()
        {
            Stop();
        }

        inline uint32_t ThreadId(const uint8_t index) const 
        {
            return (((index > 0) && (index < THREAD_COUNT)) ? _minions[index-1].ThreadId() : static_cast<uint32_t>(~0));
        }

    private:
        virtual Minion& Index(const uint8_t index) override 
        {
            return (_minions[index-1]);
        }
        virtual bool Running() override
        {
            return (true);
        }

    private:
        Minion _minions[THREAD_COUNT - 1];
        uint32_t _counters[THREAD_COUNT];
    };
}
}
