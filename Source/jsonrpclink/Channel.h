#pragma once

#include <core/core.h>

namespace WPEFramework {

namespace JSONRPC {

    class Channel {
    private:
        // -----------------------------------------------------------------------------------------------
        // Create a resource allocator for all JSON objects used in these tests
        // -----------------------------------------------------------------------------------------------
        class FactoryImpl {
        private:
            FactoryImpl(const FactoryImpl&) = delete;
            FactoryImpl& operator=(const FactoryImpl&) = delete;

            FactoryImpl() : _jsonRPCFactory(2) {
            }

        public:
            static FactoryImpl& Instance();
            ~FactoryImpl() {
            }

        public:
            Core::ProxyType< Core::JSON::Message > Element() {
                return (_jsonRPCFactory.Element());
            }

        private:
            Core::ProxyPoolType< Core::JSONRPC::Message> _jsonRPCFactory;
        };

        class ChannelImpl : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&> {
        private:
            ChannelImpl(const ChannelImpl&) = delete;
            ChannelImpl& operator=(const ChannelImpl&) = delete;

            typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&> BaseClass;

        public:
            ChannelImpl(Channel& parent, const Core::NodeId& remoteNode, const string& callsign)
                : BaseClass(5, FactoryImpl::Instance(), callsign, _T("JSON"), "", "",  false, true, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
                , _parent(parent) {
            }
            virtual ~ChannelImpl() {
            }

        public:
            virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject) override {
                string textElement;
                jsonObject->ToString(textElement);

                printf(_T("[6] Received     : %s\n"), textElement.c_str());
                printf(_T("[6] Bytes        : %d\n"), static_cast<uint32_t>(textElement.size()));
            }
            virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject) override {
                string textElement;
                jsonObject->ToString(textElement);

                printf(_T("[6] Send         : %s\n"), textElement.c_str());
                printf(_T("[6] Bytes        : %d\n"), static_cast<uint32_t>(textElement.size()));
            }
            virtual void StateChange() override {
                if (IsOpen())
                    printf(_T("[6] Open         : OK\n"));
                else
                    printf(_T("[6] Closed       : %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
            }
            virtual bool IsIdle() const {
                return (true);
            }

        private:
            Channel& _parent;
        };
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

            public:
                Administrator() 
                    : _adminLock()
                    , _callsignMap() {
                }
                ~Administrator() {
                }
  
            public:
                static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
                    return (_administrator.Instance(callsign));
                }
                static uint32_t Release (const Core::ProxyType<Channel>& object) {
                    return (_administrator.Release(object));
                }

            private: 
                Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
                    Core::ProxyType<Channel> result;

                    _adminLock.Lock();

                    string searchLine = remoteNode.HostName() + '@' + callsign;

                    std::map<const string&, Core::ProxyType<Channel> >::iterator index (_callsignMap.find(searchLine));
                    if (index != _callsignMap.end()) {
                        result = Core::ProxyType<Channel>(index->second);
                    }
                    else {                    
                        ChannelProxy* entry = new ChannelProxy(remoteId, callsign);
                        _callsignMap[serachline] = entry;
                        result = Core::ProxyType<Channel>(static_cast<IReferenceCounted*>(entry), entry);
             
                    }
                    _adminLock.Unlock();

                    return (result);
                }
                uint32_t Release (const Core::ProxyType<Channel>& object) {
                    _adminLock.Lock();

                    uint32_t result = object->Release();

                    if (result == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                        // Oke remove the entry from the MAP.

                        std::map<const string&, Core::ProxyType<Channel> >::iterator index (_callsignMap.begin());

                        while ( (index != _callsignMap.end()) && (*object == index->second) ) {
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
                Core::CriticialSection _adminLock;
                std::map<const string&, Core::ProxyType<Channel> > _callsignMap;
                static Administrator _singleton;
            };

        public:
           ~ChannelProxy() {
            }
            static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
                return (Administrator.Instance(remoteNode, callsign));
            }
        public:
            virtual uint32_t Release() const override
            {
                Administrator.Release(Core::ProxyType<Channel>(const_cast<Channel*>(this)));
            }
        };


    private:
        Channel(const Core::Node& remoteNode, const string& callsign) 
            : _channel(*this, remoteNode, callsign)
            , _sequence(0) {
        }

    public:
        virtual ~Channel() {
        }
        static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign) {
            return (ChannelProxy::Instance(remoteNode, callsign));
        }

    public:
        static Core::ProxyType<Core::JSON::Message> Message() {
            return (FactoryImpl::Instance().Element());
        }
        uint32_t Sequence () const {
            return (++_sequence);
        }

    private:
        mutable std::atomic<uint32_t> _sequence;
    };

    class Client {
    private:
        Client() = delete;
        Client(const Client&) = delete;
        Client& operator= (Client&) = delete;

        class Entry {
        private:
            Entry(const Entry&) = delete;
            Entry& operator= (const Entry&) = delete;

        public:
            Entry()
                : _signal(true, false) 
                , _response() {
            }
            ~Entry() {
            }

        public:
            const Core::ProxyType<Core::JSON::Message>& Response() const {
                return(_response);
            }
            void Signal (const Core::ProxyType<Core::JSON::Message>& response) {
                _response = response;
                _signal.SetEvent();
            }
            bool WaitForResponse (const uint32_t waitTime) {
                return (_signal.Lock(waitTime) == Core::ERROR_NONE);
            }
            
        private:
            Core::Event _signal;
            Core::ProxyType<Core::JSON::Message> _response;
        };

        static Core::NodeId RemoteNodeId(const string& remoteCallSign) {
            Core::NodeId result;
            string remoteNodeId;
            if ( (Core::SystemInfo::GetEnvironment(_T("THUNDER_ACCESS"), remoteNodeId) == true) && (remoteNodeId.empty() == false) ) {
                result = Core::NodeId(remoteNodeId);
            }
            return (result);
        }

        typedef std::map<uint32_t, Entry > PendingMap;

    public:
        Client(const string& remoteCallsign, const TCHAR* localCallsign = nullptr, const bool directed = false)
            : _adminLock()
            , _connectId(RemoteNodeId())
            , _channel(Channel::Instance(_connectId, string("/Service/") + (directed && !remoteCallSign.empty() ? remoteCallsign : "Controller")))
            , _handler()
            , _callsign((!directed || remoteCallSign.empty() ? remoteCallsign : ""))
            , _pendingQueue() {

            std::vector<uint8_t> versions;
            Core::InterlockedIncrement(sequence);

            if (remoteCallsign == nullptr) {
                versions.push_back(1);
                _handler.Designator("temporary" + Core::NumberType<uint32_t>(sequence).Text(), versions);
            }
            else {
                size_t pos = designator.find_last_of('.');
                if ( (pos != string::npos) {
                    string number = source.substr(designator.substr(pos+1));
                    if ( (number.length() > 0) &&
                         (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); })) ) {
                        versions.push_back(static_cast<uint8_t>(atoi(number.c_str())));
                    }
                }
            }
        }
        virtual ~Client() {
        }

    public:
        const string& Callsign() const {
            return (_callsign);
        }
        void Callsign(const string& callsign) {
            _callsign = callsign;
        }
        template<typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        uint32_t Register (const string& eventName, const METHOD& method, REALOBJECT* objectPtr, const uint32 waitTime) {
            _handler.Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>(methodName, method, objectPtr);
            const string parameters ("{ \"event\": \"" + eventName + "\", "\"callsign\": \"" + _handler.Callsign() + "\"}" )
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send("register", parameters, response, waitTime);

            if ( (result != Core::ERROR_NONE) || (response.Error.IsSet() == true)) {
                _handler.Unregister(methodName);
            }

            return (result);
        }
        template<typename METHOD, typename REALOBJECT>
        uint32_t Register (const string& eventName, const METHOD& method, REALOBJECT objectPtr, const uint32 waitTime) {
            _handler.Register<METHOD,REALOBJECT>(methodName, method, objectPtr);
            _handler.Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>(methodName, method, objectPtr);
            const string parameters ("{ \"event\": \"" + eventName + "\", "\"callsign\": \"" + _handler.Callsign() + "\"}" )
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send("register", parameters, response, waitTime);

            if ( (result != Core::ERROR_NONE) || (response.Error.IsSet() == true)) {
                _handler.Unregister(methodName);
            }

            return (result);
        }
        void Unregister (const string& eventName) {
            _handler.Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>(methodName, method, objectPtr);
            const string parameters ("{ \"event\": \"" + eventName + "\", "\"callsign\": \"" + _handler.Callsign() + "\"}" )
            Core::ProxyType<Core::JSONRPC::Message> response;

            Send("unregister", parameters, response, waitTime);

            _handler.Unregister(methodName);
        }
        uint32_t Invoke (const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response, const uint32 waitTime) {
            return (Send(method, parameters, response, waitTime));
        }

    private:
        uint32_t Send (const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response, const uint32 waitTime) {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == true) {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::ProxyType> message (Channel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Method = _callsign + '.' + method;
                }
                else {
                    message->Method = method;
                }
                message->Parameters = parameters;

                _adminLock.Lock();
                
                std::pair<PendingMap::iterator,bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
                                                                                        std::forward_as_tuple(id),
                                                                                        std::forward_as_tuple());
                ASSERT (newElement.second == true);

                if (newElement.second == true) {

                    Entry& slot (newElement.first->second);

                    _adminLock.Unlock();

                    _channel.Submit(message);

                    message.Release();

                    if (slot.WaitForResonse(waitTime) == true) {
                        response = slot.response();
                        result = Core::ERROR_NONE;
                    }

                    _adminLock.Lock();

                    _pendingQueue.erase(id);
                }

                _adminLock.Unlock();
            }

            return (result);
        }
        uint32_t Inbound (const Cory::ProxyType<Core::JSONRPC::Message>& inbound) {

            uint32_t result = Core::ERROR_INVALID_SIGNATURE;

            ASSERT (inbound.IsValis() == true);

            if ((inbound->Id.IsSet() == true) && (inbound->Result.IsSet() || inbound->Error().IsSet())) {
                // Looks like this is a response..
                ASSERT (inbound->Parameters.IsSet() == false);
                ASSERT (inbound->Method.IsSet() == false);

                _adminLock.Lock();

                // See if we issued this..
                PendingMap::Iterator index = _pendingQueue.find(inbound->Id.Value());

                if (index != _pendingQueue.end()) {
       
                    index->second.second = inbound;
                    index->second.first.ResetEvent();

                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();
            }
            else {
                // check if we understand this message (correct callsign?)
                uint32_t result (_handler.Validate(inbound));

                if (result == Core::ERROR_NONE) {
                    // Looks like this is an event.
                    ASSERT (inbound.Id.IsSet() == false);

                    const string response;
                    uint32_t _handler.invoke(inbound.Method(), inbound.Parameters, response);
                }
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        Core::NodeId _connectId;
        Core::ProxyType<Channel> _channel;
        Core::JSON::Handler _handler;
        string& _callsign;
        PendingMap _pendingQueue;
    }
}
} // Namepsace WPEFramework.TestSystem

#endif
