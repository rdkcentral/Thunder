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

#include <condition_variable>
#include <mutex>

#include "AWC.h"
#include "AWCContainerBase.h"

namespace Thunder {
namespace ProcessContainers {

class AWCContainer:
    public AWCContainerBase,
    public AWCStateChangeListener
{
private:
    friend class AWCContainerAdministrator;

    AWCContainer(const string& id, awc::AWCClient * client, AWCStateChangeNotifier * notifier);
public:
    AWCContainer(const AWCContainer&) = delete;
    ~AWCContainer() override;
    AWCContainer& operator=(const AWCContainer&) = delete;

    uint32_t Pid() const override;
    bool IsRunning() const override;
    bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
    bool Stop(const uint32_t timeout /*ms*/) override;
    void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) override;
private:
    uint32_t _pid{0};
    int _runId{-1};
    awc::awc_app_state_t _appState{awc::AWC_STATE_UNKNOWN};
    awc::AWCClient * _client;
    AWCStateChangeNotifier * _notifier;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _waitForResponse{false};
    bool WaitUntilAWCReady() const;
};

} /* ProcessContainers */
} /* Thunder */

