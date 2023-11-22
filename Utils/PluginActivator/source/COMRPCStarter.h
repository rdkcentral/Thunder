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
#include "Module.h"

#include "IPluginConfigurer.h"
#include "IPluginStarter.h"

using namespace WPEFramework;

/**
 * @brief COM-RPC implementation of a plugin starter
 *
 * Connects to Thunder over COM-RPC and attempts to start a given plugin
 */
class COMRPCStarter : public IShellProvider, public IPluginConfigurer, public IPluginStarter {
public:
    explicit COMRPCStarter(const string& pluginName);
    virtual ~COMRPCStarter() override;

    string pluginName() { return _pluginName; }

    inline PluginHost::IShell* getShell() override { return _shell; }
    void registerShellObserver(IShellProvider::INotification *sink) override;
    void unregisterShellObserver(IShellProvider::INotification *sink) override;

    void setConfigOverride(JsonObject &&overrides) override;
    bool activatePlugin(const uint8_t maxRetries, const uint16_t retryDelayMs) override;
    bool deactivatePlugin() override;

protected:
    void Submit(const Core::ProxyType<Core::IDispatch>& job);
    void Revoke(const Core::ProxyType<Core::IDispatch>& job);

private:
    Core::NodeId getConnectionEndpoint() const;
    bool applyConfigOverrides() override;

private:
    const string _pluginName;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> _engine;
    Core::ProxyType<RPC::CommunicatorClient> _client;
    PluginHost::IShell *_shell { nullptr };
    std::vector<IShellProvider::INotification*> _shellObservers;

    JsonObject _overrides;
    string _defaultConfigLine;
    string _modifiedConfigLine;
};
