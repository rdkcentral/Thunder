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

namespace WPEFramework {

namespace PluginHost {

struct DEPRECATED EXTERNAL IController : public virtual Core::IUnknown {

    enum { ID = RPC::ID_CONTROLLER };

    ~IController() override = default;

    virtual uint32_t Persist() = 0;

    virtual uint32_t Delete(const string& path) = 0;

    virtual uint32_t Reboot() = 0;

    virtual uint32_t Environment(const string& index, string& environment /* @out */ ) const = 0;

    virtual uint32_t Configuration(const string& callsign, string& configuration /* @out */) const = 0;
    virtual uint32_t Configuration(const string& callsign, const string& configuration) = 0;

    virtual uint32_t Clone(const string& basecallsign, const string& newcallsign) = 0;
};

}

} // namespace WPEFramework
