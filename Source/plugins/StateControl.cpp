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

#include "Module.h"
#include "IStateControl.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(PluginHost::IStateControl::state)

    { PluginHost::IStateControl::UNINITIALIZED, _TXT("UNINITIALIZED") },
    { PluginHost::IStateControl::SUSPENDED, _TXT("SUSPENDED") },
    { PluginHost::IStateControl::RESUMED, _TXT("RESUMED") },

    ENUM_CONVERSION_END(PluginHost::IStateControl::state)

    ENUM_CONVERSION_BEGIN(PluginHost::IStateControl::command)

    { PluginHost::IStateControl::SUSPEND, _TXT("Suspend") },
    { PluginHost::IStateControl::RESUME, _TXT("Resume") },

    ENUM_CONVERSION_END(PluginHost::IStateControl::command)

namespace PluginHost
{

    /* static */ const TCHAR* IStateControl::ToString(const IStateControl::state value)
    {
        return (Core::EnumerateType<state>(value).Data());
    }

    /* static */ const TCHAR* IStateControl::ToString(const IStateControl::command value)
    {
        return (Core::EnumerateType<state>(value).Data());
    }
}

} // namespace PluginHost
