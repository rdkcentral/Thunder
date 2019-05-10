#pragma once

#include "IShell.h"
#include "Module.h"

namespace WPEFramework {

namespace PluginHost {

    struct EXTERNAL IDispatcher : public virtual Core::IUnknown {
        virtual ~IDispatcher() {}

        enum { ID = RPC::ID_DISPATCHER };

        virtual Core::ProxyType<Core::JSONRPC::Message> Invoke(const uint32_t channelId, const Core::JSONRPC::Message& message) = 0;

        // Methods used directly by the Framework to handle MetaData requirements.
        // There should be no need to call these methods from the implementation directly.
        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;
        virtual void Closed(const uint32_t channelId) = 0;
        virtual uint32_t Exists(const string& method, const uint8_t version) const = 0;
    };

    class EXTERNAL JSONRPC : public IDispatcher {
    private:
        JSONRPC(const JSONRPC&) = delete;
        JSONRPC& operator=(const JSONRPC&) = delete;

    protected:

        class Registration : public Core::JSON::Container {
        private:
            Registration(const Registration&) = delete;
            Registration& operator=(const Registration&) = delete;

        public:
            Registration()
                : Core::JSON::Container()
                , Event()
                , Callsign()
            {
                Add(_T("event"), &Event);
                Add(_T("id"), &Callsign);
            }
            ~Registration()
            {
            }

        public:
            Core::JSON::String Event;
            Core::JSON::String Callsign;
        };

	private:

        class Observer {
        private:
            Observer(const Observer&) = delete;
            Observer& operator=(const Observer&) = delete;

        public:
            Observer(const uint32_t id, const string& designator)
                : _id(id)
                , _designator(designator)
            {
            }
            ~Observer()
            {
            }

            bool operator==(const Observer& rhs) const
            {
                return ((rhs._id == _id) && (rhs._designator == _designator));
            }
            bool operator!=(const Observer& rhs) const
            {
                return (!operator==(rhs));
            }

            uint32_t Id() const
            {
                return (_id);
            }
            const string& Designator() const
            {
                return (_designator);
            }

        private:
            uint32_t _id;
            string _designator;
        };

        typedef std::list<Observer> ObserverList;
        typedef std::map<string, ObserverList> ObserverMap;
		
    public:
        JSONRPC()
            : _adminLock()
            , _handler( { 1 } )
            , _observers()
            , _service(nullptr)
        {
        }
        JSONRPC(const std::vector<uint8_t> versions)
            : _adminLock()
            , _handler(versions)
            , _observers()
            , _service(nullptr)
        {
        }
        virtual ~JSONRPC()
        {
        }

    public:
        Core::ProxyType<Core::JSONRPC::Message> Message() const
        {
            return (_jsonRPCMessageFactory.Element());
        }
        uint32_t Validate(const Core::JSONRPC::Message& inbound) const
        {
            return (_handler.Validate(inbound));
        }
        template <typename PARAMETER, typename GET_METHOD, typename SET_METHOD, typename REALOBJECT>
        void Property(const string& methodName, const GET_METHOD& getter, const SET_METHOD& setter, REALOBJECT* objectPtr)
        {
            _handler.Property<PARAMETER, GET_METHOD, SET_METHOD, REALOBJECT>(methodName, getter, setter, objectPtr);
        }
        template <typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handler.Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handler.Register<INBOUND, OUTBOUND, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename OUTBOUND, typename METHOD>
        void Register(const string& methodName, const METHOD& method)
        {
            _handler.Register<INBOUND, OUTBOUND, METHOD>(methodName, method);
        }
        template <typename INBOUND, typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handler.Register<INBOUND, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename INBOUND, typename METHOD>
        void Register(const string& methodName, const METHOD& method)
        {
            _handler.Register<INBOUND, METHOD>(methodName, method);
        }
        void Unregister(const string& methodName)
        {
            _handler.Unregister(methodName);
        }
        template <typename JSONOBJECT>
        uint32_t Response(const Core::JSONRPC::Connection& channel, const JSONOBJECT& parameters)
        {
            string subject;
            parameters.ToString(subject);
            return (Response(channel, subject));
        }
        uint32_t Notify(const string& event)
        {
            return (NotifyImpl(event, _T("")));
        }
        template <typename JSONOBJECT>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters)
        {
            string subject;
            parameters.ToString(subject);
            return (NotifyImpl(event, subject));
        }
        template <typename JSONOBJECT, typename SENDIFMETHOD>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters, SENDIFMETHOD method)
        {
            string subject;
            parameters.ToString(subject);
            return NotifyImpl(event, subject, std::move(method));
        }

    private:
        uint32_t NotifyImpl(const string& event, const string& parameters, std::function<bool(const string&)>&& sendifmethod = std::function<bool(const string&)>())
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();

                result = Core::ERROR_NONE;

                while (loop != clients.end()) {
                    const string& designator(loop->Designator());

                    if (!sendifmethod || sendifmethod(designator)) {

                        Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCMessageFactory.Element();

                        ASSERT(_service != nullptr);

                        if (!parameters.empty()) {
                            message->Parameters = parameters;
                        }

                        message->Designator = (designator.empty() == false ? designator + '.' + event : event);
                        message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

                        _service->Submit(loop->Id(), message);
                    }

                    loop++;
                }
            }

            _adminLock.Unlock();

            return (result);
        }

    public:
        uint32_t Response(const Core::JSONRPC::Connection& channel, const string& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCMessageFactory.Element();

            ASSERT(_service != nullptr);

            message->Result = result;
            message->Id = channel.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel.ChannelId(), message));
        }
        uint32_t Response(const Core::JSONRPC::Connection& channel, const Core::JSONRPC::Error& result)
        {
            Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCMessageFactory.Element();

            ASSERT(_service != nullptr);

            message->Error = result;
            message->Id = channel.Sequence();
            message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

            return (_service->Submit(channel.ChannelId(), message));
        }
        virtual uint32_t Exists(const string& method, const uint8_t version) const override
        {
            return (_handler.Exists(method, version));
        }

    private:
        void Register(const uint32_t id, const string& parameters, Core::JSONRPC::Message& response)
        {
            Registration info;
            info.FromString(parameters);
            Register(id, info, response);
        }

        void Unregister(const uint32_t id, const string& parameters, Core::JSONRPC::Message& response)
        {
            Registration info;
            info.FromString(parameters);
            Unregister(id, info, response);
        }

	protected:
        void Register(const uint32_t id, const Registration& info, Core::JSONRPC::Message& response)
        {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(info.Event.Value());

            if (index == _observers.end()) {
                _observers[info.Event.Value()].emplace_back(id, info.Callsign.Value());
                response.Result = _T("0");
            } else if (std::find(index->second.begin(), index->second.end(), Observer(id, info.Callsign.Value())) == index->second.end()) {
                index->second.emplace_back(id, info.Callsign.Value());
                response.Result = _T("0");
            } else {
                response.Error.SetError(Core::ERROR_DUPLICATE_KEY);
                response.Error.Text = _T("Duplicate registration. Only 1 remains!!!");
            }

            _adminLock.Unlock();
        }

        void Unregister(const uint32_t id, const Registration& info, Core::JSONRPC::Message& response)
        {
            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(info.Event.Value());

            if (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();
                Observer key(id, info.Callsign.Value());

                while ((loop != clients.end()) && (*loop != key)) {
                    loop++;
                }

                if (loop != clients.end()) {
                    clients.erase(loop);
                    if (clients.empty() == true) {
                        _observers.erase(index);
                    }
                    response.Result = _T("0");
                }
            }

            if (response.Result.IsSet() == false) {
                response.Error.SetError(Core::ERROR_UNKNOWN_KEY);
                response.Error.Text = _T("Registration not found!!!");
            }

            _adminLock.Unlock();
        }

    private:
        virtual void Activate(IShell * service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _service = service;

            _handler.Designator(_service->Callsign());
        }
        virtual void Deactivate() override
        {
            _service = nullptr;
            _observers.clear();
        }
        virtual void Closed(const uint32_t id) override
        {

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.begin();

            while (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();

                while (loop != clients.end()) {
                    if (loop->Id() != id) {
                        loop++;
                    } else {
                        loop = clients.erase(loop);
                    }
                }
                if (clients.empty() == true) {
                    index = _observers.erase(index);
                } else {
                    index++;
                }
            }

            _adminLock.Unlock();
        }

    protected:
        inline bool Invoke(const uint32_t channelId, const Core::JSONRPC::Message& inbound, string& method, string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            bool handled = true;
            uint32_t result = _handler.Validate(inbound);

            if (inbound.Id.IsSet() == true) {
                response = Message();
                response->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
                response->Id = inbound.Id.Value();
            }

            if (result != Core::ERROR_NONE) {
                response->Error.SetError(result);
                response->Error.Text = _T("Destined invoke failed.");
            } else {
                method = inbound.Method();
                parameters = inbound.Parameters.Value();

                if (_handler.Exists(Core::JSONRPC::Message::Method(method), Core::JSONRPC::Message::Version(method)) == Core::ERROR_NONE) {
                    string result;
                    uint32_t code = _handler.Invoke(Core::JSONRPC::Connection(channelId, inbound.Id.Value()), method, parameters, result);
                    if (response.IsValid() == true) {
                        if (code == static_cast<uint32_t>(~0)) {
                            response.Release();
                        } else if (code == Core::ERROR_NONE) {
                            response->Result = result;
                        } else {
                            response->Error.Code = code;
                            response->Error.Text = Core::ErrorToString(code);
                        }
                    }
                } else {
                    handled = false;
                }
            }

            return handled;
        }

        inline bool IsRegisterMethod(const string& methodname) const
        {
            return methodname == _T("register");
        }

        inline bool IsUnregisterMethod(const string& methodname) const
        {
            return methodname == _T("unregister");
        }

        inline bool IsExistsMethod(const string& methodname) const
        {
            return methodname == _T("exists");
        }

        inline void InvokeInternalMethods(const uint32_t channelId, const Core::JSONRPC::Message& inbound, string& method, string& parameters, Core::ProxyType<Core::JSONRPC::Message>& response)
        {
            if (IsExistsMethod(method) == true) {

                ASSERT(response.IsValid() == true);

                if (response.IsValid() == true) {
                    if (_handler.Exists(Core::JSONRPC::Message::Method(parameters), Core::JSONRPC::Message::Version(parameters)) == Core::ERROR_NONE) {
                        response->Result = Core::NumberType<uint32_t>(Core::ERROR_NONE).Text();
                    } else {
                        response->Result = Core::NumberType<uint32_t>(Core::ERROR_UNKNOWN_KEY).Text();
                    }
                }
            } else if (IsRegisterMethod(method) == true) {

                ASSERT(response.IsValid() == true);

                if (response.IsValid() == true) {

                    Register(channelId, inbound.Parameters.Value(), *response);
                }
            } else if (IsUnregisterMethod(method) == true) {

                ASSERT(response.IsValid() == true);

                if (response.IsValid() == true) {

                    Unregister(channelId, inbound.Parameters.Value(), *response);
                }
            } else if (response.IsValid() == true) {
                response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                response->Error.Text = _T("Unhandled method.");
            }
        }

        virtual Core::ProxyType<Core::JSONRPC::Message> Invoke(const uint32_t channelId, const Core::JSONRPC::Message& inbound) override
        {
            string method;
            string parameters;
            Core::ProxyType<Core::JSONRPC::Message> response;

            if (Invoke(channelId, inbound, method, parameters, response) == false) {
                InvokeInternalMethods(channelId, inbound, method, parameters, response);
            }

            return response;
        }

    private:
        Core::CriticalSection _adminLock;
        Core::JSONRPC::Handler _handler;
        ObserverMap _observers;
        IShell* _service;

        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> _jsonRPCMessageFactory;
    };

    class EXTERNAL JSONRPCSupportsEventStatus : public JSONRPC {
    public:
        JSONRPCSupportsEventStatus(const JSONRPCSupportsEventStatus&) = delete;
        JSONRPCSupportsEventStatus& operator=(const JSONRPCSupportsEventStatus&) = delete;

        JSONRPCSupportsEventStatus() = default;
        virtual ~JSONRPCSupportsEventStatus() = default;

		using JSONRPC::Invoke;
		using JSONRPC::Notify;

        enum class Status { registered,
            unregistered };

        template <typename METHOD>
        void RegisterEventStatusListener(const string& event, METHOD method)
        {
            _adminLock.Lock();

            ASSERT(_observers.find(event) == _observers.end());

            _observers[event] = method;

            _adminLock.Unlock();
        }

        void UnregisterEventStatusListener(const string& event)
        {
            _adminLock.Lock();

            ASSERT(_observers.find(event) != _observers.end());

            _observers.erase(event);

            _adminLock.Unlock();
        }

    protected:

        void NotifyObservers(const string& event, const string& client, const Status status) const
        {
            _adminLock.Lock();

            StatusCallbackMap::const_iterator it = _observers.find(event);
            if (it != _observers.cend()) {
                it->second(client, status);
			}

            _adminLock.Unlock();
        }

        virtual Core::ProxyType<Core::JSONRPC::Message> Invoke(const uint32_t channelId, const Core::JSONRPC::Message& inbound) override
        {
            string method;
            string parameters;
            Core::ProxyType<Core::JSONRPC::Message> response;

            if (Invoke(channelId, inbound, method, parameters, response) == false) {
                if (IsRegisterMethod(method) == true) {
                    Registration info;
                    info.FromString(parameters);
                    Register(channelId, info, *response);
                    NotifyObservers(info.Event.Value(), info.Callsign.Value(), Status::registered);
                } else if (IsUnregisterMethod(method) == true) {
                    Registration info;
                    info.FromString(parameters);
                    NotifyObservers(info.Event.Value(), info.Callsign.Value(), Status::unregistered);
                    Unregister(channelId, info, *response);
                } else {
					InvokeInternalMethods(channelId, inbound, method, parameters, response);
                }
            }

            return response;
        }

    private:
        using EventStatusCallback = std::function<void(const string&, Status status)>;
        using StatusCallbackMap = std::map<string, EventStatusCallback>;

        mutable Core::CriticalSection _adminLock;
        StatusCallbackMap _observers;
    };
 } // namespace WPEFramework::PluginHost
 }
