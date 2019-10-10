#pragma once

#include "Module.h"
#include "WebSocketLink.h"

namespace WPEFramework {

namespace JSONRPC {

    using namespace Core::TypeTraits;

    template<typename INTERFACE>
    class LinkType {
    private:
        LinkType() = delete;
        LinkType(const LinkType&) = delete;
        LinkType& operator=(LinkType&) = delete;

        typedef std::function<void(const Core::JSONRPC::Message&)> CallbackFunction;

        class CommunicationChannel {
        private:
            // -----------------------------------------------------------------------------------------------
            // Create a resource allocator for all JSON objects used in these tests
            // -----------------------------------------------------------------------------------------------
            class EXTERNAL FactoryImpl {
            private:
                FactoryImpl(const FactoryImpl&) = delete;
                FactoryImpl& operator=(const FactoryImpl&) = delete;
    
                class EXTERNAL WatchDog {
                private:
                    WatchDog() = delete;
                    WatchDog& operator=(const WatchDog&) = delete;
    
                public:
                    WatchDog(LinkType<INTERFACE>* client)
                        : _client(client)
                    {
                    }
                    WatchDog(const WatchDog& copy)
                        : _client(copy._client)
                    {
                    }
                    ~WatchDog()
                    {
                    }
    
                    bool operator==(const WatchDog& rhs) const
                    {
                        return (rhs._client == _client);
                    }
                    bool operator!=(const WatchDog& rhs) const
                    {
                        return (!operator==(rhs));
                    }
    
                public:
                    uint64_t Timed(const uint64_t scheduledTime) {
                        return (_client->Timed());
                    }
    
                private:
                    LinkType<INTERFACE>* _client;
                };
    
                friend Core::SingletonType<FactoryImpl>;
    
                FactoryImpl()
                    : _jsonRPCFactory(2)
                    , _watchDog(Core::Thread::DefaultStackSize(), _T("JSONRPCCleaner"))
                {
                }
    
            public:
                static FactoryImpl& Instance()
                {
                    static FactoryImpl& _singleton = Core::SingletonType<FactoryImpl>::Instance();
    
                    return (_singleton);
                }
    
    
                ~FactoryImpl()
                {
                }
    
            public:
                Core::ProxyType<Core::JSONRPC::Message> Element(const string&)
                {
                    return (_jsonRPCFactory.Element());
                }
                void Trigger(const uint64_t& time, LinkType<INTERFACE>* client)
                {
                    _watchDog.Trigger(time, client);
                }
                void Revoke(LinkType<INTERFACE>* client)
                {
                    _watchDog.Revoke(client);
                }
    
            private:
                Core::ProxyPoolType<Core::JSONRPC::Message> _jsonRPCFactory;
                Core::TimerType<WatchDog> _watchDog;
            };
    
            class ChannelImpl : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, INTERFACE> {
            private:
                ChannelImpl(const ChannelImpl&) = delete;
                ChannelImpl& operator=(const ChannelImpl&) = delete;
    
                typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&, INTERFACE> BaseClass;
    
            public:
                ChannelImpl(CommunicationChannel* parent, const Core::NodeId& remoteNode, const string& callsign)
                    : BaseClass(5, FactoryImpl::Instance(), callsign, _T("JSON"), "", "", false, false, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
                    , _parent(*parent)
                {
                }
                virtual ~ChannelImpl()
                {
                }
    
            public:
                virtual void Received(Core::ProxyType<INTERFACE>& jsonObject) override
                {
                    Core::ProxyType<Core::JSONRPC::Message> inbound(Core::proxy_cast<Core::JSONRPC::Message>(jsonObject));
    
                    ASSERT(inbound.IsValid() == true);
    
                    if (inbound.IsValid() == true) {
                        _parent.Inbound(inbound);
                    }
                }
                virtual void Send(Core::ProxyType<INTERFACE>& jsonObject) override
                {
                    #ifdef __DEBUG__
                    Core::ProxyType<Core::JSONRPC::Message> inbound(Core::proxy_cast<Core::JSONRPC::Message>(jsonObject));
    
                    ASSERT(inbound.IsValid() == true);
    
                    if (inbound.IsValid() == true) {
                        string message;
                        inbound->ToString(message);
                        TRACE_L1("Message: %s send", message.c_str());
                    }
                    #endif
                }
                virtual void StateChange() override
                {
                    _parent.StateChange();
                }
                virtual bool IsIdle() const
                {
                    return (true);
                }
    
            private:
                CommunicationChannel& _parent;
            };
            class ChannelProxy : public Core::ProxyObject<CommunicationChannel> {
            private:
                ChannelProxy(const ChannelProxy&) = delete;
                ChannelProxy& operator=(const ChannelProxy&) = delete;
                ChannelProxy() = delete;
    
                ChannelProxy(const Core::NodeId& remoteNode, const string& callsign)
                    : Core::ProxyObject<CommunicationChannel>(remoteNode, callsign)
                {
                }
    
                class Administrator {
                private:
                    Administrator(const Administrator&) = delete;
                    Administrator& operator=(const Administrator&) = delete;
    
                    typedef std::map<const string, CommunicationChannel *> CallsignMap;
    
                    static Administrator& Instance()
                    {    
                        static Administrator& _instance = Core::SingletonType<Administrator>::Instance();
                        return (_instance);
                    }
    
                public:
                    Administrator()
                        : _adminLock()
                        , _callsignMap()
                    {
                    }
                    ~Administrator()
                    {
                    }
    
                public:
                    static Core::ProxyType<CommunicationChannel> Instance(const Core::NodeId& remoteNode, const string& callsign)
                    {
                        return (Instance().InstanceImpl(remoteNode, callsign));
                    }
                    static uint32_t Release(ChannelProxy* object)
                    {
                        return (Instance().ReleaseImpl(object));
                    }
    
                private:
                    Core::ProxyType<CommunicationChannel> InstanceImpl(const Core::NodeId& remoteNode, const string& callsign)
                    {
                        Core::ProxyType<CommunicationChannel> result;
    
                        _adminLock.Lock();
    
                        string searchLine = remoteNode.HostName() + '@' + callsign;
    
                        typename CallsignMap::iterator index(_callsignMap.find(searchLine));
                        if (index != _callsignMap.end()) {
                            result = Core::ProxyType<CommunicationChannel>(*(index->second));
                        } else {
                            ChannelProxy* entry = new (0) ChannelProxy(remoteNode, callsign);
                            _callsignMap[searchLine] = entry;
                            result = Core::ProxyType<CommunicationChannel>(*entry);
                        }
                        _adminLock.Unlock();
    
                        ASSERT(result.IsValid() == true);
    
                        if (result.IsValid() == true) {
                            static_cast<ChannelProxy&>(*result).Open(100);
                        }
    
                        return (result);
                    }
                    uint32_t ReleaseImpl(ChannelProxy* object)
                    {
                        _adminLock.Lock();
    
                        uint32_t result = object->ActualRelease();
    
                        if (result == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                            // Oke remove the entry from the MAP.
    
                            typename CallsignMap::iterator index(_callsignMap.begin());
    
                            while ((index != _callsignMap.end()) && (&(*object) == index->second)) {
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
                };
    
            public:
                ~ChannelProxy()
                {
                    // Guess we need to close
                    CommunicationChannel::Close();
                }
    
                static Core::ProxyType< CommunicationChannel > Instance(const Core::NodeId& remoteNode, const string& callsign)
                {
                    return (Administrator::Instance(remoteNode, callsign));
                }
    
            public:
                virtual uint32_t Release() const override
                {
                    return (Administrator::Release(const_cast<ChannelProxy*>(this)));
                }
    
            private:
                uint32_t ActualRelease() const
                {
                    return (Core::ProxyObject<CommunicationChannel >::Release());
                }
                bool Open(const uint32_t waitTime)
                {
                    return (CommunicationChannel::Open(waitTime));
                }
    
            private:
                Core::CriticalSection _adminLock;
            };
    
        protected:
            CommunicationChannel(const Core::NodeId& remoteNode, const string& callsign)
                : _channel(this, remoteNode, callsign)
                , _sequence(0)
            {
            }
    
        public:
            virtual ~CommunicationChannel()
            {
            }
            static Core::ProxyType<CommunicationChannel> Instance(const Core::NodeId& remoteNode, const string& callsign)
            {
                return (ChannelProxy::Instance(remoteNode, callsign));
            }
    
        public:
            static void Trigger(const uint64_t& time, LinkType<INTERFACE>* client)
            {
                FactoryImpl::Instance().Trigger(time, client);
            }
            static Core::ProxyType<Core::JSONRPC::Message> Message()
            {
                return (FactoryImpl::Instance().Element(string()));
            }
            bool IsOperational() const
            {
                return (const_cast< CommunicationChannel&>(*this).Open(0));
            }
            uint32_t Sequence() const
            {
                return (++_sequence);
            }
            void Register(LinkType<INTERFACE>& client)
            {
                _adminLock.Lock();
                ASSERT(std::find(_observers.begin(), _observers.end(), &client) == _observers.end());
                _observers.push_back(&client);
                _adminLock.Unlock();
            }
            void Unregister(LinkType<INTERFACE>& client)
            {
                _adminLock.Lock();
                typename std::list<LinkType<INTERFACE> * >::iterator index(std::find(_observers.begin(), _observers.end(), &client));
                if (index != _observers.end()) {
                    _observers.erase(index);
                }
                FactoryImpl::Instance().Revoke(&client);
                _adminLock.Unlock();
            }
            void Submit(const Core::ProxyType<INTERFACE>& message)
            {
                _channel.Submit(message);
            }
    
        protected:
            void StateChange()
            {
                _adminLock.Lock();
                typename std::list<LinkType<INTERFACE> * >::iterator index(_observers.begin());
                while (index != _observers.end()) {
                    if (_channel.IsOpen() == true) {
                        (*index)->Opened();
                    } else {
                        (*index)->Closed();
                    }
                    index++;
                }
                _adminLock.Unlock();
            }
            bool Open(const uint32_t waitTime)
            {
                bool result = true;
                if (_channel.IsClosed() == true) {
                    result = (_channel.Open(waitTime) == Core::ERROR_NONE);
                }
                return (result);
            }
            void Close()
            {
                _channel.Close(Core::infinite);
            }
    
        private:
            uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;
                _adminLock.Lock();
                typename std::list<LinkType<INTERFACE> *>::iterator index(_observers.begin());
                while ((result != Core::ERROR_NONE) && (index != _observers.end())) {
                    result = (*index)->Inbound(inbound);
                    index++;
                }
                _adminLock.Unlock();
    
                return (result);
            }
    
        private:
            Core::CriticalSection _adminLock;
            ChannelImpl _channel;
            mutable std::atomic<uint32_t> _sequence;
            std::list< LinkType<INTERFACE> *> _observers;
        };
        class Entry {
        private:
            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;
            struct Synchronous {
                Synchronous()
                    : _signal(false, true)
                    , _response()
                {
                }
                Core::Event _signal;
                Core::ProxyType<Core::JSONRPC::Message> _response;
            };
            struct ASynchronous {
                ASynchronous(const uint32_t waitTime, const CallbackFunction& completed)
                    : _waitTime(Core::Time::Now().Add(waitTime).Ticks())
                    , _completed(completed)
                {
                }
                uint64_t _waitTime;
                CallbackFunction _completed;
            };

        public:
            Entry()
                : _synchronous(true)
                , _info()
            {
            }
            Entry(const uint32_t waitTime, const CallbackFunction& completed)
                : _synchronous(false)
                , _info(waitTime, completed)
            {
            }
            ~Entry()
            {
            }

        public:
            const Core::ProxyType<Core::JSONRPC::Message>& Response() const
            {
                return (_info.sync._response);
            }
            void Signal(const Core::ProxyType<Core::JSONRPC::Message>& response)
            {
                if (_synchronous == true) {
                    _info.sync._response = response;
                    _info.sync._signal.SetEvent();
                } else {
                    _info.async._completed(*response);
                }
            }
            const uint64_t& Expiry() const
            {
                return (_info.async._waitTime);
            }
            void Abort(const uint32_t id)
            {
                if (_synchronous == true) {
                    _info.sync._signal.SetEvent();
                } else {
                    Core::JSONRPC::Message message;
                    message.Id = id;
                    message.Error.Code = Core::ERROR_ASYNC_ABORTED;
                    message.Error.Text = _T("Pending call has been aborted");
                    _info.async._completed(message);
                }
            }
            bool Expired(const uint32_t id, const uint64_t& currentTime, uint64_t& nextTime)
            {
                bool expired = false;

                if (_synchronous == false) {
                    if (_info.async._waitTime > currentTime) {
                        if (_info.async._waitTime < nextTime) {
                            nextTime = _info.async._waitTime;
                        }
                    } else {
                        Core::JSONRPC::Message message;
                        message.Id = id;
                        message.Error.Code = Core::ERROR_TIMEDOUT;
                        message.Error.Text = _T("Pending a-sync call has timed out");
                        _info.async._completed(message);
                        expired = true;
                    }
                }
                return (expired);
            }
            bool WaitForResponse(const uint32_t waitTime)
            {
                return (_info.sync._signal.Lock(waitTime) == Core::ERROR_NONE);
            }

        private:
            bool _synchronous;
            union Info {
            public:
                Info()
                    : sync()
                {
                }
                Info(const uint32_t waitTime, const CallbackFunction& completed)
                    : async(waitTime, completed)
                {
                }
                ~Info()
                {
                }
                Synchronous sync;
                ASynchronous async;
            } _info;
        };
        static Core::NodeId RemoteNodeId()
        {
            Core::NodeId result;
            string remoteNodeId;
            if ((Core::SystemInfo::GetEnvironment(_T("THUNDER_ACCESS"), remoteNodeId) == true) && (remoteNodeId.empty() == false)) {
                result = Core::NodeId(remoteNodeId.c_str());
            }
            return (result);
        }
        static uint8_t DetermineVersion(const string& designator)
        {
            uint8_t version = Core::JSONRPC::Message::Version(designator);
            return (version == static_cast<uint8_t>(~0) ? 1 : version);
        }

        static constexpr uint32_t DefaultWaitTime = 10000;
        typedef std::map<uint32_t, Entry> PendingMap;
        typedef std::function<uint32_t(const string&, const string& parameters, string& result)> InvokeFunction;

    public:
        LinkType(const string& remoteCallsign, const TCHAR* localCallsign, const bool directed = false)
            : _adminLock()
            , _connectId(RemoteNodeId())
            , _channel(CommunicationChannel::Instance(_connectId, string("/jsonrpc/") + (directed && !remoteCallsign.empty() ? remoteCallsign : "Controller")))
            , _handler([&](const uint32_t, const string&, const string&) { }, { DetermineVersion(remoteCallsign) })
            , _callsign((!directed || remoteCallsign.empty()) ? remoteCallsign : "")
            , _localSpace(localCallsign)
            , _pendingQueue()
            , _scheduledTime(0)
        {
            _channel->Register(*this);
        }
        LinkType(const string& remoteCallsign, const uint8_t version, const bool directed = false)
            : _adminLock()
            , _connectId(RemoteNodeId())
            , _channel(CommunicationChannel::Instance(_connectId, string("/jsonrpc/") + (directed && !remoteCallsign.empty() ? remoteCallsign : "Controller")))
            , _handler([&](const uint32_t, const string&, const string&) {}, { version })
            , _callsign((!directed || remoteCallsign.empty()) ? remoteCallsign : "")
            , _localSpace()
            , _pendingQueue()
            , _scheduledTime(0)
        {
            static uint32_t sequence;

            uint32_t scope = Core::InterlockedIncrement(sequence);

            _localSpace = string("temporary") + Core::NumberType<uint32_t>(scope).Text();
            _channel->Register(*this);
        }
        virtual ~LinkType()
        {
            _channel->Unregister(*this);
        }

    public:
        template <typename INBOUND, typename METHOD>
        uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method)
        {
            std::function<void(const INBOUND& parameters)> actualMethod = method;
            InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string& result) -> uint32_t {
                INBOUND inbound;
                inbound.FromString(parameters);
                actualMethod(inbound);
                result.clear();
                return (Core::ERROR_NONE);
            };
            _handler.Register(eventName, implementation);
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send(waitTime, "register", parameters, response);

            if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        template <typename INBOUND, typename METHOD, typename REALOBJECT>
        uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
        {
            // using INBOUND = typename Core::TypeTraits::func_traits<METHOD>::template argument<0>::type;
            std::function<void(INBOUND parameters)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
            InvokeFunction implementation = [actualMethod](const string&, const string& parameters, string& result) -> uint32_t {
                INBOUND inbound;
                inbound.FromString(parameters);
                actualMethod(inbound);
                result.clear();
                return (Core::ERROR_NONE);
            };
            _handler.Register(eventName, implementation);
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send(waitTime, "register", parameters, response);

            if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        void Unsubscribe(const uint32_t waitTime, const string& eventName)
        {
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _localSpace + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            Send(waitTime, "unregister", parameters, response);

            _handler.Unregister(eventName);
        }

        template <typename PARAMETERS, typename RESPONSE>
        typename std::enable_if<(!std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
        Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
        {
            return InternalInvoke<PARAMETERS>(waitTime, method, parameters, inbound);
        }

        template <typename PARAMETERS, typename RESPONSE>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && std::is_same<RESPONSE, void>::value), uint32_t>::type
        Invoke(const uint32_t waitTime, const string& method)
        {
            return InternalInvoke<string>(waitTime, method, EMPTY_STRING);
        }

        template <typename PARAMETERS, typename RESPONSE>
        typename std::enable_if<(!std::is_same<PARAMETERS, void>::value && std::is_same<RESPONSE, void>::value), uint32_t>::type
        Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters)
        {
            return InternalInvoke<PARAMETERS>(waitTime, method, parameters);
        }

        template <typename PARAMETERS, typename RESPONSE>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<RESPONSE, void>::value), uint32_t>::type
        Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
        {
            return InternalInvoke<string>(waitTime, method, EMPTY_STRING, inbound);
        }

        template <typename PARAMETERS, typename HANDLER>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<string, HANDLER>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                EMPTY_STRING,
                callback));
        }
        template <typename PARAMETERS, typename HANDLER>
        inline typename std::enable_if<!(std::is_same<PARAMETERS, void>::value && std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<PARAMETERS, HANDLER>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                parameters,
                callback));
        }
        template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<string, HANDLER, REALOBJECT>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                EMPTY_STRING,
                callback,
                objectPtr));
        }
        template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
        inline typename std::enable_if<!(std::is_same<PARAMETERS, void>::value && !std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<PARAMETERS, HANDLER, REALOBJECT>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                parameters,
                callback,
                objectPtr));
        }
        template <typename PARAMETERS, typename... TYPES>
        uint32_t Set(const uint32_t waitTime, const string& method, const TYPES&&... args)
        {
            PARAMETERS sendObject(args...);
            return (Set<PARAMETERS>(waitTime, method, sendObject));
        }
        template <typename PARAMETERS>
        uint32_t Set(const uint32_t waitTime, const string& method, const string& index, const PARAMETERS& sendObject)
        {
            string fullMethod = method + '@' + index;
            return (Set<PARAMETERS>(waitTime, fullMethod, sendObject));
        }
        template <typename PARAMETERS, typename NUMBER>
        uint32_t Set(const uint32_t waitTime, const string& method, const NUMBER index, const PARAMETERS& sendObject)
        {
            string fullMethod = method + '@' + Core::NumberType<NUMBER>(index).Text();
            return (Set<PARAMETERS>(waitTime, fullMethod, sendObject));
        }
        template <typename PARAMETERS>
        uint32_t Set(const uint32_t waitTime, const string& method, const PARAMETERS& sendObject)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            uint32_t result = Send(waitTime, method, sendObject, response);
            if ((result == Core::ERROR_NONE) && (response->Error.IsSet() == true)) {
                result = response->Error.Code.Value();
            }
            return (result);
        }
        template <typename PARAMETERS>
        uint32_t Get(const uint32_t waitTime, const string& method, const string& index, PARAMETERS& sendObject)
        {
            string fullMethod = method + '@' + index;
            return (Get<PARAMETERS>(waitTime, fullMethod, sendObject));
        }
        template <typename PARAMETERS, typename NUMBER>
        uint32_t Get(const uint32_t waitTime, const string& method, const NUMBER& index, PARAMETERS& sendObject)
        {
            string fullMethod = method + '@' + Core::NumberType<NUMBER>(index).Text();
            return (Get<PARAMETERS>(waitTime, fullMethod, sendObject));
        }
        template <typename PARAMETERS>
        uint32_t Get(const uint32_t waitTime, const string& method, PARAMETERS& sendObject)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            uint32_t result = Send(waitTime, method, EMPTY_STRING, response);
            if (result == Core::ERROR_NONE) {
                if (response->Error.IsSet() == true) {
                    result = response->Error.Code.Value();
                } else if ((response->Result.IsSet() == true) && (response->Result.Value().empty() == false)) {
                    sendObject.Clear();
                    //sendObject.FromString(response->Result.Value());
                }
            }
            return (result);
        }
        uint32_t Invoke(const uint32_t waitTime, const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            return (Send(waitTime, method, parameters, response));
        }

        // Generic JSONRPC methods.
        // Anything goes!
        // these objects have no type chacking, will consume more memory and processing takes more time
        // Advice: Use string typed variants above!!!
        // =====================================================================================================
        uint32_t Invoke(const char method[], const Core::JSON::VariantContainer& parameters, Core::JSON::VariantContainer& response, const uint32_t waitTime = DefaultWaitTime)
        {
            return (Invoke<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(waitTime, method, parameters, response));
        }
        uint32_t SetProperty(const char method[], const Core::JSON::VariantContainer& object, const uint32_t waitTime = DefaultWaitTime)
        {
            return (Set<Core::JSON::VariantContainer>(waitTime, method, object));
        }
        uint32_t GetProperty(const char method[], Core::JSON::VariantContainer& object, const uint32_t waitTime = DefaultWaitTime)
        {
            return (Get<Core::JSON::VariantContainer>(waitTime, method, object));
        }
        template <typename RESPONSE = Core::JSON::VariantContainer>
        DEPRECATED uint32_t Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
        // Note: use of Invoke without indicating both Parameters and Response type is deprecated -> replace this one by Invoke<void, ResponeType>(..
        {
            return InternalInvoke<string>(waitTime, method, EMPTY_STRING, inbound);
        }
        //template <typename PARAMETERS = Core::JSON::VariantContainer, typename RESPONSE = Core::JSON::VariantContainer>
        template <typename PARAMETERS = Core::JSON::VariantContainer>
        DEPRECATED uint32_t Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, Core::JSON::VariantContainer& inbound)
        // Note: use of Invoke without indicating both Parameters and Response type is deprecated -> replace this one by Invoke<PARAMETER type, Core::JSON::VariantContainer>(..
        {
            return InternalInvoke<PARAMETERS>(waitTime, method, parameters, inbound);
        }

    private:
        friend CommunicationChannel;

        uint64_t Timed()
        {
            uint64_t result = ~0;
            uint64_t currentTime = Core::Time::Now().Ticks();

            // Lets see if some callback are expire. If so trigger and remove...
            _adminLock.Lock();

            typename PendingMap::iterator index = _pendingQueue.begin();

            while (index != _pendingQueue.end()) {

                if (index->second.Expired(index->first, currentTime, result) == true) {
                    index = _pendingQueue.erase(index);
                } else {
                    index++;
                }
            }
            _scheduledTime = (result != static_cast<uint64_t>(~0) ? result : 0);

            _adminLock.Unlock();

            return (_scheduledTime);
        }
        template <typename PARAMETERS, typename RESPONSE>
        uint32_t InternalInvoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            uint32_t result = Send(waitTime, method, parameters, response);
            if (result == Core::ERROR_NONE) {
                inbound.FromString(response->Result.Value());
            }
            return (result);
        }
        template <typename PARAMETERS, typename RESPONSE>
        uint32_t InternalInvoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            return Send(waitTime, method, parameters, response);
        }
        template <typename PARAMETERS, typename HANDLER>
        uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            CallbackFunction implementation = [callback](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;
                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                }
                callback(response);
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        template <typename PARAMETERS, typename HANDLER>
        uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            CallbackFunction implementation = [callback](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;

                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                    callback(response, nullptr);
                } else {
                    callback(response, &(response.Error));
                }
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        template <typename PARAMETERS, typename HANDLER, typename REALOBJECT>
        uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            std::function<void(RESPONSE)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1);
            CallbackFunction implementation = [actualMethod](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;

                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                }

                actualMethod(response);
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        template <typename PARAMETERS, typename HANDLER, typename REALOBJECT>
        uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const PARAMETERS& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            std::function<void(RESPONSE, const Core::JSONRPC::Message::Info* result)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1, std::placeholders::_2);
            CallbackFunction implementation = [actualMethod](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const<typename std::remove_reference<RESPONSE>::type>::type response;
                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                    actualMethod(response, nullptr);
                } else {
                    actualMethod(response, &(inbound.Error));
                }
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        void Opened()
        {
            // Nice to know :-)
        }
        void Closed()
        {
            // Abort any in progress RPC command:
            _adminLock.Lock();

            // See if we issued anything, if so abort it..
            while (_pendingQueue.size() != 0) {

                _pendingQueue.begin()->second.Abort(_pendingQueue.begin()->first);
                _pendingQueue.erase(_pendingQueue.begin());
            }

            _adminLock.Unlock();
        }
        template <typename PARAMETERS>
        uint32_t Send(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == true) {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::JSONRPC::Message> message(CommunicationChannel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Designator = _callsign + '.' + method;
                } else {
                    message->Designator = method;
                }
                ToMessage(parameters, message);

                _adminLock.Lock();

                typename std::pair< typename PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple());
                ASSERT(newElement.second == true);

                if (newElement.second == true) {

                    Entry& slot(newElement.first->second);

                    _adminLock.Unlock();

                    _channel->Submit(Core::ProxyType<INTERFACE>(message));

                    message.Release();

                    if (slot.WaitForResponse(waitTime) == true) {
                        response = slot.Response();

                        // See if we have a response, maybe it was just the connection
                        // that closed?
                        if (response.IsValid() == true) {
                            result = Core::ERROR_NONE;
                        }
                    }

                    _adminLock.Lock();

                    _pendingQueue.erase(id);
                }

                _adminLock.Unlock();
            }

            return (result);
        }

        template <typename PARAMETERS> 
        uint32_t Send(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, CallbackFunction& response)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == false) {
            } else {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::JSONRPC::Message> message(CommunicationChannel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Designator = _callsign + '.' + method;
                } else {
                    message->Designator = method;
                }
                ToMessage(parameters, message);

                _adminLock.Lock();

                typename std::pair<typename PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(waitTime, response));
                ASSERT(newElement.second == true);

                if (newElement.second == true) {

                    _channel->Submit(Core::ProxyType<INTERFACE>(message));

                    result = Core::ERROR_NONE;

                    message.Release();
                    if ((_scheduledTime == 0) || (_scheduledTime > newElement.first->second.Expiry())) {
                        _scheduledTime = newElement.first->second.Expiry();
                        CommunicationChannel::Trigger(_scheduledTime, this);
                    }
                }

                _adminLock.Unlock();
            }

            return (result);
        }
        uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound)
        {
            uint32_t result = Core::ERROR_INVALID_SIGNATURE;

            ASSERT(inbound.IsValid() == true);

            if ((inbound->Id.IsSet() == true) && (inbound->Result.IsSet() || inbound->Error.IsSet())) {
                // Looks like this is a response..
                ASSERT(inbound->Parameters.IsSet() == false);
                ASSERT(inbound->Designator.IsSet() == false);

                _adminLock.Lock();

                // See if we issued this..
                typename PendingMap::iterator index = _pendingQueue.find(inbound->Id.Value());

                if (index != _pendingQueue.end()) {

                    index->second.Signal(inbound);

                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();
            } else {
                // check if we understand this message (correct callsign?)
                string callsign(inbound->FullCallsign());

                if (callsign == _localSpace) {
                    // Looks like this is an event.
                    ASSERT(inbound->Id.IsSet() == false);

                    string response;
                    _handler.Invoke(Core::JSONRPC::Connection(~0, ~0), inbound->FullMethod(), inbound->Parameters.Value(), response);
                }
            }

            return (result);
        }

    private:
        void ToMessage(Core::JSON::IMessagePack& parameters, Core::ProxyType<Core::JSONRPC::Message>& message)
        {
             std::vector<uint8_t> values;
             parameters.ToBuffer(values);
             if (values.empty() != true) {
                 message->Parameters = string(values.begin(), values.end());;
             }
             return;
        }
        void ToMessage(Core::JSON::IElement& parameters, Core::ProxyType<Core::JSONRPC::Message>& message)
        {
             string values;
             parameters.ToString(values);
             if (values.empty() != true) {
                 message->Parameters = values;
             }
             return;
        }
        void ToMessage(string& parameters, Core::ProxyType<Core::JSONRPC::Message>& message)
        {
             if (parameters.empty() != true) {
                 message->Parameters = parameters;
             }
        }
        void FromMessage(Core::JSON::IElement& response, const Core::ProxyType<Core::JSONRPC::Message>& message)
        {
            response.FromString(message.Parameters.Value());
        }
        void FromMessage(Core::JSON::IMessagePack& response, const Core::ProxyType<Core::JSONRPC::Message>& message)
        {
//            response.FromBuffer(message);
        }
    private:
        Core::CriticalSection _adminLock;
        Core::NodeId _connectId;
        Core::ProxyType< CommunicationChannel > _channel;
        Core::JSONRPC::Handler _handler;
        string _callsign;
        string _localSpace;
        PendingMap _pendingQueue;
        uint64_t _scheduledTime;
    };
}
} // namespace WPEFramework::JSONRPC
