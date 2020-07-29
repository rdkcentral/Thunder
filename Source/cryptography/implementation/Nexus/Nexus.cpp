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

#include "../../Module.h"

#include <cstdio> // for snprintf()

#include "Nexus.h"
#include <nxclient.h>


namespace Implementation {

Nexus::Nexus()
    : _initialized(false)
{
    NxClient_JoinSettings joinSettings;
    NxClient_GetDefaultJoinSettings(&joinSettings);
    snprintf(joinSettings.name, NXCLIENT_MAX_NAME, _T("%s"), QUOTE(MODULE_NAME));

    NEXUS_Error err = NxClient_Join(&joinSettings);
    if (err != NEXUS_SUCCESS) {
        TRACE_L1(_T("Failed to join Nexus [%08x]"), err);
    } else {
        _initialized = true;
        TRACE_L2(_T("Cryptography joined Nexus successfully"));
    }
}

Nexus::~Nexus()
{
    if (_initialized == true) {
        TRACE_L2(_T("Cryptography detaching from Nexus..."));
        NxClient_Uninit();
    }
}

} // namespace Implementation
