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
 
// ===========================================================================
// Author        Reason                                             Date
// ---------------------------------------------------------------------------
// P. Wielders   Initial creation                                   2010/01/14
//
// ===========================================================================

#ifndef __QUEUE_H
#define __QUEUE_H

#include <algorithm>
#include <queue>
#include <thread>

#include "Module.h"
#include "StateTrigger.h"
#include "Sync.h"

namespace WPEFramework {
namespace Core {
    template <typename CONTEXT>
    class QueueType {
    public:
        QueueType() = delete;
        QueueType(const QueueType<CONTEXT>&) = delete;
        QueueType& operator=(const QueueType<CONTEXT>&) = delete;

        explicit QueueType(
            const uint32_t a_HighWaterMark)
            : _queue()
            , _state(EMPTY)
            , _maxSlots(a_HighWaterMark)
            , _lastReportedThreshold(0)
        {
            // A highwatermark of 0 is bullshit.
            ASSERT(_maxSlots != 0);

            TRACE_L5("Constructor QueueType <%p>", (this));
        }
        ~QueueType()
        {
            TRACE_L5("Destructor QueueType <%p>", (this));

            // Disable the queue and flush all entries.
            Disable();
        }

        typedef enum {
            EMPTY = 0x0001,
            ENTRIES = 0x0002,
            LIMITED = 0x0004,
            DISABLED = 0x0008

        } enumQueueState;

        // -------------------------------------------------------------------
        // This queue enforces a Producer-Consumer pattern. It takes a
        // pointer on the heap. This pointer is created by the caller
        // (producer) of the Post method. It should be destructed by the
        // receiver (consumer). The consumer is the one that calls the
        // Receive method.
        // -------------------------------------------------------------------
        bool Remove(const CONTEXT& a_Entry, const char* caller = __builtin_FUNCTION())
        {
            bool l_Removed = false;

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state != DISABLED) {
                typename std::list<CONTEXT>::iterator
                    index
                    = std::find(_queue.begin(), _queue.end(), a_Entry);

                if (index != _queue.end()) {
                    // Yep, we found it, remove it
                    l_Removed = true;
                    _queue.erase(index);
                }

                // Determine the new state.
                UpdateThresholdReport("Remove", caller);
                _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (l_Removed);
        }

        bool Post(const CONTEXT& a_Entry, const char* caller = __builtin_FUNCTION())
        {
            bool Result = false;

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state != DISABLED) {
                // Yep, let's fill it
                //lint -e{534}
                _queue.push_back(a_Entry);

                // Determine the new state.
                UpdateThresholdReport("Post", caller);
                _state.SetState(IsFull() ? LIMITED : ENTRIES);

                Result = true;
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (Result);
        }

        bool Insert(const CONTEXT& a_Entry, uint32_t a_WaitTime, const char* caller = __builtin_FUNCTION())
        {
            bool l_Posted = false;
            bool l_Triggered = true;

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state != DISABLED) {
                do {
                    // And is there a slot available to us ?
                    if (_state != LIMITED) {
                        // We have posted it.
                        l_Posted = true;

                        // Yep, let's fill it
                        _queue.push_back(a_Entry);

                        // Determine the new state.
                        UpdateThresholdReport("Insert", caller);
                        _state.SetState(IsFull() ? LIMITED : ENTRIES);
                    } else {
                        // We are moving into a wait, release the lock.
                        _adminLock.Unlock();

                        // Wait till the status of the queue changes.
                        l_Triggered = _state.WaitState(DISABLED | ENTRIES | EMPTY, a_WaitTime);

                        // Seems something happend, lock the administration.
                        _adminLock.Lock();

                        // If we were reset, that is assumed to be also a timeout
                        l_Triggered = l_Triggered && (_state != DISABLED);
                    }

                } while ((l_Posted == false) && (l_Triggered != false));
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (l_Posted);
        }

        bool Extract(CONTEXT& a_Result, uint32_t a_WaitTime, const char* caller = __builtin_FUNCTION())
        {
            bool l_Received = false;
            bool l_Triggered = true;

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state != DISABLED) {
                do {
                    // And is there a slot to read ?
                    if (_state != EMPTY) {
                        l_Received = true;

                        typename std::list<CONTEXT>::iterator
                            index
                            = _queue.begin();

                        // Get the first entry from the first spot..
                        a_Result = *index;
                        _queue.erase(index);

                        // Determine the new state.
                        UpdateThresholdReport("Extract", caller);
                        _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
                    } else {
                        // We are moving into a wait, release the lock.
                        _adminLock.Unlock();

                        // Wait till the status of the queue changes.
                        l_Triggered = _state.WaitState(DISABLED | ENTRIES | LIMITED, a_WaitTime);

                        // Seems something happend, lock the administration.
                        _adminLock.Lock();

                        // If we were reset, that is assumed to be also a timeout
                        l_Triggered = l_Triggered && (_state != DISABLED);
                    }

                } while ((l_Received == false) && (l_Triggered != false));
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (l_Received);
        }

        void Enable(const char* caller = __builtin_FUNCTION())
        {
            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state == DISABLED) {
                _state.SetState(EMPTY);
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();
        }

        void Disable(const char* caller = __builtin_FUNCTION())
        {
            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            if (_state != DISABLED) {
                // Change the state
                _state.SetState(DISABLED);
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();
        }

        void Flush()
        {
            // Clear is only possible in a "DISABLED" state !!
            ASSERT(_state == DISABLED);

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            // Clear all entries !!std::this_thread::yield();
            while (_queue.empty() == false) {
                _queue.erase(_queue.begin());
            }

            // Done with the administration. Release the lock.
            _adminLock.Unlock();
        }

        void FreeSlot() const
        {
            _state.WaitState(false, DISABLED | ENTRIES | EMPTY, Core::infinite);
        }
        bool IsEmpty() const
        {
            return (_queue.empty());
        }
        bool IsFull() const
        {
            return (_queue.size() >= _maxSlots);
        }
        uint32_t Capacity() const
        {
            return _maxSlots;
        }
        uint32_t Length() const
        {
            return (static_cast<uint32_t>(_queue.size()));
        }
        // void action(const CONTEXT& element)
        template<typename ACTION>
        void Visit(ACTION&& action) const {
            _adminLock.Lock();
            for (const CONTEXT& entry : _queue) {
                action(entry);
            }
            _adminLock.Unlock();
        }
        bool HasEntry(const CONTEXT& element) const {

            // This needs to be atomic. Make sure it is.
            _adminLock.Lock();

            typename std::list<CONTEXT>::const_iterator index = _queue.cbegin();

            // Clear all entries !!
            while ((index != _queue.cend()) && (*index != element)) {
                index++;
            }

            bool found = (index != _queue.cend());

            // Done with the administration. Release the lock.
            _adminLock.Unlock();

            return (found);
        }
        void Lock() const {
            _adminLock.Lock();
        }
        void Unlock() const {
            _adminLock.Unlock();
        }

    private:
        uint8_t ResolveThresholdLevel(const uint32_t fillPercent) const
        {
            static constexpr uint8_t Thresholds[] = { 1, 5, 10, 15, 20, 25, 50, 60, 70, 80, 90, 100 };

            while ((level < (sizeof(Thresholds) / sizeof(Thresholds[0]))) && (fillPercent >= Thresholds[level])) {
                ++level;
            }

            return level;
        }
        void UpdateThresholdReport(const char* action, const char* caller)
        {
            // Only monitor queues that are large enough to be a WorkerPool queue.
            // Small queues (timers, events, etc.) are not relevant for WorkerPool monitoring.
            if (_maxSlots < 32) {
                return;
            }

            static constexpr uint8_t Thresholds[] = { 1, 5, 10, 15, 20, 25, 50, 60, 70, 80, 90, 100 };

            const uint32_t depth = static_cast<uint32_t>(_queue.size());
            const uint32_t fillPercent = (_maxSlots > 0) ? (((depth * 100U) / _maxSlots) < 100U ? (depth * 100U) / _maxSlots : 100U) : 0;
            const uint8_t currentLevel = ResolveThresholdLevel(fillPercent);

            if (currentLevel > _lastReportedThreshold) {
                const uint8_t index = static_cast<uint8_t>(currentLevel - 1);
                ::fprintf(stderr,
                    "[WorkerPool] Queue usage reached >= %u%%: depth=%u capacity=%u fill=%u%% action=%s caller=%s TID=%lu\n",
                    static_cast<unsigned>(Thresholds[index]),
                    static_cast<unsigned>(depth),
                    static_cast<unsigned>(_maxSlots),
                    static_cast<unsigned>(fillPercent),
                    action,
                    caller,
                    static_cast<unsigned long>(::pthread_self()));
            }

            _lastReportedThreshold = currentLevel;
        }

    private:
        std::list<CONTEXT> _queue;
        StateTrigger<enumQueueState> _state;
        mutable CriticalSection _adminLock;
        const uint32_t _maxSlots;
        uint8_t _lastReportedThreshold;
    };
}
} // namespace Core

#endif // __QUEUE_H
