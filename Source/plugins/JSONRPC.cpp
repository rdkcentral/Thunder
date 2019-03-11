#include "JSONRPC.h"

namespace WPEFramework {

namespace PluginHost {

    /* static */ Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> JSONRPC::_jsonRPCMessageFactory(4);
}
} // namespace WPEFramework::PluginHost
