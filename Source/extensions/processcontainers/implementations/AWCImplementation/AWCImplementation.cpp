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

#include "AWCContainerAdministrator.h"
#include "AWCImplementation.h"
#include "processcontainers/Tracing.h"


using namespace Thunder::ProcessContainers;

AWCContainer::AWCContainer(const string& id, awc::AWCClient * client, AWCStateChangeNotifier * notifier):
    AWCContainerBase(id)
    , _client(client)
    , _notifier(notifier)
{
    TRACE_L1("name=%s client=%p", Id().c_str(), _client);
    _notifier->addListener(this);
}

AWCContainer::~AWCContainer()
{
    TRACE_L1("id=%s _pid=%d _client=%p", Id().c_str(), _pid, _client);
    if (IsRunning()) {
        Stop(2000);
    }
    _notifier->removeListener(this);
    TRACE(ProcessContainerization, (_T("Container [%s] released"), Id().c_str()));

    static_cast<AWCContainerAdministrator &>(AWCContainerAdministrator::Instance()).RemoveContainer(this);
}

uint32_t AWCContainer::Pid() const
{
    TRACE_L1("id=%s _pid=%d _appState=%d", Id().c_str(), _pid, _appState);
    std::lock_guard<std::mutex> lock(_mutex);
    return _appState == awc::AWC_STATE_STARTED ? _pid : 0;
}

bool AWCContainer::IsRunning() const
{
    TRACE_L1("id=%s", Id().c_str());
    std::lock_guard<std::mutex> lock(_mutex);
    bool isRunning = _runId > 0 && _pid > 0 && _appState == awc::AWC_STATE_STARTED;
    TRACE_L1("isRunning=%d", isRunning);
    return isRunning;
}

bool AWCContainer::WaitUntilAWCReady() const
{
    TRACE_L1("AWC ready?");
    const int maxAttempts = 60; // for a total of max 30 secs
    int attemptCount = 0;
    bool result = false, is_ready = false;
    awc::AWCClient::awc_status_t awc_result;
    do {
        {
          std::lock_guard<std::mutex> lock(_mutex);
          is_ready = false;
          awc_result = _client->isServiceReady(is_ready);
          result = (awc_result == awc::AWCClient::AWC_STATUS_OK);
        }
        attemptCount++;

        if ((!result || !is_ready) && attemptCount < maxAttempts) {
            TRACE_L1("AWC is not ready yet (%d,%d,%d), give it some more time...", awc_result, result, is_ready);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } else {
            break;
	}
    } while (true);

    if (!result || !is_ready) {
      TRACE_L1("Giving up on waiting on awc to be ready. It is not. (%d,%d)");
    } else {
      TRACE_L1("Yes, AWC is ready!");
    }
    return is_ready;
}

bool AWCContainer::Start(const string& command, IStringIterator& parameters)
{
    if(IsRunning())
    {
        TRACE_L1("id=%s(%p) already running", Id().c_str(), this);
        return false;
    }

    WaitUntilAWCReady();

    awc::AWCClient::str_vect_sptr_t windowParams = std::make_shared<std::vector<std::string>>();
    awc::AWCClient::str_vect_sptr_t appParams = std::make_shared<std::vector<std::string>>();

    std::string cmd;
    // real - from framework command line paramaters
    while(parameters.Next())
    {
        if(!cmd.empty()) cmd += ' ';
        cmd += parameters.Current();
        appParams->push_back(parameters.Current());
    }

    TRACE_L1(
        "id=%s command=%s numOfParams=%d params=%s",
        Id().c_str(), command.c_str(), parameters.Count(), cmd.c_str());

    int run_id = -1;
    bool result = false;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _pid = 0;
        _runId = -1;
        _appState = awc::AWC_STATE_UNKNOWN;
        TRACE_L1("starting appid=%s", Id().c_str());
        awc::AWCClient::awc_status_t awc_result = _client->start(Id(),
                                                                appParams,
                                                                windowParams, run_id);
        TRACE_L1("starting result result=%d run_id=%d", awc_result, run_id);
        result = (awc_result == awc::AWCClient::AWC_STATUS_OK);
        while (result && _pid == 0 && _appState != awc::AWC_STATE_STARTED)
        {
            _runId = run_id; _waitForResponse = true;
            TRACE_L1("waiting for start Id()=%s", Id().c_str());
            _cv.wait_for(lock, std::chrono::seconds(30), [this] { return !_waitForResponse; });
            _waitForResponse = false;
            if (_appState != awc::AWC_STATE_STARTED && _appState != awc::AWC_STATE_STARTING)
            {
                TRACE_L1("unexpected notification arrived _pid=%d _appState=%d", _pid, _appState);
                result = false;
            }
        }
    }
    TRACE_L1("_pid=%d _runId=%d result=%d", _pid, _runId, result);
    return result;
}

bool AWCContainer::Stop(const uint32_t timeout /*ms*/)
{
    if(!IsRunning())
    {
        TRACE_L1("id=%s(%p) not running", Id().c_str(), this);
        return true;
    }

    TRACE_L1("id=%s timeout=%u _pid=%u _runId=%d", Id().c_str(), timeout, _pid, _runId);
    std::unique_lock<std::mutex> lock(_mutex);
    awc::AWCClient::awc_status_t awc_result = _client->stop(_pid, 0); // documentation: exit type is important only for Netflix
    bool result = (awc_result == awc::AWCClient::AWC_STATUS_OK);
    TRACE_L1("awc_result=%d result=%d", awc_result, result);
    while (result && _appState != awc::AWC_STATE_STOPPED)
    {
        _waitForResponse = true;
        TRACE_L1("waiting for stop id=%s timeout=%u[ms]", Id().c_str(), timeout);
        const auto status = _cv.wait_for(lock, std::chrono::seconds(30), [this] { return !_waitForResponse; });
        if (_waitForResponse && !status && _appState != awc::AWC_STATE_STOPPED)
        {
            TRACE_L1("timeout, stop notification not arrived _pid=%d _appState=%d", _pid, _appState);
            result = false;
        }
        _waitForResponse = false;
    }
    TRACE_L1("result=%d", result);
    return result;
}

void AWCContainer::notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_runId >= 0 && req_id == _runId)
    {
        TRACE_L1("id=%s _runId=%d req_id=%d app_state=%d status=%d pid=%d", Id().c_str(), _runId, req_id, app_state, status, pid);
        if (_appState != awc::AWC_STATE_STOPPED)
        {
            _pid = pid;
            _appState = app_state;
        }
        else
        {
            TRACE_L1("container was alredy stopped");
        }
        if (_waitForResponse)
        {
            _waitForResponse = false;
            _cv.notify_one();
        }
    }
}
