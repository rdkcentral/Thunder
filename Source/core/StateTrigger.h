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
 
// StateTrigger.h: interface for the StateTrigger class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __STATETRIGGER_H
#define __STATETRIGGER_H

#include "Module.h"
#include "Trace.h"

#ifdef __UNIX__
#include <unistd.h>
#endif

#include "Sync.h"

namespace Thunder {
namespace Core {
    template <class enumerate>
    class StateTrigger {
    public:
        StateTrigger() = delete;
        StateTrigger(StateTrigger&& move) = delete;
        StateTrigger(const StateTrigger& a_Copy) = delete;
        StateTrigger& operator=(StateTrigger&& move) = delete;
        StateTrigger& operator=(const StateTrigger& a_RHS) = delete;

    public:
        explicit StateTrigger(const enumerate a_State);
        virtual ~StateTrigger();

        enumerate GetState() const;

        // Time in milliseconds!
        bool WaitState(const unsigned int a_State, unsigned int a_Time) const;

        // Time in milliseconds!
        bool WaitStateClear(const unsigned int a_State, unsigned int a_Time) const;
        enumerate SetState(enumerate a_State);
        StateTrigger& operator=(enumerate a_State);
        bool operator==(enumerate a_State) const;
        bool operator!=(enumerate a_State) const;
        operator enumerate() const;

        void Lock() const
        {
            m_AdminLock.Lock();
        }
        void Unlock() const
        {
            m_AdminLock.Unlock();
        }

    private:
        // The state will be represented by a number.
        mutable CriticalSection m_AdminLock;
        mutable Event m_StateChange;
        volatile mutable uint32_t m_InterestCount;
        volatile enumerate m_State;
    };

    template <class enumerate>
    StateTrigger<enumerate>::StateTrigger(const enumerate a_State)
        : m_AdminLock()
        , m_StateChange(false, true)
        , m_InterestCount(0)
        , m_State(a_State)
    {
        TRACE_L5("Constructor StateTrigger <%p>", (this));
    }

    template <class enumerate>
    StateTrigger<enumerate>::~StateTrigger()
    {
        TRACE_L5("Destructor StateTrigger <%p>", (this));
    }

    template <class enumerate>
    enumerate
    StateTrigger<enumerate>::GetState() const
    {
        return (m_State);
    }

    template <class enumerate>
    bool
    StateTrigger<enumerate>::WaitState(
        const unsigned int a_State,
        unsigned int a_Time) const
    {
        bool blTimeOut = false;

        m_AdminLock.Lock();

        // Now wait till our state is reached.
        while (((a_State & static_cast<unsigned int>(m_State)) == 0) && (blTimeOut == false)) {
            InterlockedIncrement(m_InterestCount);

            m_AdminLock.Unlock();

            blTimeOut = (m_StateChange.Lock(a_Time) != 0);

            InterlockedDecrement(m_InterestCount);

            m_AdminLock.Lock();
        }

        m_AdminLock.Unlock();

        return (!blTimeOut);
    }
    template <class enumerate>

    bool
    StateTrigger<enumerate>::WaitStateClear(
        const unsigned int a_State,
        unsigned int a_Time) const
    {
        bool blTimeOut = false;

        m_AdminLock.Lock();

        // Now wait till our state is reached.
        while (((a_State & static_cast<unsigned int>(m_State)) != 0) && (blTimeOut == false)) {
            InterlockedIncrement(m_InterestCount);

            m_AdminLock.Unlock();

            blTimeOut = (m_StateChange.Lock(a_Time) != 0);

            InterlockedDecrement(m_InterestCount);

            m_AdminLock.Lock();
        }

        m_AdminLock.Unlock();

        return (!blTimeOut);
    }

    template <class enumerate>
    enumerate
    StateTrigger<enumerate>::SetState(
        enumerate a_State)
    {
        m_AdminLock.Lock();

        if (a_State != m_State) {
            m_State = a_State;

            // Unlock report it if someone is waiting.
            if (m_InterestCount > 0) {
                m_StateChange.SetEvent();

                // Wait till the interest count reaches 0 again..
                while (m_InterestCount) {
                    // Give the slice to some else
                    std::this_thread::yield();
                }

                m_StateChange.ResetEvent();
            }
        }

        m_AdminLock.Unlock();

        return (m_State);
    }

    template <class enumerate>
    StateTrigger<enumerate>&
    StateTrigger<enumerate>::
    operator=(enumerate a_State)
    {
        SetState(a_State);

        return (*this);
    }

    template <class enumerate>
    bool
    StateTrigger<enumerate>::
    operator==(enumerate a_State) const
    {
        return (a_State == m_State);
    }

    template <class enumerate>
    bool
    StateTrigger<enumerate>::
    operator!=(enumerate a_State) const
    {
        return (a_State != m_State);
    }

    template <class enumerate>
    StateTrigger<enumerate>::operator enumerate() const
    {
        return (m_State);
    }
}
} // namespace Core

#endif // __STATETRIGGER_H
