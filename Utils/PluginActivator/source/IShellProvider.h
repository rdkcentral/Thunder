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

using namespace WPEFramework;

/**
 * Interface to get IShell access
 *
 * Could be implemented with JSON-RPC or COM-RPC
 */
class IShellProvider {
public:
    virtual ~IShellProvider() = default;

    class INotification {
    public:
        virtual void obtainedShell(PluginHost::IShell *shell) = 0;
    };

    /**
     * @brief Return IShell interface
     */
    virtual PluginHost::IShell* getShell() = 0;

    /**
     * @brief Register an observer to get notified when IShell was acquired
     */
    virtual void registerShellObserver(INotification *sink) = 0;

    /**
     * @brief Unregister observer
     */
    virtual void unregisterShellObserver(INotification *sink) = 0;
};
