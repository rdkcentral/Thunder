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
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif

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
        TimerType(const TimerType&);
        TimerType& operator=(const TimerType&);

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

            inline TimeWorker(TimerType& parent, const uint32_t stackSize, const TCHAR* timerName)
                : Thread(stackSize, timerName)
                , m_Parent(parent)
            {
            }
            inline ~TimeWorker()
            {
            }

            virtual uint32_t Worker()
            {
                return (m_Parent.Process());
            }

        private:
            TimerType<CONTENT>& m_Parent;
        };

        typedef TimedInfo<CONTENT> TimeInfoBlocks;
        typedef typename std::list<TimeInfoBlocks> SubscriberList;

    public:
        TimerType(const uint32_t stackSize, const TCHAR* timerName)
            : m_PendingQueue()
            , m_TimerThread(*this, stackSize, timerName)
            , m_Admin()
            , m_NextTrigger(NUMBER_MAX_UNSIGNED(uint64_t))
        {
            // Everything is initialized, go...
            m_TimerThread.Block();
        }
        ~TimerType()
        {
            m_Admin.Lock();

            m_TimerThread.Stop();

            // Force kill on all pending stuff...
            m_PendingQueue.clear();
            m_Admin.Unlock();

            m_TimerThread.Wait(Thread::BLOCKED|Thread::STOPPED, Core::infinite);
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

    private:
        void Schedule(TimedInfo<CONTENT>&& timeInfo)
        {
            m_Admin.Lock();

            if (ScheduleEntry(std::move(timeInfo)) == true) {
                m_TimerThread.Run();
            }

            m_Admin.Unlock();
        }

    public:

        void Trigger(const uint64_t& time, const CONTENT& info)
        {
            TimedInfo<CONTENT> newEntry(time, info);

            m_Admin.Lock();

            typename SubscriberList::iterator index = m_PendingQueue.begin();

            while ((index != m_PendingQueue.end()) && ((*index).Content() != info)) {
                ++index;
            }

            if (index != m_PendingQueue.end()) {
                m_PendingQueue.erase(index);
            }

            if (ScheduleEntry(std::move(newEntry)) == true) {
                m_TimerThread.Run();
            }

            m_Admin.Unlock();
        }

        bool Revoke(const CONTENT& info)
        {
            bool foundElement = false;

            m_Admin.Lock();

            typename SubscriberList::iterator index = m_PendingQueue.begin();

            // Since we have the admin lock, we are pretty sure that there is not any
            // context running, so we can be pretty sure that if it was scheduled, it
            // is gone !!!
            if (RemoveEntry(index, info, foundElement) == true) {

                // If we added the new time up front, retrigger the scheduler.
                m_TimerThread.Run();
            }

            m_Admin.Unlock();

            return (foundElement);
        }

        uint64_t NextTrigger() const
        {
            return (m_NextTrigger);
        }

        uint32_t Pending() const
        {
            return (m_PendingQueue.size());
        }

        ::ThreadId ThreadId() const
        {
            return (m_TimerThread.Id());
        }

    protected:
        uint32_t Process()
        {
            uint32_t delayTime = Core::infinite;
            uint64_t now = Time::Now().Ticks();

            m_Admin.Lock();

            // Move to a blocked delay state. We would like to have some delay afterwards..
            // Ranging from 0-Core::infinite
            m_TimerThread.Block();

            while ((m_PendingQueue.empty() == false) && (m_PendingQueue.front().ScheduleTime() <= now)) {
                TimedInfo<CONTENT> info(std::move(m_PendingQueue.front()));

                // Make sure we loose the current one before we do the call, that one might add ;-)
                m_PendingQueue.pop_front();

                m_Admin.Unlock();

                uint64_t reschedule = info.Content().Timed(info.ScheduleTime());

                m_Admin.Lock();

                if (reschedule != 0) {
                    ASSERT(reschedule > now);

                    info.ScheduleTime(reschedule);
                    ScheduleEntry(std::move(info));
                }
            }

            // Calculate the delay...
            if (m_PendingQueue.empty() == true) {
                m_NextTrigger = NUMBER_MAX_UNSIGNED(uint64_t);
            } else {
                // Refresh the time, just to be on the safe side...
                uint64_t delta = Time::Now().Ticks();

                if (delta >= m_PendingQueue.front().ScheduleTime()) {
                    m_NextTrigger = delta;
                    delayTime = 0;
                } else {
                    // The windows counter is in 100ns intervals dus we mmoeten even delen door  1000 (us) * 10 ns = 10.000
                    // om de waarde in ms te krijgen.
                    m_NextTrigger = m_PendingQueue.front().ScheduleTime();
                    delayTime = static_cast<uint32_t>((m_NextTrigger - delta) / Time::TicksPerMillisecond);
                }
            }

            m_Admin.Unlock();

            return (delayTime);
        }

    private:
        bool ScheduleEntry(TimedInfo<CONTENT>&& infoBlock)
        {
            bool reevaluate = false;
            typename SubscriberList::iterator index = m_PendingQueue.begin();

            while ((index != m_PendingQueue.end()) && (infoBlock.ScheduleTime() >= (*index).ScheduleTime())) {
                ++index;
            }

            if (index == m_PendingQueue.begin()) {
                m_PendingQueue.push_front(std::move(infoBlock));

                // If we added the new time up front, retrigger the scheduler.
                reevaluate = true;
            } else if (index == m_PendingQueue.end()) {
                m_PendingQueue.push_back(std::move(infoBlock));
            } else {
                m_PendingQueue.insert(index, std::move(infoBlock));
            }

            return (reevaluate);
        }
        bool RemoveEntry(typename SubscriberList::iterator& index, const CONTENT& info, bool& found)
        {
            bool changedHead = false;

            found = false;

            while (index != m_PendingQueue.end()) {
                if (index->Content() == info) {
                    changedHead |= (index == m_PendingQueue.begin());
                    found = true;

                    // Remove this... Found it, remove it.
                    index = m_PendingQueue.erase(index);
                } else {

                    ++index;
                }
            }

            return (changedHead);
        }

    private:
        SubscriberList m_PendingQueue;
        TimeWorker m_TimerThread;
        CriticalSection m_Admin;
        uint64_t m_NextTrigger;
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

#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

#endif // __TIMER_H
