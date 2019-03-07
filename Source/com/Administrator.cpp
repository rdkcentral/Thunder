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
        , _changedProxies()

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
        }
        else {
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
        }
        else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }


    void Administrator::Invoke(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<InvokeMessage>& message)
    {
        uint32_t interfaceId(message->Parameters().InterfaceId());

        // stub are loaded before any action is taken and destructed if the process closes down, so no need to lock..
        std::map<uint32_t, ProxyStub::UnknownStub*>::iterator index(_stubs.find(interfaceId));

        if (index != _stubs.end()) {
            uint32_t methodId(message->Parameters().MethodId());
            index->second->Handle(methodId, channel, message);

            // Add the addrefs and releases that can be handled by this channel and where
            // issued in the mean time..
            if (_changedProxies.size() != 0) {
                Data::Output& response(message->Response());
                _adminLock.Lock();
            
                ProxyList::iterator loop (_changedProxies.begin());
                while (loop != _changedProxies.end()) {

                    if ((*loop)->Channel() == channel) {
                        if ((*loop)->ShouldAddRefRemotely()) {
                            response.AddImplementation((*loop)->Implementation(), (*loop)->InterfaceId());
                        }
                        else if ((*loop)->ShouldReleaseRemotely()) {
                            response.AddImplementation((*loop)->Implementation() , (*loop)->InterfaceId() | 0x80000000);
                            (*loop)->Release();
                        }
                        else ASSERT(false); 

                        loop = _changedProxies.erase(loop);
                    }
                    else {
                        loop++;
                    }
                }
                _adminLock.Unlock();
            }
        }
        else {
            // Oops this is an unknown interface, Do not think this could happen.
            TRACE_L1("Unknown interface. %d", interfaceId);
        }
    }

    void Administrator::RegisterProxy(ProxyStub::UnknownProxy& proxy)
    {
        _adminLock.Lock ();

        ChannelMap::iterator index(_channelProxyMap.find(proxy.Channel().operator->()));

        if (index != _channelProxyMap.end()) {
            index->second.push_back(&proxy);
            _changedProxies.push_back(&proxy);
        }
        else {
            TRACE_L1("Registering a proxy for the second time, that can not be good !!!");
        }

        _adminLock.Unlock ();
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
                if (index->second.size() == 0) {
                    _channelProxyMap.erase(index);
                }
                _changedProxies.push_back(&proxy);
            }
            else {
                TRACE_L1("Could not find the Proxy entry to be unregistered in the channel list.");
            }
        }
        else {
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

    void* Administrator::ProxyInstance(const Core::ProxyType<Core::IPCChannel>& channel, void* impl, const uint32_t id, const bool refCounted, const uint32_t interfaceId)
    {
        void* returnResult = nullptr;

        if (impl != nullptr) {

            _adminLock.Lock();

            ProxyStub::UnknownProxy* result = nullptr;

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

                    result = index->second->CreateProxy(channel, impl, refCounted);

                    ASSERT (result != nullptr);

                    if (refCounted == true) {
                        // Register it as it is remtely registered :-)
                        _channelProxyMap[channel.operator->()].push_back(result);
                    }
                }
                else {
                    TRACE_L1("Failed to find a Proxy for %d.", id);
                }
            }

            returnResult = (result != nullptr ? result->QueryInterface(interfaceId) : nullptr);

            _adminLock.Unlock();
        }

        return (returnResult);
    }
}
} // namespace Core
