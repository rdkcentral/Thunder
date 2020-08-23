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

#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Exchange {
    struct EXTERNAL IRtspClient : virtual public Core::IUnknown {
        enum { ID = ID_RTSPCLIENT };

        virtual uint32_t Configure(PluginHost::IShell* service) = 0;

        virtual uint32_t Setup(const string& assetId, uint32_t position) = 0;
        virtual uint32_t Play(int32_t scale, uint32_t position) = 0;
        virtual uint32_t Teardown() = 0;

        virtual void Set(const string& name, const string& value) = 0;
        virtual string Get(const string& name) const = 0;
    };
}
}
