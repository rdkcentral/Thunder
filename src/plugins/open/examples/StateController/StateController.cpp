/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include "StateController.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(StateController, 1, 0);

    /* virtual */ const string StateController::Initialize(PluginHost::IShell* service)
    {
        string message;
        ASSERT(_service == nullptr);

        // Setup skip URL for right offset.
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        // Receive all plugin information on state changes.
        _service->Register(&_sink);

        return message;
    }

    /* virtual */ void StateController::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        _clients.clear();

        _service = nullptr;
    }

    /* virtual */ string StateController::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void StateController::StateChange(PluginHost::IShell* plugin)
    {
        const string callsign(plugin->Callsign());
        _adminLock.Lock();

        std::map<const string, Entry>::iterator index(_clients.find(callsign));

        if (plugin->State() == PluginHost::IShell::ACTIVATED) {

            if (index == _clients.end()) {
                PluginHost::IStateControl* stateControl(plugin->QueryInterface<PluginHost::IStateControl>());

                if (stateControl != nullptr) {
                    _clients.emplace(std::piecewise_construct,
                        std::forward_as_tuple(callsign),
                        std::forward_as_tuple(stateControl));
                    stateControl->Release();
                }
            }
        } else if (plugin->State() == PluginHost::IShell::DEACTIVATION) {

            if (index != _clients.end()) { // Remove from the list, if it is already there
                _clients.erase(index);
            }
        }

        _adminLock.Unlock();
    }

} //namespace Plugin
} // namespace WPEFramework
