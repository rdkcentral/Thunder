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

#include "Administrator.h"
#include "IUnknown.h"

namespace Thunder {
namespace RPC {

    Administrator::Administrator()
        : _adminLock()
        , _stubs()
        , _proxy()
        , _factory(8)
        , _channelProxyMap()
        , _channelReferenceMap()
        , _danglingProxies()
        , _delegatedReleases(true)
    {
    }

    /* virtual */ Administrator::~Administrator()
    {
        for (auto& proxy : _proxy) {
            delete proxy.second;
        }

        for (auto& stub : _stubs) {
            delete stub.second;
        }

        _proxy.clear();
        _stubs.clear();
    }

    /* static */ Administrator& Administrator::Instance()
    {
        // We tried this, but the proxy-stubs are not SingletonType. Turing proxy-stubs in to SingletonType
        // make them needless complex and require more memory.
        // static Administrator& systemAdministrator = Core::SingletonType<Administrator>::Instance();
        static Administrator systemAdministrator;

        return (systemAdministrator);
    }

    void Administrator::AddRef(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        Stubs::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                _adminLock.Lock();
                if (channel.IsValid() == true) {
                    implementation->AddRef();
                    RegisterUnknown(channel, implementation, interfaceId);
                }
                _adminLock.Unlock();
            }
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    void Administrator::Release(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t interfaceId, const uint32_t dropCount)
    {
        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        Stubs::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            Core::IUnknown* implementation(index->second->Convert(impl));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                _adminLock.Lock();
                if (channel.IsValid() == true) {
                    UnregisterInterface(channel, implementation, interfaceId, dropCount);
                    implementation->Release();
                }
                _adminLock.Unlock();
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

    bool Administrator::UnregisterUnknownProxy(const ProxyStub::UnknownProxy& proxy)
    {
        bool removed = false;

        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(proxy.Id()));

        if (index != _channelProxyMap.end()) {
            Proxies::iterator entry(index->second.begin());
            while ((entry != index->second.end()) && ((*entry) != &proxy)) {
                entry++;
            }

            ASSERT(entry != index->second.end());

            if (entry != index->second.end()) {
                index->second.erase(entry);
                removed = true;
                if (index->second.size() == 0) {
                    _channelProxyMap.erase(index);
                }
            }
        } else {
            // If the channel nolonger exists, check the dangling map
            Proxies::iterator index = std::find(_danglingProxies.begin(), _danglingProxies.end(), &proxy);

            if (index != _danglingProxies.end()) {
                _danglingProxies.erase(index);
                removed = true;
            }
            else {
                TRACE_L1("Could not find the Proxy entry to be unregistered from a channel perspective.");
            }
        }

        _adminLock.Unlock();

        return removed;
    }

    void Administrator::Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message)
    {
        uint32_t interfaceId(message->Parameters().InterfaceId());

        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        Stubs::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            uint32_t methodId(message->Parameters().MethodId());
            REPORT_DURATION_WARNING({ index->second->Handle(methodId, channel, message); },  WarningReporting::TooLongInvokeRPC, interfaceId, methodId);
        } else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    bool Administrator::IsValid(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const uint32_t id) const
    {
        // Used by secure stubs.

        bool result = false;

        if (impl != 0) {
            // Firstly check against the temporarily valid instances (i.e. interfaces currently passed as parameters)
            const RPC::InstanceRecord* tempInstances = static_cast<const RPC::InstanceRecord*>(channel->CustomData());

            if (tempInstances != nullptr) {

                while ((*tempInstances).interface != 0) {

                    if ((impl == (*tempInstances).instance) && (id == (*tempInstances).interface)) {
                        TRACE_L3("Validated instance 0x%08" PRIxPTR " by local set", impl);
                        result = true;
                        break;
                    }

                    tempInstances++;
                }
            }

            // If not there, look in the administration...
            if (result == false) {
                _adminLock.Lock();

                ReferenceMap::const_iterator index(_channelReferenceMap.find(channel->Id()));
                const Core::IUnknown* unknown = Convert(reinterpret_cast<void*>(impl), id);

                result = ((index != _channelReferenceMap.end()) &&
                            (std::find_if(index->second.begin(), index->second.end(), [&](const RecoverySet& element) {
                                    return ((element.Unknown() == unknown) && (element.Id() == id));
                            }) != index->second.end()));

                _adminLock.Unlock();

                if (result == true) {
                    TRACE_L3("Validated instance 0x%08" PRId64 " by administration", impl);
                } else {
                    TRACE_L1("Failed to validate instance 0x%08" PRId64 " of interface 0x%08x", impl, id);
                }
            }
        }

        return (result);
    }

    ProxyStub::UnknownProxy* Administrator::ProxyFind(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const uint32_t id, void*& interface)
    {
        ProxyStub::UnknownProxy* result = nullptr;

        _adminLock.Lock();

        ChannelMap::iterator index(_channelProxyMap.find(channel->Id()));

        if (index != _channelProxyMap.end()) {
            Proxies::iterator entry(index->second.begin());
            while ((entry != index->second.end()) && (((*entry)->InterfaceId() != id) || ((*entry)->Implementation() != impl))) {
                entry++;
            }
            if (entry != index->second.end()) {
                interface = (*entry)->QueryInterface(id);
                if (interface != nullptr) {
                    result = (*entry);
                }
            }
        }

        _adminLock.Unlock();

        return (result);
    }

    ProxyStub::UnknownProxy* Administrator::ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, const Core::instance_id& impl, const bool outbound, const uint32_t id, void*& interface)
    {
        ProxyStub::UnknownProxy* result = nullptr;

        interface = nullptr;

        if (impl) {

            _adminLock.Lock();

            if (channel.IsValid() == true) {

                uint32_t channelId(channel->Id());
                ChannelMap::iterator index(_channelProxyMap.find(channelId));

                if (index != _channelProxyMap.end()) {
                    Proxies::iterator entry(index->second.begin());
                    while ((entry != index->second.end()) && (((*entry)->InterfaceId() != id) || ((*entry)->Implementation() != impl))) {
                        entry++;
                    }
                    if (entry != index->second.end()) {
                        interface = (*entry)->Acquire(outbound, id);

                        // The implementation could be found, but the current implemented proxy is not
                        // for the given interface. If that cae, the interface == nullptr and we still
                        // need to create a proxy for this specific interface.
                        if (interface != nullptr) {
                            result = (*entry);
                        }
                    }
                }

                if (result == nullptr) {
                    Factories::iterator factory(_proxy.find(id));

                    if (factory != _proxy.end()) {

                        result = factory->second->CreateProxy(channel, impl, outbound);

                        ASSERT(result != nullptr);

                        // Register it as it is remotely registered :-)
                        _channelProxyMap[channelId].push_back(result);

                        // This will increment the reference count to 2 (one in the ChannelProxyMap and one in the QueryInterface ).
                        interface = result->QueryInterface(id);
                        ASSERT(interface != nullptr);

                    } else {
                        TRACE_L1("Failed to find a Proxy for %d.", id);
                    }
                }
            }

            _adminLock.Unlock();
        }

        return (result);
    }

    // Locked by the Callee and the channel has been checked that it exists.. (IsValid)
    void Administrator::RegisterUnknown(const Core::ProxyType<Core::IPCChannel>& channel, Core::IUnknown* reference, const uint32_t id)
    {
        ASSERT(reference != nullptr);
        ASSERT(channel.IsValid() == true);

        uint32_t channelId(channel->Id());
        ReferenceMap::iterator index = _channelReferenceMap.find(channelId);

        if (index == _channelReferenceMap.end()) {
            auto result = _channelReferenceMap.emplace(std::piecewise_construct,
                std::forward_as_tuple(channelId),
                std::forward_as_tuple());
            result.first->second.emplace_back(id, reference);
            TRACE_L3("Registered interface %p(0x%08x).", reference, id);
        } else {
            // See that it does not already exists on this channel, no need to register
            // it again!!!
            std::list< RecoverySet >::iterator element(index->second.begin());

            while ( (element != index->second.end()) && ((element->Id() != id) || (element->Unknown() != reference)) ) {
                element++;
            }

            if (element == index->second.end()) {
                // Add this element to the list. We are referencing it now with a proxy on the other side..
                index->second.emplace_back(id, reference);
                TRACE_L3("Registered interface %p(0x%08x).", reference, id);
            }
            else {
                // If this happens, it means that the interface we are trying to register, is already handed out, over the same channel.
                // This means, that on the otherside (the receiving side) that will create a Proxy for this interface, finds this interface as well.
                // Now two things can happen:
                // 1) Everything is stable, when this call arrives on the otherside, the proxy is found, and the externalReferenceCount (the number
                //    of AddRefs the RemoteSide has on this Real Object is incremented by one).
                // 2) Corner case, unlikely top happen, but we need to cater for it. If during the return of this reference, that Proxy on the otherside
                //    might reach the reference 0. That will, on that side, clear out the proxy. That will send a Release for that proxy to this side and
                //    that release will not kill the "real" object here becasue we have still a reference on the real object for this interface. When this
                //    interface reaches the other side, it will simply create a new proxy with an externalReference COunt of 1.
                //
                // However, if the connection dies and scenario 2 took place, and we did *not* reference count this cleanup map, this reference for the newly
                // created proxy in step 2, is in case of a crash never released!!! So to avoid this scenario, we should also reference count the cleanup map
                // interface entry here, than we are good to go, as long as the "dropReleases" count also ends up here :-)
                TRACE_L3("Interface 0x%p(0x%08x) is already registered.", reference, id);
                element->Increment();
            }
        }
    }

    // Locked by the Callee and the channel has been checked that it exists.. (IsValid)
    void Administrator::UnregisterUnknown(const Core::ProxyType<Core::IPCChannel>& channel, const Core::IUnknown* source, const uint32_t interfaceId, const uint32_t dropCount) {
        ASSERT(source != nullptr);
        ASSERT(channel.IsValid() == true);

        ReferenceMap::iterator index(_channelReferenceMap.find(channel->Id()));

        if (index != _channelReferenceMap.end()) {
            std::list< RecoverySet >::iterator element(index->second.begin());

            while ( (element != index->second.end()) && ((element->Id() != interfaceId) || (element->Unknown() != source)) ) {
                element++;
            }

            ASSERT(element != index->second.end());

            if (element != index->second.end()) {
                if (element->Decrement(dropCount) == false) {
                    index->second.erase(element);
                    if (index->second.size() == 0) {
                        _channelReferenceMap.erase(index);
                        TRACE_L3("Unregistered interface %p(%u).", source, interfaceId);
                    }
                }
            } else {
                printf("====> Unregistering an interface [0x%x, %d] which has not been registered!!!\n", interfaceId, Core::ProcessInfo().Id());
            }
        } else {
            printf("====> Unregistering an interface [0x%x, %d] from a non-existing channel!!!\n", interfaceId, Core::ProcessInfo().Id());
        }
    }

    Core::IUnknown* Administrator::Convert(void* rawImplementation, const uint32_t id)
    {
        Stubs::const_iterator index (_stubs.find(id));
        return(index != _stubs.end() ? index->second->Convert(rawImplementation) : nullptr);
    }

    const Core::IUnknown* Administrator::Convert(void* rawImplementation, const uint32_t id) const
    {
        Stubs::const_iterator index (_stubs.find(id));
        return(index != _stubs.end() ? index->second->Convert(rawImplementation) : nullptr);
    }

    void Administrator::DeleteChannel(const Core::ProxyType<Core::IPCChannel>& channel, Proxies& pendingProxies)
    {
        _adminLock.Lock();

        uint32_t channelId(channel->Id());
        ReferenceMap::iterator remotes(_channelReferenceMap.find(channelId));

        if (remotes != _channelReferenceMap.end()) {
            std::list<RecoverySet>::iterator loop(remotes->second.begin());
            while (loop != remotes->second.end()) {

                Core::IUnknown* iface = loop->Unknown();
                ASSERT(iface != nullptr);

                if ((_delegatedReleases == true) && (iface != nullptr) && (loop->IsComposit() == false)) {

                    uint32_t result;

                    // We will release on behalf of the other side :-)
                    do {
                        result = iface->Release();
                    } while ((loop->Decrement(1)) && (result == Core::ERROR_NONE));
                }

                ASSERT (loop->Flushed() == true);
                loop++;
            }
            _channelReferenceMap.erase(remotes);
        }

        ChannelMap::iterator index(_channelProxyMap.find(channelId));

        if (index != _channelProxyMap.end()) {
            for (auto entry : index->second) {
                entry->Invalidate();
                _danglingProxies.emplace_back(entry);

                // This is actually for the pendingProxies to be reported
                // dangling!!
                entry->AddRef();
            }
            // The _channelProxyMap does have a reference for each Proxy it
            // holds, so it is safe to just move the vector from the map to
            // the pendingProxies. The receiver of pendingProxies has to take
            // care of releasing the last reference we, as administration layer
            // hold upon this..
            pendingProxies = std::move(index->second);
            _channelProxyMap.erase(index);
        }

        _adminLock.Unlock();
    }

    /* static */ Administrator& Job::_administrator= Administrator::Instance();
	/* static */ Core::ProxyPoolType<Job> Job::_factory(6);
}
} // namespace Core
