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

/**
 * Interface to configure a specified plugin before activation
 *
 * Could be implemented with JSON-RPC or COM-RPC
 */
class IPluginConfigurer {
public:
    IPluginConfigurer() = delete;
    IPluginConfigurer(const IPluginConfigurer &) = delete;
    IPluginConfigurer& operator = (const IPluginConfigurer &) = delete;

    IPluginConfigurer(IShellProvider *provider) {};
    virtual ~IPluginConfigurer() = default;

    /**
     * @brief Set the configurtions that need to be overridden (JsonObject)
     *
     * This will be merged on top of configuration line read from the IShell at the time of applying (before activation)
     * If the plugin is already running at the time of this utility invocation, nothing is done
     *
     * Note : These overrides keep updating the IShell's config line which are persistent
     *
     * @param[in]   overrides          The JsonObject that has the key:value pairs to override
     */
    virtual void setConfigOverride(JsonObject &&overrides) = 0;

    /**
     * @brief Apply the overriden configuration line to IShell
     *
     * On invocation of this API, the configuration line with all the overrides set in previous steps will be applied to IShell
     */
    virtual bool applyConfigOverrides() = 0;
};
