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

#include "Module.h"
#include "StateTrigger.h"
#include "Sync.h"

namespace Thunder {
    namespace Core {
        template <typename CONTEXT>
        class QueueType {
        public:
            QueueType() = delete;
            QueueType(QueueType<CONTEXT>&&) = delete;
            QueueType(const QueueType<CONTEXT>&) = delete;
            QueueType& operator=(QueueType<CONTEXT>&&) = delete;
            QueueType& operator=(const QueueType<CONTEXT>&) = delete;

            explicit QueueType(
                const uint32_t a_HighWaterMark)
                : _queue()
                , _state(EMPTY)
                , _maxSlots(a_HighWaterMark)
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

            bool Remove(const CONTEXT& a_Entry)
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
                    _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (l_Removed);
            }

            bool Post(const CONTEXT& a_Entry)
            {
                bool Result = false;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state != DISABLED) {
                    // Yep, let's fill it
                    //lint -e{534}
                    _queue.push_back(a_Entry);

                    // Determine the new state.
                    _state.SetState(IsFull() ? LIMITED : ENTRIES);

                    Result = true;
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (Result);
            }

            bool Insert(const CONTEXT& a_Entry, uint32_t a_WaitTime)
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
                            _state.SetState(IsFull() ? LIMITED : ENTRIES);
                        }
                        else {
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

            bool Extract(CONTEXT& a_Result, uint32_t a_WaitTime)
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
                            _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
                        }
                        else {
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

            void Enable()
            {
                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state == DISABLED) {
                    _state.SetState(EMPTY);
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();
            }

            void Disable()
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

                // Clear all entries !!
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
            uint32_t Length() const
            {
                return (static_cast<uint32_t>(_queue.size()));
            }
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
            std::list<CONTEXT> _queue;
            StateTrigger<enumQueueState> _state;
            mutable CriticalSection _adminLock;
            const uint32_t _maxSlots;
        };

        template<typename CONTEXT, const bool DYNAMIC_THRESHOLD>
        class CategoryQueueType {
        public:
            typedef enum : uint8_t {
                HIGH = 0,
                MEDIUM = 1,
                LOW = 2
            } category;

        private:
            typedef enum {
                EMPTY = 0x0001,
                ENTRIES = 0x0002,
                LIMITED = 0x0004,
                DISABLED = 0x0008

            } state;

            // All actions in the CategoryAdmin class must be protected by the parent class.
            // No locking takes place here as it is assumed that the parent call takes care of 
            // the locking.
            template<typename ELEMENT>
            class CategoryAdminType {
            private:
                using Entries = std::deque<CONTEXT>;

            public:
                CategoryAdminType(const CategoryAdminType<ELEMENT>&) = delete;
                CategoryAdminType<ELEMENT>& operator=(CategoryAdminType<ELEMENT>&&) = delete;
                CategoryAdminType<ELEMENT>& operator=(const CategoryAdminType<ELEMENT>&) = delete;

                CategoryAdminType() = delete;

                CategoryAdminType(CategoryAdminType<ELEMENT>&& move) noexcept
                    : _type(std::move(move._type))
                    , _highWaterMark(std::move(move._highWaterMark))
                    , _submitted(std::move(move._submitted))
                    , _queue(std::move(move._queue))
                    , _observerLock()
                    , _observers(move._observers.load())
                    , _observerSignal(false, true) {
                }
                CategoryAdminType(const category type, const uint16_t highWaterMark)
                    : _type(type)
                    , _highWaterMark(highWaterMark)
                    , _submitted(0)
                    , _queue()
                    , _observerLock()
                    , _observers(0)
                    , _observerSignal(false, true) {
                    // A highwatermark of 0 is bullshit.
                    ASSERT(_highWaterMark != 0);
                }
                ~CategoryAdminType() = default;

            public:
                inline category Category() const {
                    return (_type);
                }
                inline void Flush() {
                    _queue.clear();
                    _submitted = 0;

                    // We changed the queue, let waiters now..
                    Signal();
                }
                inline uint16_t Length() const
                {
                    return (static_cast<uint16_t>(_queue.size()));
				}
                inline uint16_t HighWaterMark() const
                {
                    return (_highWaterMark);
				}
                inline bool HasEntry() const
                {
                    return ((_queue.empty() == false) && (_submitted < _highWaterMark));
                }
                inline bool HasEntry(const CONTEXT& entry) const
                {
                    return (std::find(_queue.begin(), _queue.end(), entry) != _queue.end());
                }
                inline bool Remove(const CONTEXT& entry) 
                {
                    bool removed = false;

                    typename Entries::iterator index (std::find(_queue.begin(), _queue.end(), entry));

                    if (index != _queue.end()) {
                        // Yep, we found it, remove it
                        removed = true;
                        _queue.erase(index);

                        // We changed the queue, let waiters now..
                        Signal();
                    }

                    return (removed);
                }
                inline bool Submit(const CONTEXT& entry) {
                    bool submit(true);

                    ASSERT(_submitted <= _highWaterMark);

                    if (_submitted == _highWaterMark) {
                        // We can submit it.
                        _queue.push_back(entry);
                    }
                    else {
                        submit = false;
                        _submitted += 1;
                    }

                    ASSERT(_submitted <= _highWaterMark);

                    return (submit);
                }
                inline bool Submit(const CONTEXT& entry, const uint16_t currentLoad) {
                    bool submit(true);

                    ASSERT(_submitted <= _highWaterMark);

                    if ( (_submitted == _highWaterMark) || (currentLoad >= _highWaterMark) ) {
                        // We can submit it.
                        _queue.push_back(entry);
                    }
                    else {
                        submit = false;
                        _submitted += 1;
                    }

                    ASSERT(_submitted <= _highWaterMark);

                    return (submit);
                }
                inline CONTEXT Extract() {
                    ASSERT(_queue.empty() == false);
                    // Get the first entry from the first spot..
                    CONTEXT result = _queue.front();
                    _queue.pop_front();
                    // Increase the submitted count.
                    _submitted++;
                    ASSERT(_submitted <= _highWaterMark);

                    // We changed the queue, let waiters now..
                    Signal();

                    // Signal potetial waiting threads that we removed
                    // an entry, so they can evaluate..
                    return (result);
                }
                inline void Completed() {
                    _submitted--;
                    ASSERT(_submitted <= _highWaterMark);
                }
                bool AwaitQueuing(const CONTEXT& entry, const uint32_t waitTime) const
                {
                    uint32_t result = Core::ERROR_NONE;;

                    _observerLock.Lock();

                    while ((result == Core::ERROR_NONE) && (HasEntry(entry) == true)) {
                        ++_observers;

                        _observerLock.Unlock();

                        result = _observerSignal.Lock(waitTime);

                        --_observers;
                        _observerLock.Lock();
                    }

                    _observerLock.Unlock();

                    // This needs to be atomic. Make sure it is.
                    return (result == Core::ERROR_NONE);
                }
                template<typename ACTION>
                void Visit(ACTION&& action) const {
                    for (const CONTEXT& entry : _queue) {
                        action(entry);
                    }
                }

            private:
                void Signal() {
                    _observerLock.Lock();

                    if (_observers.load(Core::memory_order::memory_order_relaxed) > 0) {
                        _observerSignal.SetEvent();

						do {
                            std::this_thread::yield();
                        } while (_observers.load(Core::memory_order::memory_order_relaxed) != 0);
						
                        _observerSignal.ResetEvent();
                    }
                    _observerLock.Unlock();
                }

            public:
                const category _type;
                uint16_t _highWaterMark;
                uint16_t _submitted;
                Entries _queue;
                mutable Core::CriticalSection _observerLock;
                mutable std::atomic<uint32_t> _observers;
                mutable Core::Event _observerSignal;
            };

            using Entry = std::pair<category, CONTEXT>;
            using Entries = std::deque<Entry>;
            using Categories = std::vector<CategoryAdminType<CONTEXT>>;

        public:
            CategoryQueueType(CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>&&) = delete;
            CategoryQueueType(const CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>&) = delete;
            CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>& operator=(CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>&&) = delete;
            CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>& operator=(const CategoryQueueType<CONTEXT, DYNAMIC_THRESHOLD>&) = delete;

            CategoryQueueType(
                const uint16_t lowMaxSubmit,
                const uint16_t mediumMaxSubmit,
                const uint16_t totalSubmit)
                : _adminLock()
                , _state(state::EMPTY)
                , _highWaterMark(totalSubmit)
                , _categories()
                , _queue()
                , _inProcess() {
                // A highwatermark of 0 is bullshit.
                ASSERT(_highWaterMark != 0);
                TRACE_L5("Constructor CategoryQueueType <%p>", (this));
				_categories.reserve(3);
                _categories.emplace_back(CategoryAdminType<CONTEXT>(category::HIGH, totalSubmit));       // HIGH
                _categories.emplace_back(CategoryAdminType<CONTEXT>(category::MEDIUM, mediumMaxSubmit)); // MEDIUM
                _categories.emplace_back(CategoryAdminType<CONTEXT>(category::LOW, lowMaxSubmit));       // LOW
            }
            ~CategoryQueueType()
            {
                TRACE_L5("Destructor CategoryQueueType <%p>", (this));
            }

        public:
            bool Remove(const CONTEXT& entry)
            {
                bool removed = false;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state != DISABLED) {
                    typename Entries::iterator index(_queue.begin());

                    while ((index != _queue.end()) && (index->second != entry)) {
                        ++index;
                    }

                    if (index != _queue.end()) {
                        // Yep, we found it, remove it
                        removed = true;
                        _queue.erase(index);

                        // Determine the new state.
                        _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
                    }
                    else {
                        typename Categories::iterator categoryIndex = _categories.begin();
                        while ((categoryIndex != _categories.end()) && (categoryIndex->Remove(entry) == false)) {
                            categoryIndex++;
                        }
                        removed = categoryIndex != _categories.end();
                    }
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (removed);
            }
            bool Post(const CONTEXT& entry, const category type = category::LOW)
            {
                bool result = false;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state != DISABLED) {
                    // Yep, let's fill it
                    //lint -e{534}
                    if (_categories[type].Submit(entry) == false) {
                        // We can post it.
                        _queue.push_back(std::pair<category, CONTEXT>(type, entry));
                    }

                    // Determine the new state.
                    _state.SetState(IsFull() ? LIMITED : ENTRIES);

                    result = true;
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (result);
            }
            bool Insert(const CONTEXT& entry, const uint32_t waitTime, const category type = category::LOW)
            {
                bool posted = false;
                bool triggered = true;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state != DISABLED) {
                    do {
                        // And is there a slot available to us ?
                        if ((_state != LIMITED) && (Submit(entry, type) == false)) {
                            // We have posted it.
                            posted = true;

                            // We can post it.
                            _queue.push_back(std::pair<category,CONTEXT>(type, entry));

                            // Determine the new state.
                            _state.SetState(IsFull() ? LIMITED : ENTRIES);
                        }
                        else if (_state != LIMITED) {
                            _adminLock.Unlock();
                            // It's a priority wait. We need to block ourself till we 
                            // are actually scheduled for a post.
                            posted = _categories[type].AwaitQueuing(entry, waitTime);
                            triggered = false;

                            _adminLock.Lock();
                        }
                        else {
                            // We are moving into a wait, release the lock.
                            _adminLock.Unlock();

                            // Wait till the status of the queue changes.
                            triggered = _state.WaitState(DISABLED | ENTRIES | EMPTY, waitTime);

                            // Seems something happend, lock the administration.
                            _adminLock.Lock();

                            // If we were reset, that is assumed to be also a timeout
                            triggered = triggered && (_state != DISABLED);
                        }

                    } while ((posted == false) && (triggered != false));
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (posted);
            }

            bool Extract(CONTEXT& result, uint32_t waitTime)
            {
                bool received = false;
                bool triggered = true;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state != DISABLED) {
                    do {
                        // And is there a slot to read ?
                        if (_queue.empty() == false) {
                            received = true;

                            // Get the first entry from the first spot..
                            result = _queue.front().second;

                            // Move to entry to InProcess...
                            _inProcess.push_back(_queue.front());
                            _queue.pop_front();

                            // Update the state to reflect the queue content (EMPTY has priority).
                            _state.SetState(IsEmpty() ? EMPTY : (IsFull() ? LIMITED : ENTRIES));
                        }
                        else {
                            // We are moving into a wait, release the lock.
                            _adminLock.Unlock();

                            // Wait till the status of the queue changes.
                            triggered = _state.WaitState(DISABLED | ENTRIES | LIMITED, waitTime);

                            // Seems something happend, lock the administration.
                            _adminLock.Lock();

                            // If we were reset, that is assumed to be also a timeout
                            triggered = triggered && (_state != DISABLED);
                        }

                    } while ((received == false) && (triggered != false));
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (received);
            }

            void Completion(const CONTEXT& request) {

                _adminLock.Lock();

                // Find the entry that is completed to report it done..
                typename Entries::iterator index(_inProcess.begin());

                while ((index != _inProcess.end()) && (index->second != request)) {
                    index++;
                }

                ASSERT(index != _inProcess.end());

                if (index != _inProcess.end()) {
                    _categories[index->first].Completed();

                    // Yep, we found it, remove it
                    _inProcess.erase(index);

                    typename Categories::iterator categoryIndex = _categories.begin();

                    if (_state != DISABLED) {
                        // Submit (if possible) a new one from high Prio to low..
                        while ((categoryIndex != _categories.end()) && (categoryIndex->HasEntry() == false)) {
                            categoryIndex++;
                        }

                        if (categoryIndex == _categories.end()) {
                            // Determine the new state.
                            _state.SetState(IsEmpty() ? EMPTY : ENTRIES);
                        }
                        else {
                            // We have a new entry to submit, so we can post it.
                            _queue.emplace_back(std::make_pair(categoryIndex->Category(), categoryIndex->Extract()));
                            // Determine the new state.
                            _state.SetState(IsFull() ? LIMITED : ENTRIES);
                        }
                    }
                }

                _adminLock.Unlock();
            }

            inline void Enable()
            {
                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                if (_state == DISABLED) {
                    _state.SetState(EMPTY);
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();
            }
            inline void Disable()
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
            inline void Flush()
            {
                // Clear is only possible in a "DISABLED" state !!
                ASSERT(_state == DISABLED);

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                // Clear all entries !!
                _queue.clear();

                for (auto& category : _categories) {
                    category.Flush();
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();
            }

            //inline void FreeSlot() const
			//{
			//	_state.WaitState(false, DISABLED | ENTRIES | EMPTY, Core::infinite);
			//}
            inline bool IsEmpty() const
            {
                return (_queue.empty());
            }
            inline bool IsFull() const
            {
                return ((_queue.size() + _inProcess.size()) >= _highWaterMark);
            }
            inline uint32_t Length() const
            {
                uint32_t length = 0;
                for (const auto& category : _categories) {
                    length += static_cast<uint32_t>(category.Length());
                }
                return (static_cast<uint32_t>(_queue.size() + length));
            }
            inline bool HasEntry(const CONTEXT& entry) const {
                bool found = false;

                // This needs to be atomic. Make sure it is.
                _adminLock.Lock();

                typename Entries::const_iterator index(_queue.begin());
                
                while ((index != _queue.end()) && (index->second != entry)) {
                    ++index;
                }

                if (index != _queue.end()) {
                    found = true;
                }
                else {
                    index = _inProcess.begin();

                    while ((index != _inProcess.end()) && (index->second != entry)) {
                        ++index;
                    }

                    if (index != _inProcess.end()) {
                        // Yep, we found it, remove it
                        found = true;
                    }
                    else {
                        typename Categories::const_iterator categoryIndex(_categories.begin());
                        while ((categoryIndex != _categories.end()) && (categoryIndex->HasEntry(entry) == false)) {
                            categoryIndex++;
                        }
                        found = categoryIndex != _categories.end();
                    }
                }

                // Done with the administration. Release the lock.
                _adminLock.Unlock();

                return (found);
            }
            template<typename ACTION>
            void Visit(ACTION&& action) const {
                _adminLock.Lock();
                for (const Entry& entry : _queue) {
                    action(entry.second);
                }
                for (const auto& entry : _categories) {
                    entry.Visit(action);
				}
                _adminLock.Unlock();
            }
            inline void Lock() const {
                _adminLock.Lock();
            }
            inline void Unlock() const {
                _adminLock.Unlock();
            }

        private:
            inline bool Submit(const CONTEXT& entry, const category type) {
                if (DYNAMIC_THRESHOLD == false) {
                    return (_categories[type].Submit(entry));
                }
				return (_categories[type].Submit(entry, static_cast<uint16_t>(_queue.size())));
            }

        private:
            mutable Core::CriticalSection _adminLock;
            StateTrigger<state> _state;
            uint16_t _highWaterMark;
            Categories _categories;
            Entries _queue;
            Entries _inProcess;
        };

    }
} // namespace Core


#endif // __QUEUE_H
