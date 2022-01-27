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

namespace WPEFramework {
namespace PluginHost {
    struct IShell;
}
}

extern "C" {
/**
 * @brief Announce the availability of the Thunder Controller Plugin's Shell
 *
 * @param controller Pointer to Controller's IShell
 *
 */
EXTERNAL void connector_announce(WPEFramework::PluginHost::IShell* controller);

/**
 * @brief Revoke the  Thunder Controller Plugin's Shell that was already announced
 *
 * @param controller Pointer to Controller's IShell
 *
 */
EXTERNAL void connector_revoke(WPEFramework::PluginHost::IShell*);

/**
 * @brief Get the Thunder Controller Plugin's Shell.
 *
 * @return Reference counted Pointer to Controller's IShell. Caller must release after usage.
 *         Returns nullptr if Controller's IShell was not announced.
 */
EXTERNAL WPEFramework::PluginHost::IShell* connector_controller();
}
