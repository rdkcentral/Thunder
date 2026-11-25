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

#include "Module.h"
#include "Singleton.h"
#include "Sync.h"
#include "Thread.h"

#include <vector>

namespace Thunder {

namespace Core {

    // Something the WatchDog can supervise. All methods are called from the WatchDog thread, they
    // must neither block nor take locks that the supervised subject can hold while it is stalled.
    struct EXTERNAL IWatchable {
        virtual ~IWatchable() = default;

        virtual const TCHAR* Name() const = 0;

        // Timeout in milliseconds, it determines how often the WatchDog needs to evaluate this subject.
        virtual uint32_t Timeout() const = 0;

        // Evaluate the subject, now is a monotonic timestamp in microseconds (see WatchDog::Ticks()).
        virtual bool Expired(const uint64_t now) = 0;

        // Additional information to log when the subject expired.
        virtual string Report() const
        {
            return (string());
        }
    };

    // The one and only thread supervising all registered IWatchable subjects. The thread is only created once the
    // first subject is registered.
    class EXTERNAL WatchDog {
    private:
        friend class SingletonType<WatchDog>;

        struct Entry {
            IWatchable* Watchable;
            bool AbortOnExpiry;
            bool Reported;
        };

        class Monitor : public Thread {
        public:
            Monitor(Monitor&&) = delete;
            Monitor(const Monitor&) = delete;
            Monitor& operator=(Monitor&&) = delete;
            Monitor& operator=(const Monitor&) = delete;

            explicit Monitor(WatchDog& parent)
                : Thread(Thread::DefaultStackSize(), _T("Core::WatchDog"))
                , _parent(parent)
            {
                Thread::Init();
            }
            ~Monitor() override
            {
                Terminate();
            }

        private:
            uint32_t Worker() override
            {
                // Block first, a Register() arriving while we evaluate makes us run again.
                Block();
                return (_parent.Evaluate());
            }

        private:
            WatchDog& _parent;
        };

        WatchDog();

    public:
        WatchDog(WatchDog&&) = delete;
        WatchDog(const WatchDog&) = delete;
        WatchDog& operator=(WatchDog&&) = delete;
        WatchDog& operator=(const WatchDog&) = delete;

        ~WatchDog();

        static WatchDog& Instance();

        // Monotonic clock in microseconds, not affected by wall clock corrections.
        static uint64_t Ticks();

        // The watchable must stay valid until Unregister() returned.
        void Register(IWatchable& watchable, const bool abortOnExpiry);
        void Unregister(IWatchable& watchable);

    private:
        uint32_t Evaluate();

    private:
        CriticalSection _adminLock;
        std::vector<Entry> _entries;
        Monitor* _monitor;
    };
}
}
