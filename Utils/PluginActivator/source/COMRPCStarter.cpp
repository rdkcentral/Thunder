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
    : IPluginStarter()
    , _connector()
    , _pluginName(pluginName)
{
}

COMRPCStarter::~COMRPCStarter()
{
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

    while (!success && currentRetry <= maxRetries) {
        LOG_INF(_pluginName.c_str(), "Attempting to activate plugin - attempt %d/%d", currentRetry, maxRetries);

        auto start = Core::Time::Now();

        if (_connector.IsOperational() == false) {
            uint32_t result = _connector.Open(RPC::CommunicationTimeOut, ControllerConnector::Connector());
            if(result != Core::ERROR_NONE) {
                LOG_ERROR(_pluginName.c_str(), "Failed to get controller interface, error %u (%s)", result, Core::ErrorToString(result));
            }
        }

        Exchange::Controller::ILifeTime* lifetime = _connector.Interface();

        if (lifetime == nullptr) {
            LOG_ERROR(_pluginName.c_str(), "Failed to open ILifeTime interface" );
            currentRetry++;

            _connector.Close(RPC::CommunicationTimeOut);

            // Sleep, then try again
            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
        } else {
            // Will block until plugin is activated
            uint32_t result = lifetime->Activate(_pluginName.c_str());

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
            lifetime->Release();
        }
    }

    if (!success) {
        LOG_ERROR(_pluginName.c_str(), "Max retries hit - giving up activating the plugin");
    }

    if (_connector.IsOperational() == true) {
        _connector.Close(RPC::CommunicationTimeOut);
    }

    return success;
}
