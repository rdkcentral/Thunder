#pragma once

#include "Module.h"

namespace WPEFramework {

namespace JSONRPC {

    class Client;

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
            Core::ProxyType<Core::JSONRPC::Message> Element(const string&) {
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
                Core::ProxyType<Core::JSONRPC::Message> inbound (Core::proxy_cast<Core::JSONRPC::Message>(jsonObject));

                ASSERT (inbound.IsValid() == true);

                if (inbound.IsValid() == true) {
                    _parent.Inbound(inbound);
                }
            }
            virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject) override {
                #ifdef __DEBUG__
                Core::ProxyType<Core::JSONRPC::Message> inbound (Core::proxy_type<Core::JSONRPC::Message>(jsonObject));

                ASSERT (inbound.IsValid() == true);

                if (inbound.IsValid() == true) {
                    TRACE_L1("Message: %s send", inbound.ToString().c_str());
                }
                #endif
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

    protected:
        Channel(const Core::NodeId& remoteNode, const string& callsign) 
            : _channel(*this, remoteNode, callsign)
            , _sequence(0) {
        }

    public:
        virtual ~Channel() {
        }
        static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign);

    public:
        static Core::ProxyType<Core::JSONRPC::Message> Message() {
            return (FactoryImpl::Instance().Element(string()));
        }
        uint32_t Sequence () const {
            return (++_sequence);
        }
        void Register (Client& client) {
            _adminLock.Lock();
            ASSERT (std::find(_observers.begin() , _observers.end(), &client) == _observers.end());
            _observers.push_back(&client);
            _adminLock.Unlock();
        }
        void Unregister (Client& client) {
            _adminLock.Lock();
            std::list<Client*>::iterator index (std::find(_observers.begin() , _observers.end(), &client));
            if (index == _observers.end()) {
                _observers.erase(index);
            }
            _adminLock.Unlock();
        }
        void Submit(const Core::ProxyType<Core::JSON::IElement>& message) {
            _channel.Submit(message);
        }

    private:
        uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound);

    private:
        Core::CriticalSection _adminLock;
        ChannelImpl _channel;
        mutable std::atomic<uint32_t> _sequence;
        std::list<Client*> _observers;
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
            const Core::ProxyType<Core::JSONRPC::Message>& Response() const {
                return(_response);
            }
            void Signal (const Core::ProxyType<Core::JSONRPC::Message>& response) {
                _response = response;
                _signal.SetEvent();
            }
            bool WaitForResponse (const uint32_t waitTime) {
                return (_signal.Lock(waitTime) == Core::ERROR_NONE);
            }
            
        private:
            Core::Event _signal;
            Core::ProxyType<Core::JSONRPC::Message> _response;
        };

        static Core::NodeId RemoteNodeId() {
            Core::NodeId result;
            string remoteNodeId;
            if ( (Core::SystemInfo::GetEnvironment(_T("THUNDER_ACCESS"), remoteNodeId) == true) && (remoteNodeId.empty() == false) ) {
                result = Core::NodeId(remoteNodeId.c_str());
            }
            return (result);
        }

        typedef std::map<uint32_t, Entry > PendingMap;

    public:
        Client(const string& remoteCallsign, const TCHAR* localCallsign = nullptr, const bool directed = false)
            : _adminLock()
            , _connectId(RemoteNodeId())
            , _channel(Channel::Instance(_connectId, string("/Service/") + (directed && !remoteCallsign.empty() ? remoteCallsign : "Controller")))
            , _handler()
            , _callsign((!directed || remoteCallsign.empty()) ? remoteCallsign : "")
            , _pendingQueue() {
            static uint32_t sequence;

            std::vector<uint8_t> versions;
            Core::InterlockedIncrement(sequence);

            if (localCallsign == nullptr) {
                versions.push_back(1);
                _handler.Designator("temporary" + Core::NumberType<uint32_t>(sequence).Text(), versions);
            }
            else {
                string designator (localCallsign);
                size_t pos = designator.find_last_of('.');
                if (pos != string::npos) {
                    string number = designator.substr(pos+1);
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
        uint32_t Register (const string& eventName, const METHOD& method, REALOBJECT* objectPtr, const uint32_t waitTime) {
            _handler.Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>(eventName, method, objectPtr);
            const string parameters ("{ \"event\": \"" + eventName + "\", \"callsign\": \"" + _handler.Callsign() + "\"}" );
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send("register", parameters, response, waitTime);

            if ( (result != Core::ERROR_NONE) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        template<typename METHOD, typename REALOBJECT>
        uint32_t Register (const string& eventName, const METHOD& method, REALOBJECT objectPtr, const uint32_t waitTime) {
            _handler.Register<METHOD,REALOBJECT>(eventName, method, objectPtr);
            const string parameters ("{ \"event\": \"" + eventName + "\", \"callsign\": \"" + _handler.Callsign() + "\"}" );
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send("register", parameters, response, waitTime);

            if ( (result != Core::ERROR_NONE) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        void Unregister (const string& eventName, const uint32_t waitTime) {
            const string parameters ("{ \"event\": \"" + eventName + "\", \"callsign\": \"" + _handler.Callsign() + "\"}" );
            Core::ProxyType<Core::JSONRPC::Message> response;

            Send("unregister", parameters, response, waitTime);

            _handler.Unregister(eventName);
        }
        template <typename PARAMETERS, typename RESPONSE>
        uint32_t Invoke (const string& method, const PARAMETERS& parameters, RESPONSE& response, const uint32_t waitTime) {
            uint32_t result = Send(method, parameters.ToString(), response, waitTime);
            if (result == Core::ERROR_NONE) {
                response.FromString(response->result.Value());
            }
            return (result);
        }
        uint32_t Invoke (const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response, const uint32_t waitTime) {
            return (Send(method, parameters, response, waitTime));
        }

    private:
        friend class Channel;

        uint32_t Send (const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response, const uint32_t waitTime) {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == true) {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::JSONRPC::Message> message (Channel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Designator = _callsign + '.' + method;
                }
                else {
                    message->Designator = method;
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

                    _channel->Submit(message);

                    message.Release();

                    if (slot.WaitForResponse(waitTime) == true) {
                        response = slot.Response();
                        result = Core::ERROR_NONE;
                    }

                    _adminLock.Lock();

                    _pendingQueue.erase(id);
                }

                _adminLock.Unlock();
            }

            return (result);
        }
        uint32_t Inbound (const Core::ProxyType<Core::JSONRPC::Message>& inbound) {

            uint32_t result = Core::ERROR_INVALID_SIGNATURE;

            ASSERT (inbound.IsValis() == true);

            if ((inbound->Id.IsSet() == true) && (inbound->Result.IsSet() || inbound->Error.IsSet())) {
                // Looks like this is a response..
                ASSERT (inbound->Parameters.IsSet() == false);
                ASSERT (inbound->Method.IsSet() == false);

                _adminLock.Lock();

                // See if we issued this..
                PendingMap::iterator index = _pendingQueue.find(inbound->Id.Value());

                if (index != _pendingQueue.end()) {
       
                    index->second.Signal(inbound);

                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();
            }
            else {
                // check if we understand this message (correct callsign?)
                uint32_t result (_handler.Validate(*inbound));

                if (result == Core::ERROR_NONE) {
                    // Looks like this is an event.
                    ASSERT (inbound.Id.IsSet() == false);

                    string response;
                    _handler.Invoke(~0, inbound->Method(), inbound->Parameters.Value(), response);
                }
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        Core::NodeId _connectId;
        Core::ProxyType<Channel> _channel;
        Core::JSONRPC::Handler _handler;
        string _callsign;
        PendingMap _pendingQueue;
    };
}
} // Namepsace WPEFramework::JSONRPC
