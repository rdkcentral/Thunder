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

#include "WatchDog.h"

#include "Trace.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>

namespace Thunder {

namespace Core {

    namespace {
        constexpr uint32_t MaxInterval = 250;
    }

    WatchDog::WatchDog()
        : _adminLock()
        , _entries()
        , _monitor(nullptr)
    {
    }

    WatchDog::~WatchDog()
    {
        // Entries are expected to be gone, but never leave the thread behind that evaluates them.
        delete _monitor;
    }

    /* static */ WatchDog& WatchDog::Instance()
    {
        return (SingletonType<WatchDog>::Instance());
    }

    /* static */ uint64_t WatchDog::Ticks()
    {
        return (static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()));
    }

    void WatchDog::Register(IWatchable& watchable, const bool abortOnExpiry)
    {
        _adminLock.Lock();

        if (std::find_if(_entries.begin(), _entries.end(), [&](const Entry& entry) { return (entry.Watchable == &watchable); }) == _entries.end()) {
            _entries.push_back(Entry{ &watchable, abortOnExpiry, false });
        }

        if (_monitor == nullptr) {
            _monitor = new Monitor(*this);
        }

        Monitor* monitor = _monitor;

        _adminLock.Unlock();

        monitor->Run();
    }

    void WatchDog::Unregister(IWatchable& watchable)
    {
        _adminLock.Lock();

        _entries.erase(std::remove_if(_entries.begin(), _entries.end(), [&](const Entry& entry) { return (entry.Watchable == &watchable); }), _entries.end());

        _adminLock.Unlock();
    }

    uint32_t WatchDog::Evaluate()
    {
        uint32_t interval = Core::infinite;
        const uint64_t now = Ticks();

        _adminLock.Lock();

        for (Entry& entry : _entries) {
            const uint32_t timeout = entry.Watchable->Timeout();

            if (entry.Watchable->Expired(now) == false) {
                entry.Reported = false;
            } else if (entry.Reported == false) {
                entry.Reported = true;

                const string details(entry.Watchable->Report());
                CC_SYSLOG("Watchdog [%s] expired, no response for %u ms%s%s",
                    entry.Watchable->Name(),
                    timeout,
                    (details.empty() == true ? "" : ": "),
                    details.c_str());

                if (entry.AbortOnExpiry == true) {
                    CC_SYSLOG("Aborting process due to watchdog expiry");
                    std::abort();
                }
            }

            interval = std::min(interval, std::max<uint32_t>(1, std::min<uint32_t>(timeout / 4, MaxInterval)));
        }

        _adminLock.Unlock();

        return (interval);
    }
}
}
