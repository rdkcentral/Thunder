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

#ifdef __APPLE__
#include <time.h>
#endif

#include <iostream>
#include <chrono>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

// The tests are 'equivalent' to the flow of ::Thunder::Core::CyclicBuffer with pthread_cond_signal
// or with pthread_cond_broadcvast as in the proposed patch. They illustrate 'broadcast to wake up any
// blocked threads / processes simplifies code and it gurantueed by the standard :
// https://pubs.opengroup.org/onlinepubs/9799919799/functions/pthread_cond_broadcast.html
// Test 'Deadlocked' is deliberately disabled to safeguard progression for this unit.

namespace Thunder {
namespace Tests {
namespace Core {

    template <typename DERIVED>
    class SharedAccess {
    public :

        SharedAccess(const SharedAccess&) = delete;
        SharedAccess(SharedAccess&&) = delete;
        SharedAccess& operator=(const SharedAccess&) = delete;
        SharedAccess& operator=(SharedAccess&&) = delete;

        ~SharedAccess()
        {}

        uint32_t Lock(uint32_t waitTime = ::Thunder::Core::infinite)
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {

                result = ::Thunder::Core::ERROR_TIMEDOUT;

                // In the original ::Thunder::Core::CyclicBuffer the flow is:
                // 1. AdminLock
                // 2. Do while
                // 3. AdminUnlock

                uint32_t timeLeft{ waitTime };

                result = AdminLock();

                do {

                    // Force the second branch for illustrative purpose

                    if (_locked.fetch_or(1) == 0) {
                    } else if (true) {

                        // In the original :: Thunder::Core::CyclicBuffer the flow is:
                        // 1. AdminUnlock
                        // 2. SignalLock
                        // 3. AdminLock
                        //
                        // However, METROL-1188 is required to comply to the requirement a
                        // locked mutex prior pthread_cond_(timed)wait. Hence the 'correct'
                        // flow :
                        // 1. SignalLock
                        // 2. AdminUnlock
                        // 3. AdminLock

                        _agents++;

                        // Any blocked thread / process may continue on a signal, all, 
                        // continue on a broadcast

                        // For illustrative purpose the return value has a diffent meaning: error code
                        result = SignalLock(timeLeft);

                        _agents--;

                        if (result == ::Thunder::Core::ERROR_NONE) {
                            result = AdminUnlock();
                        } else {
                            /* uint32_t */ AdminUnlock();
                        }

                        // All contenders 'wait' here until the guarded Reevaluate has completed
                        if (result == ::Thunder::Core::ERROR_NONE) {
                            result = AdminLock();
                        } else {
                            /* uint32_t */ AdminLock();
                        }

                    }

                  // For illustrative purpose break the loop

                } while (false);

                if (result == ::Thunder::Core::ERROR_NONE) {
                    result = AdminUnlock();
                } else {
                    /* uint32_t */ AdminUnlock();
                }
            }

            return result;
        }

        uint32_t Unlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {
                result = static_cast<DERIVED*>(this)->UnlockImplementation();
            }

            return result;
        }

        int Error() const
        {
            return _errval;
        }

        uint32_t Agents() const
        {
            return _agents;
        }

    protected :

        SharedAccess()
        {}

        uint32_t AdminLock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            if ((errval = pthread_mutex_lock(&_mutex)) != 0) {
                _errval = errval;

                result = ::Thunder::Core::ERROR_GENERAL;
            }

            return result;
        }

        uint32_t AdminUnlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            if ((errval = pthread_mutex_unlock(&_mutex)) != 0) {
                _errval = errval;

                result = ::Thunder::Core::ERROR_GENERAL;
            }

            return result;
        }

        // For illustrative purpose the return value has a different meaning: error code
        uint32_t SignalLock(uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_GENERAL };

            int errval{ 0 };

            if (waitTime != ::Thunder::Core::infinite) {

                // For illustrative purpose this branch should not be used
                ASSERT(false);

            } else {

                // Assume the 'lock' is successfully released
                // Assume execution is (almost) instant as in no other process is able to get rescheduled at hte calling site

                if ((errval = pthread_cond_wait(&_condition, &_mutex)) == 0) {
                    result = :: Thunder::Core::ERROR_NONE;
                 } else {
                    _errval = errval;
                 }

            }

            return result;
        }

        // Return value added in constrast to original
        uint32_t Reevaluate()
        {
            uint32_t result{ ::Thunder::Core::ERROR_GENERAL };

            if (_initialized != false) {
                result = static_cast<DERIVED*>(this)->ReevaluateImplementation();
            }

            return result;
        }

        pthread_cond_t _condition{};
        pthread_mutex_t _mutex{};
        const bool _initialized{ Initialize() };
        std::atomic<uint32_t> _agents{ 0 };
        int _errval{ 0 };

    private :

        bool Initialize()
        {
            pthread_condattr_t condition_attr;
            pthread_mutexattr_t mutex_attr;

            bool result =    pthread_condattr_init(&condition_attr) == 0
                          && pthread_condattr_setclock(&condition_attr, CLOCK_MONOTONIC) == 0
                          && pthread_cond_init(&_condition, &condition_attr) == 0
                          && pthread_mutexattr_init(&mutex_attr) == 0
                          && pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK) == 0
                          && pthread_mutex_init(&_mutex, &mutex_attr) == 0
                          ;

            return result;
        }

        std::atomic<uint8_t> _locked{ 0 };
    };

    class SharedAccessWrong : public SharedAccess<SharedAccessWrong> {
    public :
        SharedAccessWrong(const SharedAccessWrong&) = delete;
        SharedAccessWrong(SharedAccessWrong&&) = delete;
        SharedAccessWrong& operator=(const SharedAccessWrong&) = delete;
        SharedAccessWrong& operator=(SharedAccessWrong&&) = delete;

        SharedAccessWrong() = default;
        ~SharedAccessWrong() = default;

        uint32_t UnlockImplementation()
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {

                // Flow 'copied' at ::Thunder::Core::CyclicBuffer is:
                // 1. AdminLock()
                // 2. Reevaluate()
                // 3. AdminUnlock()

                if (   this->AdminLock() == ::Thunder::Core::ERROR_NONE
                    && this->Reevaluate() == ::Thunder::Core::ERROR_NONE
                    && this->AdminUnlock() == ::Thunder::Core::ERROR_NONE
                ) {
                    result = ::Thunder::Core::ERROR_NONE;
                } else {
                    result = ::Thunder::Core::ERROR_GENERAL;
                }

            }

            return result;
        }

        // Return value added in constrast to original
        uint32_t ReevaluateImplementation()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };
            
            // Flow of interest at ::Thunder::Core::CyclicBuffer:
            // 1. agents > 0, eg, there are blocked threads / processes
            // 2. at least one unique blocked thread / process is released
            // 3. wait until agents == 0

            // Asumme signal delivery has been successful to blocked threads /
            // processes

            // 'Signal' is guarantueed to wake up one or more blocked threads /
            // processes
            for (uint32_t index = _agents.load(); index != 0; index--) {
                if ((errval = pthread_cond_signal(&_condition)) == 0) {
                } else {
                    _errval = errval;
                    result = ::Thunder::Core::ERROR_GENERAL;
                    break;
                }
            }

            uint8_t count{ 0 };

            // Wait till all waiters have seen the trigger..
            while (_agents.load() > 0) {
                ::Thunder::Core::Thread::Yield(count);
            }

            return result;
        }

    private :
    };

    class SharedAccessGood : public SharedAccess<SharedAccessGood> {
    public :
        SharedAccessGood(const SharedAccessGood&) = delete;
        SharedAccessGood(SharedAccessGood&&) = delete;
        SharedAccessGood& operator=(const SharedAccessGood&) = delete;
        SharedAccessGood& operator=(SharedAccessGood&&) = delete;

        SharedAccessGood() = default;
        ~SharedAccessGood() = default;

        uint32_t UnlockImplementation()
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {
                // Reevaluate without mutex-lock-unlock 
                result = this->Reevaluate();
            }

            return result;
        }

        // Return value added in constrast to original
        uint32_t ReevaluateImplementation()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            // Flow of interest at ::Thunder::Core::CyclicBuffer:
            // 1. agents > 0, eg, there are blocked threads / processes
            // 2. at least one unique blocked thread / process is released
            // 3. wait until agents == 0

            // Asumme signal delivery has been successful to blocked threads /
            // processes

            // 'Signal' is guarantueed to wake up one or more blocked threads /
            // processes
            for (uint32_t index = _agents.load(); index != 0; index--) {
                if ((errval = pthread_cond_signal(&_condition)) == 0) {
                } else {
                    _errval = errval;
                    result = ::Thunder::Core::ERROR_GENERAL;
                    break;
                }
            }

            uint8_t count{ 0 };

            ::Thunder::Core::Thread::Yield(count);

            return result;
        }
 
    private :

    };

    template <typename T>
    class ProcessEmulator : public ::Thunder::Core::Thread
    {
    public :
        ProcessEmulator(const ProcessEmulator&) = delete;

        ProcessEmulator& operator=(const ProcessEmulator&) = delete;
        ProcessEmulator& operator=(ProcessEmulator&&) = delete;

        ProcessEmulator(uint32_t workTime, uint32_t waitTime, T& sharedBuffer)
            : ::Thunder::Core::Thread{}
            , _sharedBuffer{ sharedBuffer }
            , _workTime{ workTime }
            , _waitTime{ waitTime }
        {}

        ProcessEmulator(ProcessEmulator&& other)
            : _sharedBuffer{ other._sharedBuffer }
            , _workTime{ other._workTime }
            , _waitTime{ other._waitTime }
        {}

        ~ProcessEmulator() override
        {
            // Educated guess
            constexpr uint32_t maxCleanupTime{ 1000 };

            Stop();

            /* bool */ Wait(::Thunder::Core::Thread::STOPPED | ::Thunder::Core::Thread::BLOCKED | ::Thunder::Core::Thread::STOPPING, maxCleanupTime); 
        }

        uint32_t Worker() override 
        {
            constexpr uint32_t result{ ::Thunder::Core::infinite };

            /* uint32_t */ _sharedBuffer.Lock(_waitTime);

            // Emulate time spending on work
            SleepMs(_workTime);

            /* uint32_t */ _sharedBuffer.Unlock();

            Stop();

            return result;
        }

    private :
        ProcessEmulator()
            : ProcessEmulator{ 0 }
        {}

        T& _sharedBuffer;

        const uint32_t _workTime;
        const uint32_t _waitTime;
    };

    // Disable to prevent any completion of all tests
    TEST(METROL_1199, DISABLED_Deadlocked)
    {
        constexpr size_t N{ 3 };

        // Educated guess
        constexpr uint32_t workTime{ 3000 };
        constexpr uint32_t waitTime{ ::Thunder::Core::infinite };

        SharedAccessWrong sharedBuffer;

        ProcessEmulator<SharedAccessWrong> nonAgent{ ProcessEmulator<SharedAccessWrong>{ (N + 1) * workTime , waitTime, sharedBuffer } };

        std::array<ProcessEmulator<SharedAccessWrong>, N> agents = { ProcessEmulator<SharedAccessWrong>{ workTime, waitTime, sharedBuffer }, ProcessEmulator<SharedAccessWrong>{ workTime, waitTime, sharedBuffer }, ProcessEmulator<SharedAccessWrong>{ workTime, waitTime, sharedBuffer } };

        nonAgent.Run();

        // Give it time to take the lock
        SleepMs(1000);

        for (auto& agent : agents) {
            agent.Run();
        }  

        // Let all agents get to the SignalLock
        SleepMs(1000);

        EXPECT_EQ(sharedBuffer.Agents(), N);

        ::Thunder::Core::StopWatch stopWatch;

        /* uint64_t */ stopWatch.Reset();

        bool timedOut{ false };

        while (   std::all_of
                  (
                     agents.begin()
                   , agents.end() 
                   , [&workTime](ProcessEmulator<SharedAccessWrong>& agent) { return agent.Wait(::Thunder::Core::Thread::STOPPED | ::Thunder::Core::Thread::STOPPING | ::Thunder::Core::Thread::BLOCKED, workTime) != false; }
                  ) != true
               && !(timedOut = ((stopWatch.Elapsed() / 1000) > ((N + 1) * workTime))) != false
        ) {}
        
        nonAgent.Stop();

        EXPECT_TRUE(nonAgent.Wait(::Thunder::Core::Thread::STOPPED | ::Thunder::Core::Thread::BLOCKED | ::Thunder::Core::Thread::STOPPING));

        EXPECT_EQ(sharedBuffer.Error(), 0);

        EXPECT_EQ(sharedBuffer.Agents(), 0);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(METROL_1199, NonDeadlocked)
    {
        constexpr size_t N{ 3 };

        // Educated guess
        constexpr uint32_t workTime{ 3000 };
        constexpr uint32_t waitTime{ ::Thunder::Core::infinite };

        SharedAccessGood sharedBuffer;

        ProcessEmulator<SharedAccessGood> nonAgent{ ProcessEmulator<SharedAccessGood>{ (N + 1) * workTime, waitTime , sharedBuffer } };

        std::array<ProcessEmulator<SharedAccessGood>, N> agents = { ProcessEmulator<SharedAccessGood>{ workTime, waitTime, sharedBuffer }, ProcessEmulator<SharedAccessGood>{ workTime, waitTime, sharedBuffer }, ProcessEmulator<SharedAccessGood>{ workTime, waitTime, sharedBuffer } };

        nonAgent.Run();

        // Give it time to take the lock
        SleepMs(1000);

        for (auto& agent : agents) {
            agent.Run();
        }  

        // Let all agents get to the SignalLock
        SleepMs(1000);

        EXPECT_EQ(sharedBuffer.Agents(), N);

        ::Thunder::Core::StopWatch stopWatch;

        /* uint64_t */ stopWatch.Reset();

        bool timedOut{ false };

        while (   std::all_of
                  (
                     agents.begin()
                   , agents.end() 
                   , [&workTime](ProcessEmulator<SharedAccessGood>& agent) { return agent.Wait(::Thunder::Core::Thread::STOPPED | ::Thunder::Core::Thread::STOPPING | ::Thunder::Core::Thread::BLOCKED, workTime) != false; }
                  ) != true
               && !(timedOut = ((stopWatch.Elapsed() / 1000) > ((N + 1) * workTime))) != false
        ) {}
        
        nonAgent.Stop();

        EXPECT_TRUE(nonAgent.Wait(::Thunder::Core::Thread::STOPPED | ::Thunder::Core::Thread::BLOCKED | ::Thunder::Core::Thread::STOPPING));

        EXPECT_EQ(sharedBuffer.Error(), 0);

        EXPECT_EQ(sharedBuffer.Agents(), 0);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder
