#include "JSONRPCLink.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(WPEFramework::JSONRPC::JSONPluginState)

    { WPEFramework::JSONRPC::DEACTIVATED, _TXT("Deactivated") },
    { WPEFramework::JSONRPC::ACTIVATED, _TXT("Activated") },

ENUM_CONVERSION_END(WPEFramework::JSONRPC::JSONPluginState)

}
