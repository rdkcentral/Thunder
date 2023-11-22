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

#include "COMRPCMonitor.h"
#include "Log.h"

#include <chrono>
#include <thread>

COMRPCStarterAndMonitor::COMRPCStarterAndMonitor(const string& pluginName)
    : IPluginMonitor(this)
    , COMRPCStarter(pluginName)
    , _maxRetries(0)
    , _retryDelayMs(500)
    , _sink(this)
    , _registeredSink(false)
{
    registerShellObserver(&_sink);
}

COMRPCStarterAndMonitor::~COMRPCStarterAndMonitor()
{
    if (_pendingJob.IsValid()) {
        Revoke(Core::ProxyType<Core::IDispatch>(_pendingJob));
        _pendingJob.Release();
    }

    unregisterShellObserver(&_sink);

    PluginHost::IShell *shell = getShell();
    if (shell)
        shell->Unregister(&_sink);
}

void COMRPCStarterAndMonitor::onShellAccess(PluginHost::IShell *shell)
{
    configureMonitor(_maxRetries, _retryDelayMs);
}

void COMRPCStarterAndMonitor::Activated(const string& callsign, PluginHost::IShell* plugin)
{
    if (callsign != pluginName())
        return;

    LOG_INF(pluginName().c_str(), "Plugin activated");
}

void COMRPCStarterAndMonitor::Deactivated(const string& callsign, PluginHost::IShell* plugin)
{
    if (callsign != pluginName())
        return;

    if (_pendingJob.IsValid()) {
        Revoke(Core::ProxyType<Core::IDispatch>(_pendingJob));
        _pendingJob.Release();
    }

    LOG_INF(pluginName().c_str(), "Plugin deactivated - attempting to re-activate");
    _pendingJob = Core::ProxyType<ReactivateJob>::Create(this);
    Submit(Core::ProxyType<Core::IDispatch>(_pendingJob));
}

void COMRPCStarterAndMonitor::configureMonitor(const uint8_t maxRetries, const uint16_t retryDelayMs)
{
    _maxRetries = maxRetries;
    _retryDelayMs = retryDelayMs;

    PluginHost::IShell *shell = getShell();
    if (!shell)
        return;

    if (!_registeredSink && _maxRetries > 0) {
        _registeredSink = true;
        shell->Register(&_sink);
    } else if (_registeredSink && _maxRetries < 1) {
        _registeredSink = false;
        shell->Unregister(&_sink);
    }
}

void COMRPCStarterAndMonitor::onReactivationFailure(Callback &&handler)
{
    _handler = move(handler);
}

void COMRPCStarterAndMonitor::ReactivateJob::Dispatch()
{
    if (!_parent)
        return;

    if (!_parent->activatePlugin(_parent->_maxRetries, _parent->_retryDelayMs) && _parent->_handler) {
        LOG_WARN(_parent->pluginName().c_str(), "Failed to re-activate plugin, proceeding to exit");
        _parent->_handler();
    }
}
