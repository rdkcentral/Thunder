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

#include "AWCImplementation.h"
#include "processcontainers/common/CGroupContainerInfo.h"

#define _TRACE_FUNCTION_ __PRETTY_FUNCTION__

namespace WPEFramework {
namespace ProcessContainers {

    // stubs only implementation
    struct AWCMemoryInfo : public BaseRefCount<IMemoryInfo> {
        AWCMemoryInfo()
        {
        }

        uint64_t Allocated() const
        {
            return 0;
        }

        uint64_t Resident() const
        {
            return 0;
        }

        uint64_t Shared() const
        {
            return 0;
        }
    };

    // stubs only implementation
    struct AWCProcessorInfo : public BaseRefCount<IProcessorInfo> {
        AWCProcessorInfo()
        {
        }

        uint64_t TotalUsage() const
        {
            return 0;
        }

        uint64_t CoreUsage(uint32_t coreNum) const
        {
            return 0;
        }

        uint16_t NumberOfCores() const
        {
            return 0;
        }
    };

    AWCContainer::AWCContainer(const string& name, awc::AWCClient * client, AWCStateChangeNotifier * notifier, const string& containerLogDir, const string& configuration)
        : _name(name)
        , _pid(0)
        , _runId(-1)
        , _appState(awc::AWC_STATE_UNKNOWN)
        , _referenceCount(1)
        , _client(client)
        , _notifier(notifier)
        , _mutex()
        , _cv()
        , _waitForResponse(false)
    {
        TRACE_L3("%s name=%s client=%p conntainerLogDir=%s configuration=%s", _TRACE_FUNCTION_, _name.c_str(), _client, containerLogDir.c_str(), configuration.c_str());
        _notifier->addListener(this);
    }

    AWCContainer::~AWCContainer()
    {
        TRACE_L3("%s _name=%s _pid=%d _client=%p", _TRACE_FUNCTION_, _name.c_str(), _pid, _client);
        if (IsRunning()) {
            Stop(2000);
        }
        _notifier->removeListener(this);
        TRACE(ProcessContainers::ProcessContainerization, (_T("Container [%s] released"), _name.c_str()));

        static_cast<AWCContainerAdministrator&>(AWCContainerAdministrator::Instance()).RemoveContainer(this);
    }

    const string& AWCContainer::Id() const
    {
        TRACE_L3("%s id=%s", _TRACE_FUNCTION_, _name.c_str());
        return _name;
    }

    uint32_t AWCContainer::Pid() const
    {
        TRACE_L3("%s _name=%s _pid=%d _appState=%d", _TRACE_FUNCTION_, _name.c_str(), _pid, _appState);
        std::lock_guard<std::mutex> lock(_mutex);
        return _appState == awc::AWC_STATE_STARTED ? _pid : 0;
    }

    IMemoryInfo* AWCContainer::Memory() const
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        return new AWCMemoryInfo();
    }

    IProcessorInfo* AWCContainer::ProcessorInfo() const
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        return new AWCProcessorInfo();
    }

    INetworkInterfaceIterator* AWCContainer::NetworkInterfaces() const
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        return nullptr;
    }

    bool AWCContainer::IsRunning() const
    {
        TRACE_L3("%s _name=%s", _TRACE_FUNCTION_, _name.c_str());
        std::lock_guard<std::mutex> lock(_mutex);
        bool isRunning = _runId > 0 && _pid > 0 && _appState == awc::AWC_STATE_STARTED;
        TRACE_L1("%s isRunning=%d", _TRACE_FUNCTION_, isRunning);
        return isRunning;
    }

    bool AWCContainer::Start(const string& command, IStringIterator& parameters)
    {
        TRACE_L1("%s _name=%s command=%s numOfParams=%d", _TRACE_FUNCTION_, _name.c_str(), command.c_str(), parameters.Count());
        int paramIdx = 0;
        awc::AWCClient::str_vect_sptr_t windowParams = std::make_shared<std::vector<std::string>>();
        awc::AWCClient::str_vect_sptr_t appParams = std::make_shared<std::vector<std::string>>();

        // real - from framework command line paramaters
        while(parameters.Next())
        {
            TRACE_L3("%s param[%d]=%s", _TRACE_FUNCTION_, paramIdx++, parameters.Current().c_str());
            appParams->push_back(parameters.Current());
        }

        int run_id = -1;
        bool result = false;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _pid = 0;
            _runId = -1;
            _appState = awc::AWC_STATE_UNKNOWN;
            TRACE_L1("%s starting app_name=%s", _TRACE_FUNCTION_, _name.c_str());
            awc::AWCClient::awc_status_t awc_result = _client->start(_name,
                                                                    appParams,
                                                                    windowParams, run_id);
            TRACE_L1("%s starting result result=%d run_id=%d", _TRACE_FUNCTION_, awc_result, run_id);
            result = (awc_result == awc::AWCClient::AWC_STATUS_OK);
            while (result && _pid == 0 && _appState != awc::AWC_STATE_STARTED)
            {
                _runId = run_id; _waitForResponse = true;
                TRACE_L1("%s waiting for start _name=%s", _TRACE_FUNCTION_, _name.c_str());
                _cv.wait_for(lock, std::chrono::seconds(30), [this] { return !_waitForResponse; });
                _waitForResponse = false;
                if (_appState != awc::AWC_STATE_STARTED && _appState != awc::AWC_STATE_STARTING)
                {
                    TRACE_L1("%s unexpected notification arrived _pid=%d _appState=%d", _TRACE_FUNCTION_, _pid, _appState);
                    result = false;
                }
            }
        }
        TRACE_L1("%s _pid=%d _runId=%d result=%d", _TRACE_FUNCTION_, _pid, _runId, result);
        return result;
    }

    bool AWCContainer::Stop(const uint32_t timeout /*ms*/)
    {
        TRACE_L1("%s _name=%s timeout=%u _pid=%u _runId=%d", _TRACE_FUNCTION_, _name.c_str(), timeout, _pid, _runId);
        std::unique_lock<std::mutex> lock(_mutex);
        awc::AWCClient::awc_status_t awc_result = _client->stop(_pid, 0); // documentation: exit type is important only for Netflix
        bool result = (awc_result == awc::AWCClient::AWC_STATUS_OK);
        TRACE_L1("%s awc_result=%d result=%d", _TRACE_FUNCTION_, awc_result, result);
        if (result)
        {
            _waitForResponse = true;
            TRACE_L1("%s waiting for stop _name=%s timeout=%u[ms]", _TRACE_FUNCTION_, _name.c_str(), timeout);
            _cv.wait_for(lock, std::chrono::seconds(30), [this] { return !_waitForResponse; });
            _waitForResponse = false;
            if (_appState != awc::AWC_STATE_STOPPED)
            {
                TRACE_L1("%s stop notification not arrived _pid=%d _appState=%d", _TRACE_FUNCTION_, _pid, _appState);
                result = false;
            }
        }
        TRACE_L1("%s result=%d", _TRACE_FUNCTION_, result);
        return result;
    }

    void AWCContainer::AddRef() const
    {
        TRACE_L3("%s _name=%s", _TRACE_FUNCTION_, _name.c_str());
        WPEFramework::Core::InterlockedIncrement(_referenceCount);
    }

    uint32_t AWCContainer::Release() const
    {
        TRACE_L3("%s _name=%s", _TRACE_FUNCTION_, _name.c_str());
        uint32_t retval = WPEFramework::Core::ERROR_NONE;
        if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
            delete this;
            retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return retval;
    }

    void AWCContainer::notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_runId >= 0 && req_id == _runId)
        {
            TRACE_L1("%s _name=%s _runId=%d req_id=%d app_state=%d status=%d pid=%d", _TRACE_FUNCTION_, _name.c_str(), _runId, req_id, app_state, status, pid);
            if (_appState != awc::AWC_STATE_STOPPED)
            {
                _pid = pid;
                _appState = app_state;
            }
            else
            {
                TRACE_L1("%s container was alredy stopped", _TRACE_FUNCTION_);
            }
            if (_waitForResponse)
            {
                _waitForResponse = false;
                _cv.notify_one();
            }
        }
    }

    AWCContainerAdministrator::AWCContainerAdministrator()
        : BaseContainerAdministrator()
    {
        TRACE(ProcessContainers::ProcessContainerization, (_T("AWC library initialization")));
        TRACE_L3("%s Getting AWC client instance", _TRACE_FUNCTION_);
        _client = awc::AWCClient::getInstance();
        TRACE_L3("%s AWC client instance %p", _TRACE_FUNCTION_, _client);
        if (_client)
        {
            _client->setListener(std::make_shared<AWCListener>(this));
        }
    }

    AWCContainerAdministrator::~AWCContainerAdministrator()
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        if (_client)
        {
            _client->removeListener();
        }
    }

    IContainer* AWCContainerAdministrator::Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration)
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        AWCContainer* container = nullptr;
        container = new AWCContainer(name, _client, this, containerLogDir, configuration);
        if (container == nullptr) {
            TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition for name [%s] could not be found!"), name.c_str()));
        }

        return static_cast<IContainer*>(container);
    }

    void AWCContainerAdministrator::Logging(const string& globalLogDir, const string& loggingOptions)
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
    }

    IContainerAdministrator& IContainerAdministrator::Instance()
    {
        TRACE_L3("%s", _TRACE_FUNCTION_);
        static AWCContainerAdministrator& myAWCContainerAdministrator = Core::SingletonType<AWCContainerAdministrator>::Instance();
        return myAWCContainerAdministrator;
    }

    AWCContainerAdministrator::AWCListener::AWCListener(AWCStateChangeNotifier * notifier) : _notifier(notifier) {}

    void AWCContainerAdministrator::AWCListener::notifyWindowChange(int window_id, awc::AWCClient::awc_window_state_t window_state, unsigned int pid)
    {
        // intentionally left empty
    }

    void AWCContainerAdministrator::AWCListener::notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
    {
        TRACE_L3("%s req_id=%d app_state=%d status=%d pid=%u", _TRACE_FUNCTION_, req_id, app_state, status, pid);
        _notifier->notify(req_id, app_state, status, pid);
    }

    AWCStateChangeNotifier::AWCStateChangeNotifier() : _listeners(), _mutex() {};

    void AWCStateChangeNotifier::addListener(AWCStateChangeListener * listener)
    {
        if (!listener) return;
        std::lock_guard<std::mutex> lock(_mutex);
        if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end())
        {
            _listeners.push_back(listener);
        }
        else
        {
            TRACE_L1("%s listener already registrered %p", _TRACE_FUNCTION_, listener);
        }
    }

    void AWCStateChangeNotifier::removeListener(AWCStateChangeListener * listener)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
    }

    void AWCStateChangeNotifier::notify(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto listener : _listeners)
        {
            listener->notifyStateChange(req_id, app_state, status, pid);
        }
    }
}
} //namespace WPEFramework
