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

#include "COMRPCStarter.h"
#include "IPluginMonitor.h"

using namespace WPEFramework;

class COMRPCStarterAndMonitor : public COMRPCStarter, public IPluginMonitor {
public:
    class Sink : public PluginHost::IPlugin::INotification, public IShellProvider::INotification {
    public:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;
        ~Sink() override = default;

        Sink(COMRPCStarterAndMonitor *parent) : _parent(parent) {}

        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

        void Activation(const string& callsign, PluginHost::IShell* plugin) {}
        void Deactivation(const string& callsign, PluginHost::IShell* plugin) {}

        void Activated(const string& name, PluginHost::IShell* plugin) override {
            _parent->Activated(name, plugin);
        }

        void Deactivated(const string& name, PluginHost::IShell* plugin) override {
            _parent->Deactivated(name, plugin);
        }

        void Unavailable(const string& name, PluginHost::IShell* plugin) override {}

        void obtainedShell(PluginHost::IShell *shell) override {
            _parent->onShellAccess(shell);
        }

    private:
        COMRPCStarterAndMonitor *_parent;
    };

    class ReactivateJob : public Core::IDispatch {
        public:
            ReactivateJob(COMRPCStarterAndMonitor *parent) : _parent(parent) {}
            void Dispatch() override;

        private:
            COMRPCStarterAndMonitor *_parent;
    };

public:
    explicit COMRPCStarterAndMonitor(const string& pluginName);
    virtual ~COMRPCStarterAndMonitor() override;

    void configureMonitor(const uint8_t maxRetries, const uint16_t retryDelayMs);
    void onReactivationFailure(Callback &&handler);

private:
    void Activated(const string& name, PluginHost::IShell* plugin);
    void Deactivated(const string& name, PluginHost::IShell* plugin);
    void onShellAccess(PluginHost::IShell *shell);

private:
    Core::Sink<Sink> _sink;
    Core::ProxyType<ReactivateJob> _pendingJob;

    uint8_t _maxRetries;
    uint16_t _retryDelayMs;
    bool _registeredSink;
    Callback _handler;
};
