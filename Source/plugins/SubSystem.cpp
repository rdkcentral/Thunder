#include "ISubSystem.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::ISubSystem::subsystem)

    { PluginHost::ISubSystem::subsystem::PLATFORM, _TXT("Platform") },
    { PluginHost::ISubSystem::subsystem::NOT_PLATFORM, _TXT("!Platform") },

    { PluginHost::ISubSystem::subsystem::NETWORK, _TXT("Network") },
    { PluginHost::ISubSystem::subsystem::NOT_NETWORK, _TXT("!Network") },

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

ENUM_CONVERSION_END(PluginHost::ISubSystem::subsystem)

} // namespace WPEFramework
