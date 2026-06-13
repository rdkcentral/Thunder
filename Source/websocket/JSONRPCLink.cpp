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

#include "JSONRPCLink.h"

namespace WPEFramework {

namespace JSONRPC {

    // Shared channel map stored in the library's data segment.
    // This ensures all DSOs using JSONRPCLink share the same map instance,
    // preventing crashes when plugins unload while others still hold references.
    Core::ProxyMapType<string, CommunicationChannelBase>& GetChannelMap()
    {
        static Core::ProxyMapType<string, CommunicationChannelBase> channelMap;
        return channelMap;
    }

} // namespace JSONRPC

ENUM_CONVERSION_BEGIN(WPEFramework::JSONRPC::JSONPluginState)

    { WPEFramework::JSONRPC::DEACTIVATED, _TXT("Deactivated") },
    { WPEFramework::JSONRPC::ACTIVATED, _TXT("Activated") },

ENUM_CONVERSION_END(WPEFramework::JSONRPC::JSONPluginState)
    namespace JSONRPC {

    // Explicit instantiation of LinkType for all supported INTERFACE types.
    // This anchors all symbols (vtables, statics, function bodies) for the entire class
    // in the websocket library, preventing use-after-free when a plugin that would
    // otherwise have instantiated this template is unloaded.
    // See the extern template declarations in JSONRPCLink.h.
    template class LinkType<Core::JSON::IElement>;
    template class LinkType<Core::JSON::IMessagePack>;

} // namespace JSONRPC


}
