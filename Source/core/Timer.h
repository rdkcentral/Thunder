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
 
#ifndef __TIMER_H
#define __TIMER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "Sync.h"
#include "Thread.h"
#include "Time.h"
#include <utility>

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

// ---- Class Definition ----

//
// Description: Helper class to use pointers or proxies (if lifetime management needs to be automated)
//              as a carrier to be executed by the threadpooltype.
//
namespace WPEFramework {
namespace Core {
    template <typename CONTENT>
    class TimerType {

    private:
        template <typename ACTIVECONTENT>
        class TimedInfo {
        public:
            inline TimedInfo()
                : m_ScheduleTime(0)
                , m_Info()
            {
            }

            inline TimedInfo(const uint64_t time, const ACTIVECONTENT& contents)
                : m_ScheduleTime(time)
                , m_Info(contents)
            {
            }

            inline TimedInfo(const uint64_t time, ACTIVECONTENT&& contents)
                : m_ScheduleTime(time)
                , m_Info(std::move(contents))
            {
            }

            inline TimedInfo(const TimedInfo& copy)
                : m_ScheduleTime(copy.m_ScheduleTime)
                , m_Info(copy.m_Info)
            {
            }

            inline TimedInfo(TimedInfo&& copy) noexcept
                : m_ScheduleTime(copy.m_ScheduleTime)
                , m_Info(std::move(copy.m_Info))
            {
            }

            inline ~TimedInfo()
            {
            }

            inline bool operator== (const CONTENT& RHS) const
            {
                return (m_Info == RHS);
            }
            inline bool operator!= (const CONTENT& RHS) const
            {
                return (m_Info != RHS);
            }

            inline TimedInfo& operator=(const TimedInfo& RHS)
            {
                m_ScheduleTime = RHS.m_ScheduleTime;
                m_Info = RHS.m_Info;

                return (*this);
            }

            inline TimedInfo& operator=(TimedInfo&& RHS)
            {
                m_ScheduleTime = RHS.m_ScheduleTime;
                m_Info = std::move(RHS.m_Info);

                return (*this);
            }

            inline uint64_t ScheduleTime() const
            {
                return (m_ScheduleTime);
            }

            inline void ScheduleTime(const uint64_t scheduleTime)
            {
                m_ScheduleTime = scheduleTime;
            }

            inline ACTIVECONTENT& Content()
            {
                return (m_Info);
            }

        private:
            uint64_t m_ScheduleTime;
            ACTIVECONTENT m_Info;
        };

        class TimeWorker : public Thread {
        public:
            TimeWorker() = delete;
            TimeWorker(const TimeWorker&) = delete;
            TimeWorker& operator=(const TimeWorker&) = delete;

            TimeWorker(TimerType& parent, const uint32_t stackSize, const TCHAR* timerName)
                : Thread(stackSize, timerName)
                , m_Parent(parent)
            {
            }
            ~TimeWorker() = default;

            uint32_t Worker() override
            {
                return (m_Parent.Process());
            }

        private:
            TimerType<CONTENT>& m_Parent;
        };

        using TimeInfoBlocks = TimedInfo<CONTENT>;
        using SubscriberList = typename std::list<TimeInfoBlocks>;

    public:
        TimerType(const TimerType&) = delete;
        TimerType& operator=(const TimerType&) = delete;

        TimerType(const uint32_t stackSize, const TCHAR* timerName)
            : _pendingQueue()
            , _timerThread(*this, stackSize, timerName)
            , _adminLock()
            , _nextTrigger(NUMBER_MAX_UNSIGNED(uint64_t))
            , _waitForCompletion(true, true)
            , _executing(nullptr)

        {
            // Everything is initialized, go...
            _timerThread.Block();
        }
        ~TimerType()
        {
            _adminLock.Lock();

            _timerThread.Stop();

            // Force kill on all pending stuff...
            _pendingQueue.clear();

            _adminLock.Unlock();

            _timerThread.Wait(Thread::STOPPED, Core::infinite);
        }

        inline void Schedule(const Time& time, CONTENT&& info)
        {
            Schedule(time.Ticks(), std::move(info));
        }

        inline void Schedule(const Time& time, const CONTENT& info)
        {
            Schedule(time.Ticks(), info);
        }

        inline void Schedule(const uint64_t& time, CONTENT&& info)
        {
            Schedule(TimedInfo<CONTENT>(time, std::move(info)));
        }

        inline void Schedule(const uint64_t& time, const CONTENT& info)
        {
            Schedule(std::move(TimedInfo<CONTENT>(time, info)));
        }

        inline void Flush() {
            _adminLock.Lock();

            _timerThread.Block();

            // Force kill on all pending stuff...
            _pendingQueue.clear();
            _adminLock.Unlock();

            _timerThread.Wait(Thread::BLOCKED, Core::infinite);
        }

        inline bool HasEntry(const CONTENT& element) const {

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            typename SubscriberList::const_iterator index = _pendingQueue.cbegin();

            // Clear all entries !!
            while ((index != _pendingQueue.cend()) && (*index != element)) {
                index++;
            }

            bool found = (index != _pendingQueue.cend());

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (found);
        }

    private:
        void Schedule(TimedInfo<CONTENT>&& timeInfo)
        {
            _adminLock.Lock();

            if (ScheduleEntry(std::move(timeInfo)) == true) {
                _timerThread.Run();
            }

            _adminLock.Unlock();
        }

    public:
        void Trigger(const uint64_t& time, const CONTENT& info)
        {
            TimedInfo<CONTENT> newEntry(time, info);

            _adminLock.Lock();

            typename SubscriberList::iterator index = _pendingQueue.begin();

            while ((index != _pendingQueue.end()) && ((*index).Content() != info)) {
                ++index;
            }

            if (index != _pendingQueue.end()) {
                _pendingQueue.erase(index);
            }

            if (ScheduleEntry(std::move(newEntry)) == true) {
                _timerThread.Run();
            }

            _adminLock.Unlock();
        }

        bool Revoke(const CONTENT& info)
        {
            bool foundElement = false;

            _adminLock.Lock();

            if (&info == _executing) {

                // Seems like we are also executing this ocntext, wait till it is completed and signal that it should not reschedule !!!
                _executing = nullptr;

                _adminLock.Unlock();

                _waitForCompletion.Lock();

                _adminLock.Lock();
            }

            typename SubscriberList::iterator index = _pendingQueue.begin();

            bool changedHead = false;

            while (index != _pendingQueue.end()) {
                if (index->Content() == info) {
                    changedHead |= (index == _pendingQueue.begin());
                    foundElement = true;

                    // Remove this... Found it, remove it.
                    index = _pendingQueue.erase(index);
                }
                else {
                    ++index;
                }
            }

            if (changedHead == true) {
                // If we added the new time up front, retrigger the scheduler.
                _timerThread.Run();
            }

            _adminLock.Unlock();

            return (foundElement);
        }

        uint64_t NextTrigger() const
        {
            return (_nextTrigger);
        }

        uint32_t Pending() const
        {
            return (static_cast<uint32_t>(_pendingQueue.size()));
        }

        ::ThreadId ThreadId() const
        {
            return (_timerThread.Id());
        }

    protected:
        uint32_t Process()
        {
            uint32_t delayTime = Core::infinite;
            uint64_t now = Time::Now().Ticks();

            _adminLock.Lock();

            // Move to a blocked delay state. We would like to have some delay afterwards..
            // Ranging from 0-Core::infinite
            _timerThread.Block();

            while ((_pendingQueue.empty() == false) && (_pendingQueue.front().ScheduleTime() <= now)) {
                TimedInfo<CONTENT> info(std::move(_pendingQueue.front()));
                _executing = &(info.Content());

                // Make sure we loose the current one before we do the call, that one might add ;-)
                _pendingQueue.pop_front();
                _waitForCompletion.ResetEvent();

                _adminLock.Unlock();

                uint64_t reschedule = _executing->Timed(info.ScheduleTime());

                _adminLock.Lock();

                if ((_executing != nullptr) && (reschedule != 0)) {
                    ASSERT(reschedule > now);

                    info.ScheduleTime(reschedule);
                    ScheduleEntry(std::move(info));
                }

                _waitForCompletion.SetEvent();
                _executing = nullptr;
            }

            // Calculate the delay...
            if (_pendingQueue.empty() == true) {
                _nextTrigger = NUMBER_MAX_UNSIGNED(uint64_t);
            } else {
                // Refresh the time, just to be on the safe side...
                uint64_t delta = Time::Now().Ticks();

                if (delta >= _pendingQueue.front().ScheduleTime()) {
                    _nextTrigger = delta;
                    delayTime = 0;
                } else {
                    // The windows counter is in 100ns intervals dus we mmoeten even delen door  1000 (us) * 10 ns = 10.000
                    // om de waarde in ms te krijgen.
                    _nextTrigger = _pendingQueue.front().ScheduleTime();
                    delayTime = static_cast<uint32_t>((_nextTrigger - delta) / Time::TicksPerMillisecond);
                }
            }

            _adminLock.Unlock();

            return (delayTime);
        }

    private:
        bool ScheduleEntry(TimedInfo<CONTENT>&& infoBlock)
        {
            bool reevaluate = false;
            typename SubscriberList::iterator index = _pendingQueue.begin();

            while ((index != _pendingQueue.end()) && (infoBlock.ScheduleTime() >= (*index).ScheduleTime())) {
                ++index;
            }

            if (index == _pendingQueue.begin()) {
                _pendingQueue.push_front(std::move(infoBlock));

                // If we added the new time up front, retrigger the scheduler.
                reevaluate = true;
            } else if (index == _pendingQueue.end()) {
                _pendingQueue.push_back(std::move(infoBlock));
            } else {
                _pendingQueue.insert(index, std::move(infoBlock));
            }

            return (reevaluate);
        }

    private:
        SubscriberList _pendingQueue;
        TimeWorker _timerThread;
        mutable CriticalSection _adminLock;
        uint64_t _nextTrigger;
        Core::Event _waitForCompletion;
        CONTENT* _executing;
    };

    template <typename HANDLER>
    class WatchDogType : public Thread {
    public:
        WatchDogType() = delete;
        WatchDogType(const WatchDogType<HANDLER>&) = delete;
        WatchDogType& operator=(const WatchDogType&) = delete;
        WatchDogType(const uint32_t stackSize, const TCHAR* threadName)
            : Thread(stackSize, threadName)
            , _job()
            , _delay(Core::infinite)
        {
            Initialize();
        }
        WatchDogType(const uint32_t stackSize, const TCHAR* threadName, HANDLER source)
            : Thread(stackSize, threadName)
            , _job(source)
            , _delay(Core::infinite)
        {
            Initialize();
        }
        ~WatchDogType()
        {
            Terminate();
        }
        inline void Arm(uint32_t waitTime)
        {
            Thread::Lock();
            _delay = waitTime;
            Run();
            Thread::Unlock();
        }
        inline void Arm(uint32_t waitTime, HANDLER source)
        {
            Thread::Lock();
            _delay = waitTime;
            _job = source;
            Run();
            Thread::Unlock();
        }
        inline void Reset()
        {
            Thread::Lock();
            _delay = Core::infinite;
            Run();
            Thread::Unlock();
        }

    private:
        virtual uint32_t Worker()
        {
            uint32_t nextDelay = Core::infinite;

            Thread::Lock();

            Block();

            if (_delay == 0) {
                nextDelay = _job.Expired();
            } else {
                nextDelay = _delay;
                _delay = 0;
            }

            Thread::Unlock();

            return (nextDelay);
        }

    private:
        HANDLER _job;
        volatile uint32_t _delay;
    };
}
} // namespace Core

POP_WARNING()

#endif // __TIMER_H
