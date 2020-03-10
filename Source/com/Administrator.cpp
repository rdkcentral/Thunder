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

#include "Administrator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace RPC {

    Administrator::Administrator()
        : _adminLock()
        , _stubs()
        , _proxy()
        , _factory(8)
        , _channelProxyMap()
    {
    }

    /* virtual */ Administrator::~Administrator()
    {
    }

    /* static */ Administrator& Administrator::Instance()
    {
        static Administrator systemAdministrator;

        return (systemAdministrator);
    }

    void Administrator::AddRef(Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                implementation->AddRef();
                RegisterInterface(channel, implementation, interfaceId);
            }
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    void Administrator::Release(Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                UnregisterInterface(channel, implementation, interfaceId);
                implementation->Release();
            }
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    // This Release is only called from the Stub code, once the invoke is completed...
    void Administrator::Release(ProxyStub::UnknownProxy* proxy, Data::Output& response)
    {
        proxy->Complete(response);
    }

    void Administrator::UnregisterProxy(const ProxyStub::UnknownProxy& proxy)
    {
        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(proxy.Channel().operator->()));

        if (index != _channelProxyMap.end()) {
            ProxyList::iterator entry(index->second.begin());
            while ((entry != index->second.end()) && ((*entry) != &proxy)) {
                entry++;
            }
            if (entry != index->second.end()) {
                index->second.erase(entry);
                if (index->second.size() == 0) {
                    _channelProxyMap.erase(index);
                }
            } else {
                TRACE_L1("Could not find the Proxy entry to be unregistered in the channel list.");
            }
        } else {
            TRACE_L1("Could not find the Proxy entry to be unregistered from a channel perspective.");
        }

        _adminLock.Unlock();
    }

    void Administrator::Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message)
    {
        uint32_t interfaceId(message->Parameters().InterfaceId());

        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            uint32_t methodId(message->Parameters().MethodId());
            index->second->Handle(methodId, channel, message);
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }
    void* Administrator::ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const uint32_t interfaceId)
    {
        void* result = nullptr;

        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(channel.operator->()));

        if (index != _channelProxyMap.end()) {
            ProxyList::iterator entry(index->second.begin());
            while ((entry != index->second.end()) && (((*entry)->InterfaceId() != id) || ((*entry)->Implementation() != impl))) {
                entry++;
            }
            if (entry != index->second.end()) {
                result = (*entry)->QueryInterface(interfaceId);
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    ProxyStub::UnknownProxy* Administrator::ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool outbound, const uint32_t interfaceId, const bool piggyBack)
    {
        ProxyStub::UnknownProxy* result = nullptr;

        ASSERT(piggyBack == !outbound);

        if (impl != nullptr) {

            _adminLock.Lock();

            ChannelMap::iterator index(_channelProxyMap.find(channel.operator->()));

            if (index != _channelProxyMap.end()) {
                ProxyList::iterator entry(index->second.begin());
                while ((entry != index->second.end()) && (((*entry)->InterfaceId() != id) || ((*entry)->Implementation() != impl))) {
                    entry++;
                }
                if (entry != index->second.end()) {
                    result = (*entry);
                }
            }

            if (result == nullptr) {
                std::map<uint32_t, IMetadata*>::iterator index(_proxy.find(id));

                if (index != _proxy.end()) {

                    result = index->second->CreateProxy(channel, impl, outbound);

                    ASSERT(result != nullptr);

                    // Register it as it is remotely registered :-)
                    _channelProxyMap[channel.operator->()].push_back(result);

                } else {
                    TRACE_L1("Failed to find a Proxy for %d.", id);
                }
            }

            _adminLock.Unlock();
        }

        return (result);
    }

    void* Administrator::ProxyInstanceQuery(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId, const bool piggyBack)
    {
        void* result = nullptr;
        ProxyStub::UnknownProxy* proxyStub = ProxyInstance(channel, impl, id, refCounted, interfaceId, piggyBack);
        if (proxyStub != nullptr) {
            result = proxyStub->QueryInterface(interfaceId);
        }
        return (result);
    }

    void Administrator::RegisterInterface(Core::ProxyType<Core::IPCChannel>& channel, Core::IUnknown* reference, const uint32_t id)
    {
        ReferenceMap::iterator index = _channelReferenceMap.find(channel.operator->());

        if (index == _channelReferenceMap.end()) {
            auto result = _channelReferenceMap.emplace(std::piecewise_construct,
                std::forward_as_tuple(channel.operator->()),
                std::forward_as_tuple());
            result.first->second.emplace_back(id, reference);
        } else {
            // See that it does not already exists on this channel, no need to register
            // it again!!!
            std::list< std::pair<uint32_t, Core::IUnknown*> >::iterator element(index->second.begin());

            while ( (element != index->second.end()) && ((element->first != id) || (element->second != reference)) ) {
                element++;
            }

            if (element == index->second.end()) {
                // Add this element to the list. We are referencing it now with a proxy on the other side..
                index->second.emplace_back(id, reference);
            }
            else {
                printf("====> According to Bartjes law, this should not happen !\n");
            }
        }
    }

    Core::IUnknown* Administrator::Convert(void* rawImplementation, const uint32_t id) 
    {
        std::map<uint32_t, ProxyStub::UnknownStub*>::const_iterator index (_stubs.find(id));
        return(index != _stubs.end() ? index->second->Convert(rawImplementation) : nullptr);
    }

    void Administrator::DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, std::list<ProxyStub::UnknownProxy*>& pendingProxies)
    {
        _adminLock.Lock();

        ReferenceMap::iterator remotes(_channelReferenceMap.find(channel.operator->()));

        if (remotes != _channelReferenceMap.end()) {
            std::list<std::pair<uint32_t, Core::IUnknown*>>::iterator loop(remotes->second.begin());
            while (loop != remotes->second.end()) {
                // We will release on behalf of the other side :-)
                loop->second->Release();
                loop++;
            }
            _channelReferenceMap.erase(remotes);
        }

        ChannelMap::iterator index(_channelProxyMap.find(channel.operator->()));

        if (index != _channelProxyMap.end()) {
            ProxyList::iterator loop(index->second.begin());
            while (loop != index->second.end()) {
                // There is a small possibility that the last reference to this proxy
                // interface is released in the same time before we report this interface
                // to be dead. So lets keep a refernce so we can work on a real object
                // still. This race condition, was observed by customer testing.
                (*loop)->AddRef();
                pendingProxies.push_back(*loop);

                loop++;
            }
        }

        _adminLock.Unlock();
    }

    /* static */ Administrator& Job::_administrator= Administrator::Instance();
	/* static */ Core::ProxyPoolType<Job> Job::_factory(6);

}
} // namespace Core
