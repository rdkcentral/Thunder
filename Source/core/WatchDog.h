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

namespace WPEFramework {
namespace Core {

    struct EXTERNAL IWatchable {
        virtual ~IWatchable() = default;

        virtual const TCHAR* Name() const = 0;
        virtual uint32_t Timeout() const = 0;
        virtual bool Expired(const uint64_t now) = 0;
        virtual string Report() const
        {
            return (string());
        }
    };

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

            explicit Monitor(WatchDog& parent);
            ~Monitor() override;

        private:
            uint32_t Worker() override;

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
        static uint64_t Ticks();

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