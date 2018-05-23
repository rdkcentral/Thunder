#include "IShell.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::IShell::state)

        { PluginHost::IShell::DEACTIVATED,   _TXT("Deactivated")  },
        { PluginHost::IShell::DEACTIVATION,  _TXT("Deactivation") },
        { PluginHost::IShell::ACTIVATED,     _TXT("Activated")    },
        { PluginHost::IShell::ACTIVATION,    _TXT("Activation")   },
        { PluginHost::IShell::PRECONDITION,  _TXT("Precondition") },
        { PluginHost::IShell::DESTROYED,     _TXT("Destroyed")    },

ENUM_CONVERSION_END(PluginHost::IShell::state)

ENUM_CONVERSION_BEGIN(PluginHost::IShell::reason)

        { PluginHost::IShell::REQUESTED,       _TXT("Requested")      },
        { PluginHost::IShell::AUTOMATIC,       _TXT("Automatic")      },
        { PluginHost::IShell::FAILURE,         _TXT("Failure")        },
        { PluginHost::IShell::MEMORY_EXCEEDED, _TXT("MemoryExceeded") },
        { PluginHost::IShell::STARTUP,         _TXT("Startup")        },
        { PluginHost::IShell::SHUTDOWN,        _TXT("Shutdown")       },
        { PluginHost::IShell::CONDITIONS,      _TXT("Conditions")     },

ENUM_CONVERSION_END(PluginHost::IShell::reason)

namespace PluginHost {

	/* static */ const TCHAR* IShell::ToString(const state value) {
		return (Core::EnumerateType<state>(value).Data());
	}

	/* static */ const TCHAR* IShell::ToString(const reason value) {
		return (Core::EnumerateType<reason>(value).Data());
	}

} } // namespace PluginHost
