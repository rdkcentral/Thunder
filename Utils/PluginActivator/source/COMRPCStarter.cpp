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

#include "COMRPCStarter.h"
#include "Log.h"

#include <chrono>
#include <thread>

COMRPCStarter::COMRPCStarter(const string& pluginName)
    : IPluginConfigurer(this)
    , _pluginName(pluginName)
    , _engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create())
    , _client(Core::ProxyType<RPC::CommunicatorClient>::Create(getConnectionEndpoint(),
          Core::ProxyType<Core::IIPCServer>(_engine)))
{
    // Announce our arrival
    _engine->Announcements(_client->Announcement());

    if (!_client.IsValid()) {
        LOG_ERROR(pluginName.c_str(), "Failed to create valid COM-RPC client");
    }
}

COMRPCStarter::~COMRPCStarter()
{
    _shellObservers.clear();

    // Close the connection just in case something left it open
    if (_client->IsOpen()) {
        _client->Close(RPC::CommunicationTimeOut);
    }

    if (_client.IsValid()) {
        _client.Release();
    }

    if (_shell) {
        _shell->Release();
        _shell = nullptr;
    }
}

void COMRPCStarter::Submit(const Core::ProxyType<Core::IDispatch>& job)
{
    _engine->Submit(job);
}

void COMRPCStarter::Revoke(const Core::ProxyType<Core::IDispatch>& job)
{
    _engine->Revoke(job);
}

void COMRPCStarter::registerShellObserver(IShellProvider::INotification *observer)
{
    if (!observer)
        return;

    auto it = std::find(_shellObservers.begin(), _shellObservers.end(), observer);
    if (it == _shellObservers.end())
        _shellObservers.push_back(observer);
}

void COMRPCStarter::unregisterShellObserver(IShellProvider::INotification *observer)
{
    if (!observer)
        return;

    auto it = std::find(_shellObservers.begin(), _shellObservers.end(), observer);
    if (it != _shellObservers.end())
        _shellObservers.erase(it);
}

void COMRPCStarter::setConfigOverride(JsonObject &&overrides)
{
    _overrides = std::move(overrides);
}

bool COMRPCStarter::applyConfigOverrides()
{
    PluginHost::IShell *shell = getShell();
    if (!_overrides.IsSet() || _overrides.Size() < 1 || !shell)
        return false;

    if (_defaultConfigLine.empty()) {
        _defaultConfigLine = shell->ConfigLine();
        LOG_DBG(_pluginName.c_str(), "Default config line : %s", _defaultConfigLine.c_str());
    }

    if (_modifiedConfigLine.empty()) {
        JsonObject defaultConfig;
        if (defaultConfig.IElement::FromString(_defaultConfigLine)) {

            auto iterator = _overrides.Variants();
            while (iterator.Next()) {
                auto key = iterator.Label();
                auto val = iterator.Current();

                LOG_INF(_pluginName.c_str(), "Applying override for key \"%s\"", key);

                if (defaultConfig.HasLabel(key)) {
                    switch (defaultConfig[key].Content()) {
                        case Core::JSON::Variant::type::BOOLEAN: val = (val.String() == "true"); break;
                        case Core::JSON::Variant::type::NUMBER: val = stol(val.String()); break;
                        case Core::JSON::Variant::type::FLOAT: val = stof(val.String()); break;
                        case Core::JSON::Variant::type::DOUBLE: val = stod(val.String()); break;
                    }
                }

                defaultConfig[key] = val;
            }

            if (!defaultConfig.ToString(_modifiedConfigLine)) {
                LOG_INF(_pluginName.c_str(), "Failure in stringifying modified config object");
                _modifiedConfigLine.clear();
            } else {
                LOG_DBG(_pluginName.c_str(), "Modified config line : %s", _modifiedConfigLine.c_str());
            }
        } else {
            LOG_INF(_pluginName.c_str(), "Failure in parsing configuration lines");
        }
    }

    if (!_modifiedConfigLine.empty()) {
        shell->ConfigLine(_modifiedConfigLine);
    } else {
        shell->ConfigLine(_defaultConfigLine);
    }

    return true;
}

/**
 * @brief Attempt to activate the plugin and automatically retry on failure
 *
 * @param[in]   maxRetries      Maximum amount of times to retry activation if it fails
 * @param[in]   retryDelayMs    Delay in ms between retry attempts
 *
 * @return True if plugin successfully activated, false if failed to activate
 */
bool COMRPCStarter::activatePlugin(const uint8_t maxRetries, const uint16_t retryDelayMs)
{
    // Attempt to open the plugin shell
    bool success = false;
    int currentRetry = 1;

    PluginHost::IShell *shell = getShell();

    while (!success && currentRetry <= maxRetries) {
        LOG_INF(_pluginName.c_str(), "Attempting to activate plugin - attempt %d/%d", currentRetry, maxRetries);

        auto start = Core::Time::Now();

        if (!shell) {
            shell = _shell = _client->Open<PluginHost::IShell>(_pluginName, ~0, RPC::CommunicationTimeOut);

            if (shell) {
                for (auto *observer : _shellObservers)
                    observer->obtainedShell(shell);
            }
        }

        // We could not open IShell for some reason - Thunder doesn't give an error about why this might have failed
        // Normally this is because either:
        //  a) Thunder isn't running or we can't connect to /tmp/communicator
        //  b) A plugin with the specified callsign does not exist
        if (!shell) {
            LOG_ERROR(_pluginName.c_str(), "Failed to open IShell interface for %s", _pluginName.c_str());
            currentRetry++;

            // Must close the connection before we retry, since even if shell is null the underlying
            // connection will still have been opened
            _client->Close(RPC::CommunicationTimeOut);

            // Sleep, then try again
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        } else {
            // Connected to Thunder successfully and got the plugin shell
            if (shell->State() == PluginHost::IShell::ACTIVATED) {
                LOG_INF(_pluginName.c_str(), "Plugin is already activated - nothing to do");
                success = true;
            } else {

                applyConfigOverrides();

                // Will block until plugin is activated
                uint32_t result = shell->Activate(PluginHost::IShell::REQUESTED);

                auto duration = Core::Time::Now().Sub(start.MilliSeconds());

                if (result != Core::ERROR_NONE) {
                    if (result == Core::ERROR_INPROGRESS) {
                        LOG_INF(_pluginName.c_str(), "Plugin activation in progress, will check after %dms", retryDelayMs);
                    } else if (result == Core::ERROR_PENDING_CONDITIONS) {
                        // Ideally we'd print out which preconditions are un-met for debugging, but that data is not exposed through the IShell interface
                        LOG_ERROR(_pluginName.c_str(), "Failed to activate plugin due to unmet preconditions after %dms", duration.MilliSeconds());
                    } else {
                        LOG_ERROR(_pluginName.c_str(), "Failed to activate plugin with error %u (%s) after %dms", result, Core::ErrorToString(result), duration.MilliSeconds());
                    }

                    // Try activation again up until the max number of retries
                    currentRetry++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                } else {
                    // Our work here is done!
                    LOG_INF(_pluginName.c_str(), "Successfully activated plugin after %dms", duration.MilliSeconds());
                    success = true;
                }
            }
        }
    }

    if (!success) {
        LOG_ERROR(_pluginName.c_str(), "Max retries hit - giving up activating the plugin");
    }

    return success;
}

/**
 * @brief Attempt to deactivate the plugin
 *
 * @return True if plugin successfully deactivated, false if failed
 */
bool COMRPCStarter::deactivatePlugin()
{
    PluginHost::IShell *shell = getShell();

    if (!shell)
        return false;

    return (shell->Deactivate(PluginHost::IShell::REQUESTED) == Core::ERROR_NONE);
}

/**
 * Retrieves the socket we will communicate over for COM-RPC
 */
Core::NodeId COMRPCStarter::getConnectionEndpoint() const
{
    string communicatorPath;
    Core::SystemInfo::GetEnvironment(_T("COMMUNICATOR_PATH"), communicatorPath);

    // On linux, Thunder defaults to /tmp/communicator for the generic COM-RPC
    // interface
    if (communicatorPath.empty()) {
        communicatorPath = _T("/tmp/communicator");
    }

    return Core::NodeId(communicatorPath.c_str());
}
