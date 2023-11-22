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

#include "IShellProvider.h"

#include <functional>

using namespace WPEFramework;

using Callback = std::function<void()>;

/**
 * Interface to configure monitoring of a Plugin
 *
 * This monitor can also be configured to re-activate the failed/crashed plugin with specified timeout delays & retries
 * The Monitor actually reuses the retry & delays configured by COMRPCStarter. It simply does COMRPCStarter::activatePlugin(retry, timeout)
 * on detection of deactivation
 *
 */
class IPluginMonitor {
    public:
    IPluginMonitor() = delete;
    IPluginMonitor(const IPluginMonitor &) = delete;
    IPluginMonitor& operator = (const IPluginMonitor &) = delete;

    IPluginMonitor(IShellProvider *provider) {};
    virtual ~IPluginMonitor() = default;

    /**
     * @brief Configure the retry count & delay between retries
     *
     * @param[in]   maxRetries          Maximum amount of attempts to activate the plugin - if plugin is not activated within the amount of retries
     *                                  this method will return false
     * @param[in]   retryDelayMs        Amount of time to wait after a failed activation before retrying again
     */
    void configureMonitor(const uint8_t maxRetries, const uint16_t retryDelayMs);

    /**
     * @brief Sets a handler to deal with the case where the attempts to reactivate the plugin failed
     *
     * @param[in]   handler          A function object that carries out certain steps to deal with resurrection failure
     */
    void onReactivationFailure(Callback &&handler);
};
