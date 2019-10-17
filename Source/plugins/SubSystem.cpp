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

#include "Module.h"
#include "ISubSystem.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::ISubSystem::subsystem)

    { PluginHost::ISubSystem::subsystem::PLATFORM, _TXT("Platform") },
    { PluginHost::ISubSystem::subsystem::NOT_PLATFORM, _TXT("!Platform") },

    { PluginHost::ISubSystem::subsystem::NETWORK, _TXT("Network") },
    { PluginHost::ISubSystem::subsystem::NOT_NETWORK, _TXT("!Network") },

    { PluginHost::ISubSystem::subsystem::SECURITY, _TXT("Security") },
    { PluginHost::ISubSystem::subsystem::NOT_SECURITY, _TXT("!Security") },

    { PluginHost::ISubSystem::subsystem::IDENTIFIER, _TXT("Identifier") },
    { PluginHost::ISubSystem::subsystem::NOT_IDENTIFIER, _TXT("!Identifier") },

    { PluginHost::ISubSystem::subsystem::INTERNET, _TXT("Internet") },
    { PluginHost::ISubSystem::subsystem::NOT_INTERNET, _TXT("!Internet") },

    { PluginHost::ISubSystem::subsystem::LOCATION, _TXT("Location") },
    { PluginHost::ISubSystem::subsystem::NOT_LOCATION, _TXT("!Location") },

    { PluginHost::ISubSystem::subsystem::TIME, _TXT("Time") },
    { PluginHost::ISubSystem::subsystem::NOT_TIME, _TXT("!Time") },

    { PluginHost::ISubSystem::subsystem::PROVISIONING, _TXT("Provisioning") },
    { PluginHost::ISubSystem::subsystem::NOT_PROVISIONING, _TXT("!Provisioning") },

    { PluginHost::ISubSystem::subsystem::DECRYPTION, _TXT("Decryption") },
    { PluginHost::ISubSystem::subsystem::NOT_DECRYPTION, _TXT("!Decryption") },

    { PluginHost::ISubSystem::subsystem::GRAPHICS, _TXT("Graphics") },
    { PluginHost::ISubSystem::subsystem::NOT_GRAPHICS, _TXT("!Graphics") },

    { PluginHost::ISubSystem::subsystem::WEBSOURCE, _TXT("WebSource") },
    { PluginHost::ISubSystem::subsystem::NOT_WEBSOURCE, _TXT("!WebSource") },

    { PluginHost::ISubSystem::subsystem::STREAMING, _TXT("Streaming") },
    { PluginHost::ISubSystem::subsystem::NOT_STREAMING, _TXT("!Streaming") },

    { PluginHost::ISubSystem::subsystem::BLUETOOTH, _TXT("Bluetooth") },
    { PluginHost::ISubSystem::subsystem::NOT_BLUETOOTH, _TXT("!Bluetooth") },

    ENUM_CONVERSION_END(PluginHost::ISubSystem::subsystem)

} // namespace WPEFramework
