#include "Channel.h"

namespace WPEFramework {

namespace JSONRPC {

    /* static */ Channel::FactoryImpl& Channel::FactoryImpl::Instance() {
        static FactoryImpl _singleton;

        return (_singleton);
    }

    class ChannelProxy : public Core::ProxyObject<Channel> {
    private:
        ChannelProxy(const ChannelProxy&) = delete;
        ChannelProxy& operator=(const ChannelProxy&) = delete;
        ChannelProxy() = delete;

        ChannelProxy(const Core::NodeId& remoteNode, const string& callsign) : Core::ProxyObject<Channel>(remoteNode, callsign) {}

        class Administrator {
        private:
            Administrator(const Administrator&) = delete;
            Administrator& operator= (const Administrator&) = delete;

            typedef std::map<const string, Channel* > CallsignMap;

        public:
            Administrator()
                : _adminLock()
                , _callsignMap() {
            }
            ~Administrator() {
            }

        public:
            static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
                return (_administrator.InstanceImpl(remoteNode, callsign));
            }
            static uint32_t Release (const Core::ProxyType<Channel>& object) {
                return (_administrator.ReleaseImpl(object));
            }

        private:
            Core::ProxyType<Channel> InstanceImpl(const Core::NodeId& remoteNode, const string& callsign) {
                Core::ProxyType<Channel> result;

                _adminLock.Lock();

                string searchLine = remoteNode.HostName() + '@' + callsign;

                CallsignMap::iterator index (_callsignMap.find(searchLine));
                if (index != _callsignMap.end()) {
                    result = Core::ProxyType<Channel>(*(index->second));
                }
                else {
                    ChannelProxy* entry = new (0) ChannelProxy(remoteNode, callsign);
                    _callsignMap[searchLine] = entry;
                    result = Core::ProxyType<Channel>(static_cast<IReferenceCounted*>(entry), entry);
                }
                _adminLock.Unlock();

                return (result);
            }
            uint32_t ReleaseImpl (const Core::ProxyType<Channel>& object) {
                _adminLock.Lock();

                uint32_t result = object.Release();

                if (result == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                    // Oke remove the entry from the MAP.

                    CallsignMap::iterator index (_callsignMap.begin());

                    while ( (index != _callsignMap.end()) && (&(*object) == index->second) ) {
                        index++;
                    }

                    if (index != _callsignMap.end()) {
                        _callsignMap.erase(index);
                    }
                }

                _adminLock.Unlock();

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }

        private:
            Core::CriticalSection _adminLock;
            CallsignMap _callsignMap;
            static Administrator _administrator;
        };

    public:
        ~ChannelProxy() {
        }

        static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
                return (Administrator::Instance(remoteNode, callsign));
        }

    public:
        virtual uint32_t Release() const override
        {
            return (Administrator::Release(Core::ProxyType<Channel>(*const_cast<ChannelProxy*>(this))));
        }

    private:
        Core::CriticalSection _adminLock;
    };

    /* static */ ChannelProxy::Administrator ChannelProxy::Administrator::_administrator;

    /* static */ Core::ProxyType<Channel> Channel::Instance(const Core::NodeId& remoteNode, const string& callsign) {
        return (ChannelProxy::Instance(remoteNode, callsign));
    }

    uint32_t Channel::Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound) {
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _adminLock.Lock();
        std::list<Client*>::iterator index ( _observers.begin());
        while ((result != Core::ERROR_NONE) && (index != _observers.end())) { 
            result = (*index)->Inbound(inbound);
            index++;
        }
        _adminLock.Unlock();

        return (result);
    }
}
}
