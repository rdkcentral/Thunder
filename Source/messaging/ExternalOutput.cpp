/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "ExternalOutput.h"

#if !defined(MESSAGING_EXTERNAL_OUTPUT)
extern "C" {
    bool ThunderExternalOutput_Initialize(void)
    {
        return (false);
    }

    bool ThunderExternalOutput_IsEnabled(const char*, const ThunderExternalLogLevel)
    {
        return (false);
    }

    void ThunderExternalOutput_Submit(const char*, const ThunderExternalLogLevel, const char*)
    {
    }

    void ThunderExternalOutput_Deinitialize(void)
    {
    }
}
#endif