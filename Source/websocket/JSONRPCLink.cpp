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

ENUM_CONVERSION_BEGIN(WPEFramework::JSONRPC::JSONPluginState)

    { WPEFramework::JSONRPC::DEACTIVATED, _TXT("Deactivated") },
    { WPEFramework::JSONRPC::ACTIVATED, _TXT("Activated") },

ENUM_CONVERSION_END(WPEFramework::JSONRPC::JSONPluginState)

namespace JSONRPC {

template<typename INTERFACE>
typename LinkType<INTERFACE>::CommunicationChannel::FactoryImpl& LinkType<INTERFACE>::CommunicationChannel::FactoryImpl::Instance()
{
    static FactoryImpl& _singleton = Core::SingletonType<FactoryImpl>::Instance();
    return (_singleton);
}
template LinkType<Core::JSON::IElement>::CommunicationChannel::FactoryImpl& LinkType<Core::JSON::IElement>::CommunicationChannel::FactoryImpl::Instance();

template<typename INTERFACE>
typename LinkType<INTERFACE>::CommunicationChannel::ChannelProxy::Administrator& LinkType<INTERFACE>::CommunicationChannel::ChannelProxy::Administrator::Instance()
{
    static Administrator& _instance = Core::SingletonType<Administrator>::Instance();
    return (_instance);
}
template LinkType<Core::JSON::IElement>::CommunicationChannel::ChannelProxy::Administrator& LinkType<Core::JSON::IElement>::CommunicationChannel::ChannelProxy::Administrator::Instance();

}
}
