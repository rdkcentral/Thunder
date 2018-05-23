#include "IStateControl.h"

namespace WPEFramework { 

ENUM_CONVERSION_BEGIN(PluginHost::IStateControl::state)

        { PluginHost::IStateControl::UNINITIALIZED, _TXT("UNINITIALIZED") },
        { PluginHost::IStateControl::SUSPENDED,     _TXT("SUSPENDED")     },
        { PluginHost::IStateControl::RESUMED,       _TXT("RESUMED")       },

ENUM_CONVERSION_END(PluginHost::IStateControl::state)

ENUM_CONVERSION_BEGIN(PluginHost::IStateControl::command)

        { PluginHost::IStateControl::SUSPEND,       _TXT("Suspend")       },
        { PluginHost::IStateControl::RESUME,        _TXT("Resume")        },

ENUM_CONVERSION_END(PluginHost::IStateControl::command)

namespace PluginHost {

	/* static */ const TCHAR* IStateControl::ToString(const IStateControl::state value) {
		return (Core::EnumerateType<state>(value).Data());
	}

	/* static */ const TCHAR* IStateControl::ToString(const IStateControl::command value) {
		return (Core::EnumerateType<state>(value).Data());
	}
}

} // namespace PluginHost
