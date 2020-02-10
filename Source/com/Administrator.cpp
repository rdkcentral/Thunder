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

    void Administrator::AddRef(void* impl, const uint32_t interfaceId)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                implementation->AddRef();
            }
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    void Administrator::Release(void* impl, const uint32_t interfaceId)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                implementation->Release();
            }
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    void Administrator::Release(ProxyStub::UnknownProxy* proxy, Data::Output& response)
    {
        if (proxy->ShouldAddRefRemotely()) {
            response.AddImplementation(proxy->Implementation(), proxy->InterfaceId());
        } else if (proxy->ShouldReleaseRemotely()) {
            response.AddImplementation(proxy->Implementation(), proxy->InterfaceId() | 0x80000000);
        } else {
            proxy->ClearCache();
        }
        proxy->Release();
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

    void Administrator::RegisterProxy(ProxyStub::UnknownProxy& proxy)
    {
        const Core::IPCChannel* channel = proxy.Channel().operator->();

        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(channel));

        if (index == _channelProxyMap.end()) {
            auto slot = _channelProxyMap.emplace(std::piecewise_construct,
                std::forward_as_tuple(channel),
                std::forward_as_tuple());
            slot.first->second.push_back(&proxy);

        } else {
            ASSERT(std::find(index->second.begin(), index->second.end(), &proxy) == index->second.end());

            index->second.push_back(&proxy);
        }

        Core::InterlockedIncrement(proxy._refCount);

        _adminLock.Unlock();
    }
    void Administrator::UnregisterProxy(ProxyStub::UnknownProxy& proxy)
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
                Core::InterlockedDecrement(proxy._refCount);
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

    ProxyStub::UnknownProxy* Administrator::ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId, const bool piggyBack)
    {
        ProxyStub::UnknownProxy* result = nullptr;

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

                    if (refCounted == true) {
                       if( (*entry)->AddRefCachedCount() == false ) {
                           result = nullptr; // we cannot use this proxy it is being destructed, we need to create a new one
                       }
                    } else if (piggyBack == true) {
                        // Reference counting can be cached on this on object for now. This is a request
                        // from an incoming interface of which the lifetime is guaranteed by the callee.
                        result->EnableCaching();
                    }
                }
            }

            if (result == nullptr) {
                std::map<uint32_t, IMetadata*>::iterator index(_proxy.find(id));

                if (index != _proxy.end()) {

                    result = index->second->CreateProxy(channel, impl, refCounted);

                    ASSERT(result != nullptr);

                    if (refCounted == true) {
                        // Register it as it is remotely registered :-)
                        _channelProxyMap[channel.operator->()].push_back(result);
                    } else if (piggyBack == true) {
                        // Reference counting can be cached on this on object for now. This is a request
                        // from an incoming interface of which the lifetime is guaranteed by the callee.
                        result->EnableCaching();
                    }
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

    void Administrator::RegisterInterface(Core::ProxyType<Core::IPCChannel>& channel, Core::IUnknown* reference, void* rawImplementation, const uint32_t id)
    {
        ReferenceMap::iterator index = _channelReferenceMap.find(channel.operator->());

        if (index == _channelReferenceMap.end()) {
            auto result = _channelReferenceMap.emplace(std::piecewise_construct,
                std::forward_as_tuple(channel.operator->()),
                std::forward_as_tuple());
            index = result.first;
        } else {
            // See that it does not already exists on this channel, no need to register
            // it again!!!
            std::list<ExternalReference>::iterator element(std::find(index->second.begin(), index->second.end(), rawImplementation));

            if (element != index->second.end()) {
                element->Increment();
                rawImplementation = nullptr;
            }
        }

        if (rawImplementation != nullptr) {
            index->second.emplace_back(
                reference,
                rawImplementation,
                id);
        }
    }

    Core::IUnknown* Administrator::Convert(void* rawImplementation, const uint32_t id) 
    {
        std::map<uint32_t, ProxyStub::UnknownStub*>::const_iterator index (_stubs.find(id));
        return(index != _stubs.end() ? index->second->Convert(rawImplementation) : nullptr);
    }

    void Administrator::DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, std::list<ProxyStub::UnknownProxy*>& pendingProxies, std::list<ExposedInterface>& usedInterfaces)
    {
        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(channel.operator->()));

        if (index != _channelProxyMap.end()) {
            ProxyList::iterator loop(index->second.begin());
            while (loop != index->second.end()) {
                // There is a small possibility that the last reference to this proxy
                // interface is released in the same time before we report this interface
                // to be dead. So lets keep a refernce so we can work on a real object
                // still. This race condition, was observed by customer testing.
                if( (*loop)->DropRegistration() == true ) {
                    pendingProxies.push_back(*loop);
                }
                loop++;
            }
            _channelProxyMap.erase(index);
        }
        ReferenceMap::iterator remotes(_channelReferenceMap.find(channel.operator->()));

        if (remotes != _channelReferenceMap.end()) {
            std::list<ExternalReference>::iterator loop(remotes->second.begin());
            while (loop != remotes->second.end()) {
                usedInterfaces.emplace_back(loop->Source(), loop->RefCount());
                loop++;
            }
            _channelReferenceMap.erase(remotes);
        }

        _adminLock.Unlock();
    }

    /* static */ Administrator& Job::_administrator= Administrator::Instance();
	/* static */ Core::ProxyPoolType<Job> Job::_factory(6);

}
} // namespace Core
