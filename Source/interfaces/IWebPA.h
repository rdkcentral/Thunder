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

#ifndef WEBPA_H
#define WEBPA_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IWebPA : virtual public Core::IUnknown {
        enum { ID = ID_WEBPA };
        virtual uint32_t Initialize(PluginHost::IShell*) = 0;
        virtual void Deinitialize(PluginHost::IShell*) = 0;

        struct EXTERNAL IWebPAClient : virtual public Core::IUnknown {
            enum { ID = ID_WEBPA_CLIENT };
            virtual uint32_t Configure(PluginHost::IShell*) = 0;
            virtual uint32_t Launch() = 0;
        };

        virtual IWebPAClient* Client(const string& name) = 0;
    };
}
}

#endif // WEBPA_H
