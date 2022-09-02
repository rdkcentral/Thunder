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
    : mPluginName(pluginName)
    , mEngine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create())
    , mClient(Core::ProxyType<RPC::CommunicatorClient>::Create(getConnectionEndpoint(),
          Core::ProxyType<Core::IIPCServer>(mEngine)))
{
    // Announce our arrival
    mEngine->Announcements(mClient->Announcement());

    if (!mClient.IsValid()) {
        LOG_ERROR(pluginName.c_str(), "Failed to create valid COM-RPC client");
    }
}

COMRPCStarter::~COMRPCStarter()
{
    // Disconnect from Thunder and clean up
    mClient->Close(RPC::CommunicationTimeOut);
    if (mClient.IsValid()) {
        mClient.Release();
    }

    Core::Singleton::Dispose();
}

/**
 * @brief Attempt to activate the plugin and automatically retry on failure
 *
 * @param[in]   maxRetries      Maximum amount of times to retry activation if it fails
 * @param[in]   retryDelayMs    Delay in ms between retry attempts
 *
 * @return True if plugin successfully activated, false if failed to activate
 */
bool COMRPCStarter::activatePlugin(const int maxRetries, const int retryDelayMs)
{
    // Attempt to open the plugin shell
    bool success = false;
    int currentRetry = 1;

    PluginHost::IShell* shell = nullptr;

    while (!success && currentRetry <= maxRetries) {
        LOG_INF(mPluginName.c_str(), "Attempting to activate plugin - attempt %d/%d", currentRetry, maxRetries);

        auto start = std::chrono::steady_clock::now();

        if (!shell) {
            shell = mClient->Open<PluginHost::IShell>(mPluginName, ~0, RPC::CommunicationTimeOut);
        }

        // We could not open IShell for some reason - Thunder doesn't give an error about why this might have failed
        // Normally this is because either:
        //  a) Thunder isn't running or we can't connect to /tmp/communicator
        //  b) A plugin with the specified callsign does not exist
        if (!shell) {
            LOG_ERROR(mPluginName.c_str(), "Failed to open IShell interface for %s", mPluginName.c_str());
            currentRetry++;

            // Must close the connection before we retry, since even if shell is null the underlying
            // connection will still have been opened
            mClient->Close(RPC::CommunicationTimeOut);

            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            continue;
        };

        // Connected to Thunder successfully and got the plugin shell
        if (shell->State() == PluginHost::IShell::ACTIVATED) {
            LOG_INF(mPluginName.c_str(), "Plugin is already activated - nothing to do");
            success = true;
        } else {
            // Will block until plugin is activated
            uint32_t result = shell->Activate(PluginHost::IShell::REQUESTED);

            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            if (result != Core::ERROR_NONE) {
                if (result == Core::ERROR_PENDING_CONDITIONS) {
                    // Ideally we'd print out which preconditions are un-met for debugging, but that data is not exposed through the IShell interface
                    LOG_ERROR(mPluginName.c_str(), "Failed to activate plugin due to unmet preconditions after %ldms", duration.count());
                } else {
                    LOG_ERROR(mPluginName.c_str(), "Failed to activate plugin with error %u (%s) after %ldms", result, Core::ErrorToString(result), duration.count());
                }
                // Try activation again up until the max number of retries
                currentRetry++;
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                continue;
            } else {
                // Our work here is done!
                LOG_INF(mPluginName.c_str(), "Successfully activated plugin after %ldms", duration.count());
                success = true;
            }
        }
    }

    if (!success) {
        LOG_ERROR(mPluginName.c_str(), "Max retries hit - giving up activating the plugin");
    }

    if (shell) {
        shell->Release();
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
        communicatorPath = "/tmp/communicator";
    }

    return Core::NodeId(communicatorPath.c_str());
}