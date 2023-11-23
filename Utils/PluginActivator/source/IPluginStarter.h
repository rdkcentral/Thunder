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

/**
 * Interface to start a specified plugin
 *
 * Could be implemented with JSON-RPC or COM-RPC
 */
class IPluginStarter {
public:
    virtual ~IPluginStarter() = default;

    /**
     * @brief Activate a Thunder plugin
     *
     * Will block until either the Thunder plugin has activated or until the maximum amount of retires has occurred
     *
     * @param[in]   maxRetries          Maximum amount of attempts to activate the plugin - if plugin is not activated within the amount of retries
     *                                  this method will return false
     * @param[in]   retryDelayMs        Amount of time to wait after a failed activation before retrying again
     */
    virtual bool activatePlugin(const uint8_t maxRetries, const uint16_t retryDelayMs) = 0;

    /**
     * @brief Deactivate a Thunder plugin
     *
     * Will block until either the Thunder plugin is deactivated or until the request times out
     *
     * @param[in]   maxRetries          Maximum amount of attempts to deactivate the plugin - if plugin is not deactivated within the amount of retries
     *                                  this method will return false
     * @param[in]   retryDelayMs        Amount of time to wait after a failed deactivation before retrying again
     */
    virtual bool deactivatePlugin(const uint8_t maxRetries, const uint16_t retryDelayMs) = 0;
};
