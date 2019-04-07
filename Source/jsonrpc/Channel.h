#pragma once

#include "Module.h"

namespace WPEFramework {

namespace JSONRPC {

    class Client;

    using namespace Core::TypeTraits;

    class EXTERNAL Channel {
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
                WatchDog(Client* client)
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
                uint64_t Timed(const uint64_t scheduledTime);

            private:
                Client* _client;
            };

            FactoryImpl()
                : _jsonRPCFactory(2)
                , _watchDog(Core::Thread::DefaultStackSize(), _T("JSONRPCCleaner"))
            {
            }

        public:
            static FactoryImpl& Instance();
            ~FactoryImpl()
            {
            }

        public:
            Core::ProxyType<Core::JSONRPC::Message> Element(const string&)
            {
                return (_jsonRPCFactory.Element());
            }
            void Trigger(const uint64_t& time, Client* client)
            {
                _watchDog.Trigger(time, client);
            }
            void Revoke(Client* client)
            {
                _watchDog.Revoke(client);
            }

        private:
            Core::ProxyPoolType<Core::JSONRPC::Message> _jsonRPCFactory;
            Core::TimerType<WatchDog> _watchDog;
        };

        class ChannelImpl : public Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&> {
        private:
            ChannelImpl(const ChannelImpl&) = delete;
            ChannelImpl& operator=(const ChannelImpl&) = delete;

            typedef Core::StreamJSONType<Web::WebSocketClientType<Core::SocketStream>, FactoryImpl&> BaseClass;

        public:
            ChannelImpl(Channel& parent, const Core::NodeId& remoteNode, const string& callsign)
                : BaseClass(5, FactoryImpl::Instance(), callsign, _T("JSON"), "", "", false, false, false, remoteNode.AnyInterface(), remoteNode, 256, 256)
                , _parent(parent)
            {
            }
            virtual ~ChannelImpl()
            {
            }

        public:
            virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject) override
            {
                Core::ProxyType<Core::JSONRPC::Message> inbound(Core::proxy_cast<Core::JSONRPC::Message>(jsonObject));

                ASSERT(inbound.IsValid() == true);

                if (inbound.IsValid() == true) {
                    _parent.Inbound(inbound);
                }
            }
            virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject) override
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
            Channel& _parent;
        };

    protected:
        Channel(const Core::NodeId& remoteNode, const string& callsign)
            : _channel(*this, remoteNode, callsign)
            , _sequence(0)
        {
        }

    public:
        virtual ~Channel()
        {
        }
        static Core::ProxyType<Channel> Instance(const Core::NodeId& remoteNode, const string& callsign);

    public:
        static void Trigger(const uint64_t& time, Client* client)
        {
            FactoryImpl::Instance().Trigger(time, client);
        }
        static Core::ProxyType<Core::JSONRPC::Message> Message()
        {
            return (FactoryImpl::Instance().Element(string()));
        }
        bool IsOperational() const
        {
            return (const_cast<Channel&>(*this).Open(0));
        }
        uint32_t Sequence() const
        {
            return (++_sequence);
        }
        void Register(Client& client)
        {
            _adminLock.Lock();
            ASSERT(std::find(_observers.begin(), _observers.end(), &client) == _observers.end());
            _observers.push_back(&client);
            _adminLock.Unlock();
        }
        void Unregister(Client& client)
        {
            _adminLock.Lock();
            std::list<Client*>::iterator index(std::find(_observers.begin(), _observers.end(), &client));
            if (index == _observers.end()) {
                _observers.erase(index);
            }
            FactoryImpl::Instance().Revoke(&client);
            _adminLock.Unlock();
        }
        void Submit(const Core::ProxyType<Core::JSON::IElement>& message)
        {
            _channel.Submit(message);
        }

    protected:
        void StateChange();
        bool Open(const uint32_t waitTime)
        {
            bool result = true;
            if (_channel.IsOpen() == false) {
                result = (_channel.Open(waitTime) == Core::ERROR_NONE);
            }
            return (result);
        }
        void Close()
        {
            _channel.Close(Core::infinite);
        }

    private:
        uint32_t Inbound(const Core::ProxyType<Core::JSONRPC::Message>& inbound);

    private:
        Core::CriticalSection _adminLock;
        ChannelImpl _channel;
        mutable std::atomic<uint32_t> _sequence;
        std::list<Client*> _observers;
    };

    class EXTERNAL Client {
    private:
        Client() = delete;
        Client(const Client&) = delete;
        Client& operator=(Client&) = delete;

        typedef std::function<void(const Core::JSONRPC::Message&)> CallbackFunction;

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

        static constexpr uint32_t DefaultWaitTime = 10000;
        typedef std::map<uint32_t, Entry> PendingMap;
        typedef std::function<uint32_t(const string& parameters, string& result)> InvokeFunction;

    public:
        Client(const string& remoteCallsign, const TCHAR* localCallsign = nullptr, const bool directed = false)
            : _adminLock()
            , _connectId(RemoteNodeId())
            , _channel(Channel::Instance(_connectId, string("/jsonrpc/") + (directed && !remoteCallsign.empty() ? remoteCallsign : "Controller")))
            , _handler()
            , _callsign((!directed || remoteCallsign.empty()) ? remoteCallsign : "")
            , _pendingQueue()
            , _scheduledTime(0)
        {
            static uint32_t sequence;

            std::vector<uint8_t> versions;
            Core::InterlockedIncrement(sequence);

            if (localCallsign == nullptr) {
                versions.push_back(1);
                _handler.Designator("temporary" + Core::NumberType<uint32_t>(sequence).Text(), versions);
            } else {
                string designator(localCallsign);
                size_t pos = designator.find_last_of('.');
                if (pos != string::npos) {
                    string number = designator.substr(pos + 1);
                    designator = designator.substr(0, pos);
                    if ((number.length() > 0) && (std::all_of(number.begin(), number.end(), [](TCHAR c) { return std::isdigit(c); }))) {
                        versions.push_back(static_cast<uint8_t>(atoi(number.c_str())));
                    }
                }
                _handler.Designator(designator, versions);
            }
            _channel->Register(*this);
        }
        virtual ~Client()
        {
            _channel->Unregister(*this);
        }

    public:
        uint64_t Timed()
        {
            uint64_t result = ~0;
            uint64_t currentTime = Core::Time::Now().Ticks();

            // Lets see if some callback are expire. If so trigger and remove...
            _adminLock.Lock();

            PendingMap::iterator index = _pendingQueue.begin();

            while (index != _pendingQueue.end()) {

                if (index->second.Expired(index->first, currentTime, result) == true) {
                    _pendingQueue.erase(index);
                } else {
                    index++;
                }
            }
            _scheduledTime = (result != static_cast<uint64_t>(~0) ? result : 0);

            _adminLock.Unlock();

            return (_scheduledTime);
        }
        const string& Callsign() const
        {
            return (_callsign);
        }
        void Callsign(const string& callsign)
        {
            _callsign = callsign;
        }
        template <typename INBOUND, typename METHOD>
        uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method)
        {
            // @PierreWielders: zie Inbound hieronder: waarom ook niet de INBOUND uit te METHOD halen? (technische reden?)
            std::function<void(const INBOUND& parameters)> actualMethod = method;
            InvokeFunction implementation = [actualMethod](const string& parameters, string& result) -> uint32_t {
                INBOUND inbound;
                inbound = parameters;
                actualMethod(inbound);
                result.clear();
                return (Core::ERROR_NONE);
            };
            _handler.Register(eventName, implementation);
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _handler.Callsign() + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send(waitTime, "register", parameters, response);

            if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        template <typename METHOD, typename REALOBJECT>
        uint32_t Subscribe(const uint32_t waitTime, const string& eventName, const METHOD& method, REALOBJECT* objectPtr)
        {
            using INBOUND = typename Core::TypeTraits::func_traits<METHOD>::template argument<0>::type;
            std::function<void(const INBOUND& parameters)> actualMethod = std::bind(method, objectPtr, std::placeholders::_1);
            InvokeFunction implementation = [actualMethod](const string& parameters, string& result) -> uint32_t {
                INBOUND inbound;
                inbound = parameters;
                actualMethod(inbound);
                result.clear();
                return (Core::ERROR_NONE);
            };
            _handler.Register(eventName, implementation);
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _handler.Callsign() + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            uint32_t result = Send(waitTime, "register", parameters, response);

            if ((result != Core::ERROR_NONE) || (response.IsValid() == false) || (response->Error.IsSet() == true)) {
                _handler.Unregister(eventName);
            }

            return (result);
        }
        void Unsubscribe(const uint32_t waitTime, const string& eventName)
        {
            const string parameters("{ \"event\": \"" + eventName + "\", \"id\": \"" + _handler.Callsign() + "\"}");
            Core::ProxyType<Core::JSONRPC::Message> response;

            Send(waitTime, "unregister", parameters, response);

            _handler.Unregister(eventName);
        }

        // Opaque JSON structure methods.
        // Anything goes!
        // ===================================================================================
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

        // Specific JSONRPC methods.
        // Less memory footprint, less processing power and type checking applied.
        // =====================================================================================================
        template <typename RESPONSE = Core::JSON::VariantContainer>
        uint32_t Invoke(const uint32_t waitTime, const string& method, RESPONSE& inbound)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            uint32_t result = Send(waitTime, method, EMPTY_STRING, response);
            if (result == Core::ERROR_NONE) {
                inbound.FromString(response->Result.Value());
            }
            return (result);
        }
        template <typename PARAMETERS = Core::JSON::VariantContainer, typename RESPONSE = Core::JSON::VariantContainer>
        uint32_t Invoke(const uint32_t waitTime, const string& method, const PARAMETERS& parameters, RESPONSE& inbound)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            string subject;
            parameters.ToString(subject);
            uint32_t result = Send(waitTime, method, subject, response);
            if (result == Core::ERROR_NONE) {
                inbound.FromString(response->Result.Value());
            }
            return (result);
        }
        template <typename PARAMETERS, typename HANDLER>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<HANDLER>(
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

            string subject;
            parameters.ToString(subject);

            return (InternalInvoke<HANDLER>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                subject,
                callback));
        }
        template <typename PARAMETERS, typename HANDLER, typename REALOBJECT = typename Core::TypeTraits::func_traits<HANDLER>::classtype>
        typename std::enable_if<(std::is_same<PARAMETERS, void>::value && !std::is_same<typename Core::TypeTraits::func_traits<HANDLER>::classtype, void>::value), uint32_t>::type
        Dispatch(const uint32_t waitTime, const string& method, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using ERRORCODE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<1>::type;

            return (InternalInvoke<HANDLER, REALOBJECT>(
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

            string subject;
            parameters.ToString(subject);

            return (InternalInvoke<HANDLER, REALOBJECT>(
                ::TemplateIntToType<std::is_same<ERRORCODE, Core::JSONRPC::Error*>::value>(),
                waitTime,
                method,
                subject,
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
        uint32_t Set(const uint32_t waitTime, const string& method, const PARAMETERS& sendObject)
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
            string subject;
            sendObject.ToString(subject);
            uint32_t result = Send(waitTime, method, subject, response);
            if ((result == Core::ERROR_NONE) && (response->Error.IsSet() == true)) {
                result = response->Error.Code.Value();
            }
            return (result);
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
                    sendObject.FromString(response->Result.Value());
                }
            }
            return (result);
        }
        uint32_t Invoke(const uint32_t waitTime, const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            return (Send(waitTime, method, parameters, response));
        }

    private:
        friend class Channel;

        template <typename HANDLER>
        uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const string& parameters, const HANDLER& callback)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            CallbackFunction implementation = [callback](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const< typename std::remove_reference<RESPONSE>::type>::type response;
                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                }
                callback(response);
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        template <typename HANDLER>
        uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const string& parameters, const HANDLER& callback)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            CallbackFunction implementation = [callback](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const< typename std::remove_reference<RESPONSE>::type>::type response;

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
        template <typename HANDLER, typename REALOBJECT>
        uint32_t InternalInvoke(const ::TemplateIntToType<1>&, const uint32_t waitTime, const string& method, const string& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            std::function<void(RESPONSE)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1);
            CallbackFunction implementation = [actualMethod](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const< typename std::remove_reference<RESPONSE>::type>::type response;

                if (inbound.Error.IsSet() == false) {
                    response.FromString(inbound.Result.Value());
                }

                actualMethod(response);
            };

            uint32_t result = Send(waitTime, method, parameters, implementation);
            return (result);
        }
        template <typename HANDLER, typename REALOBJECT>
        uint32_t InternalInvoke(const ::TemplateIntToType<0>&, const uint32_t waitTime, const string& method, const string& parameters, const HANDLER& callback, REALOBJECT* objectPtr)
        {
            using RESPONSE = typename Core::TypeTraits::func_traits<HANDLER>::template argument<0>::type;

            std::function<void(RESPONSE, const Core::JSONRPC::Message::Info* result)> actualMethod = std::bind(callback, objectPtr, std::placeholders::_1, std::placeholders::_2);
            CallbackFunction implementation = [actualMethod](const Core::JSONRPC::Message& inbound) -> void {
                typename std::remove_const < typename std::remove_reference < RESPONSE > ::type >::type response;
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
        uint32_t Send(const uint32_t waitTime, const string& method, const string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == true) {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::JSONRPC::Message> message(Channel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Designator = _callsign + '.' + method;
                } else {
                    message->Designator = method;
                }
                if (parameters.empty() == false) {
                    message->Parameters = parameters;
                }

                _adminLock.Lock();

                std::pair<PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple());
                ASSERT(newElement.second == true);

                if (newElement.second == true) {

                    Entry& slot(newElement.first->second);

                    _adminLock.Unlock();

                    _channel->Submit(message);

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
        uint32_t Send(const uint32_t waitTime, const string& method, const string& parameters, CallbackFunction& response)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_channel.IsValid() == false) {
            } else {

                result = Core::ERROR_ASYNC_FAILED;

                Core::ProxyType<Core::JSONRPC::Message> message(Channel::Message());
                uint32_t id = _channel->Sequence();
                message->Id = id;
                if (_callsign.empty() == false) {
                    message->Designator = _callsign + '.' + method;
                } else {
                    message->Designator = method;
                }
                if (parameters.empty() == false) {
                    message->Parameters = parameters;
                }

                _adminLock.Lock();

                std::pair<PendingMap::iterator, bool> newElement = _pendingQueue.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id),
                    std::forward_as_tuple(waitTime, response));
                ASSERT(newElement.second == true);

                if (newElement.second == true) {

                    _channel->Submit(message);

                    message.Release();
                    if ((_scheduledTime == 0) || (_scheduledTime > newElement.first->second.Expiry())) {
                        _scheduledTime = newElement.first->second.Expiry();
                        Channel::Trigger(_scheduledTime, this);
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
                PendingMap::iterator index = _pendingQueue.find(inbound->Id.Value());

                if (index != _pendingQueue.end()) {

                    index->second.Signal(inbound);

                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();
            } else {
                // check if we understand this message (correct callsign?)
                uint32_t result(_handler.Validate(*inbound));

                if (result == Core::ERROR_NONE) {
                    // Looks like this is an event.
                    ASSERT(inbound->Id.IsSet() == false);

                    string response;
                    _handler.Invoke(Core::JSONRPC::Connection(~0, ~0), inbound->Method(), inbound->Parameters.Value(), response);
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
        uint64_t _scheduledTime;
    };
}
} // namespace WPEFramework::JSONRPC
