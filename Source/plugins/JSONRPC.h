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
            , _handler()
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
        template <typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT* objectPtr)
        {
            _handler.Register<INBOUND, OUTBOUND, METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        template <typename METHOD, typename REALOBJECT>
        void Register(const string& methodName, const METHOD& method, REALOBJECT objectPtr)
        {
            _handler.Register<METHOD, REALOBJECT>(methodName, method, objectPtr);
        }
        void Unregister(const string& methodName)
        {
            _handler.Unregister(methodName);
        }
        template <typename JSONOBJECT>
        uint32_t Notify(const string& event, const JSONOBJECT& parameters)
        {
            string subject;
            parameters.ToString(subject);
            return (Notify(event, subject));
        }
        uint32_t Notify(const string& event, const string& parameters)
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
                    Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCMessageFactory.Element();

                    ASSERT(_service != nullptr);

                    message->Parameters = parameters;
                    message->Designator = (designator.empty() == false ? designator + '.' + event : event);
                    message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;

                    _service->Submit(loop->Id(), message);
                    loop++;
                }
            }

            _adminLock.Unlock();

            return (result);
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
        void Unregister(const uint32_t id, const string& parameters, Core::JSONRPC::Message& response)
        {
            Registration info;
            info.FromString(parameters);

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
        virtual void Activate(IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);

            _service = service;
            std::vector<uint8_t> data;

            // Extract the version list from the config
            Core::JSON::ArrayType<Core::JSON::DecUInt8> versions;
            versions.FromString(service->Versions());
            Core::JSON::ArrayType<Core::JSON::DecUInt8>::Iterator index(versions.Elements());

            while (index.Next() == true) {
                data.push_back(index.Current().Value());
            }

            // If no versions are give, lets assume this is version 1, and we support it :-)
            if (data.empty() == true) {
                data.push_back(1);
            }

            _handler.Designator(_service->Callsign(), data);
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
        virtual Core::ProxyType<Core::JSONRPC::Message> Invoke(const uint32_t channelId, const Core::JSONRPC::Message& inbound) override
        {
            Core::ProxyType<Core::JSONRPC::Message> response;
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
                const string method = inbound.Method();
                const string parameters = inbound.Parameters.Value();

                if (_handler.Exists(Core::JSONRPC::Message::Method(method), Core::JSONRPC::Message::Version(method)) == Core::ERROR_NONE) {
                    string result;
                    uint32_t code = _handler.Invoke(method, parameters, result);
                    if (response.IsValid() == true) {
                        if (code == Core::ERROR_NONE) {
                            response->Result = result;
                        } else {
                            response->Error.SetError(code);
                            response->Error.Text = result;
                        }
                    }
                } else if (method == _T("exists")) {

                    ASSERT(response.IsValid() == true);

                    if (response.IsValid() == true) {
                        if (_handler.Exists(Core::JSONRPC::Message::Method(parameters), Core::JSONRPC::Message::Version(parameters)) == Core::ERROR_NONE) {
                            response->Result = _T("0");
                        } else {
                            response->Result = _T("22"); // ERROR_UNKNOWN_KEY
                        }
                    }
                } else if (method == _T("register")) {

                    ASSERT(response.IsValid() == true);

                    if (response.IsValid() == true) {

                        Register(channelId, inbound.Parameters.Value(), *response);
                    }
                } else if (method == _T("unregister")) {

                    ASSERT(response.IsValid() == true);

                    if (response.IsValid() == true) {

                        Unregister(channelId, inbound.Parameters.Value(), *response);
                    }
                } else {
                    response->Error.SetError(Core::ERROR_UNKNOWN_KEY);
                    response->Error.Text = _T("Unhandled method.");
                }
            }
            return (response);
        }

    private:
        Core::CriticalSection _adminLock;
        Core::JSONRPC::Handler _handler;
        ObserverMap _observers;
        IShell* _service;

        static Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> _jsonRPCMessageFactory;
    };
}
} // namespace WPEFramework::PluginHost
