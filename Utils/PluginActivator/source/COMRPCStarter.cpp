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
    : _pluginName(pluginName)
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
    // Close the connection just in case something left it open
    if (_client->IsOpen()) {
        _client->Close(RPC::CommunicationTimeOut);
    }

    if (_client.IsValid()) {
        _client.Release();
    }
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

    PluginHost::IShell* shell = nullptr;

    while (!success && currentRetry <= maxRetries) {
        LOG_INF(_pluginName.c_str(), "Attempting to activate plugin - attempt %d/%d", currentRetry, maxRetries);

        auto start = Core::Time::Now();

        if (!shell) {
            shell = _client->Open<PluginHost::IShell>(_pluginName, ~0, RPC::CommunicationTimeOut);
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
                // Will block until plugin is activated
                uint32_t result = shell->Activate(PluginHost::IShell::REQUESTED);

                auto duration = Core::Time::Now().Sub(start.MilliSeconds());

                if (result != Core::ERROR_NONE) {
                    if (result == Core::ERROR_PENDING_CONDITIONS) {
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

    if (shell) {
        shell->Release();
    }

    // Be a good citizen and don't leave any open connections
    if (_client->IsOpen()) {
        _client->Close(RPC::CommunicationTimeOut);
    }

    return success;
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