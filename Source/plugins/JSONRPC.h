#pragma once

#include "Module.h"
#include "IShell.h"

namespace WPEFramework {

namespace PluginHost {

    struct IDispatcher : public virtual Core::IUnknown {
        virtual ~IDispatcher() {}

        enum { ID = RPC::ID_DISPATCHER };

        virtual void Activate(IShell* service) = 0;
        virtual void Deactivate() = 0;

        virtual uint32_t Invoke (const uint32_t id, const string& methodName, const string& parameters, string& result) = 0;
        virtual uint32_t Notify (const string& event, const string& parameters) = 0;

        virtual void Closed(const uint32_t id) = 0;
    };

    class EXTERNAL JSONRPC : public IDispatcher {
    private:
        JSONRPC(const JSONRPC&) = delete;
        JSONRPC& operator= (const JSONRPC&) = delete;

        struct ThreadLocalStorageId {
            uint32_t _id;
        };

        class Observer {
        private:
            Observer(const Observer&) = delete;
            Observer& operator=(const Observer&) = delete;

        public:
            Observer(const uint32_t id, const string& designator) 
                : _id(id)
                , _designator(designator) {
            }
            ~Observer() {
            }

            bool operator== (const Observer& rhs) const {
                return ((rhs._id == _id) && (rhs._designator == _designator));
            }
            bool operator!= (const Observer& rhs) const {
                return (!operator== (rhs));
            }

            uint32_t Id() const {
                return (_id);
            }
            const string& Designator() const {
                return (_designator);
            }

        private:
            uint32_t _id;
            string _designator;
        };

        typedef std::list<Observer> ObserverList;
        typedef std::map<string, ObserverList > ObserverMap;

    public:
        JSONRPC() 
            : _adminLock()
            , _handler() 
            , _observers()
            , _service(nullptr) {
        }
        virtual ~JSONRPC() {
        }

    public:
        template<typename INBOUND, typename OUTBOUND, typename METHOD, typename REALOBJECT>
        void Register (const string& methodName, const METHOD& method, REALOBJECT* objectPtr) {
            _handler.Register<INBOUND,OUTBOUND,METHOD,REALOBJECT>(methodName, method, objectPtr);
        }
        template<typename METHOD, typename REALOBJECT>
        void Register (const string& methodName, const METHOD& method, REALOBJECT objectPtr ) {
            _handler.Register<METHOD,REALOBJECT>(methodName, method, objectPtr);
        }
        void Unregister (const string& methodName) {
            _handler.Unregister(methodName);
        }
        uint32_t Validate  (const Core::JSONRPC::Message& message) const {
            return (_handler.Validate(message));
        }
        uint32_t Invoke (const uint32_t ID, const Core::JSONRPC::Message& inbound, string& response) {
            // Store the Channel Id on the thread local storage, might be used during registrations...            
            Core::Thread::GetContext<ThreadLocalStorageId>()._id = ID;

            return (_handler.Invoke(inbound, response));
        }
        virtual void Activate(IShell* service) override {
            ASSERT (_service == nullptr);
            ASSERT (service != nullptr);

            _service = service;
            std::vector<uint8_t> data;

            // Extract the version list from the config
            Core::JSON::ArrayType<Core::JSON::DecUInt8> versions; versions.FromString(service->Versions());
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
        virtual void Deactivate() override {
            _service = nullptr;
            _observers.clear();
        }
        virtual uint32_t Invoke (const uint32_t id, const string& method, const string& parameters, string& response) override {
            return (_handler.Invoke(id,method,parameters,response));
        }
        virtual uint32_t Notify (const string& event, const string& parameters) override {
            uint32_t result = Core::ERROR_UNKNOWN_KEY_PASSED;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();

                result = Core::ERROR_NONE;

                while (loop != clients.end()) {
                    const string& designator (loop->Designator());
                    Core::ProxyType<Web::JSONBodyType< Core::JSONRPC::Message > > message = _jsonRPCMessageFactory.Element();

                    ASSERT (_service != nullptr);

                    message->Parameters = parameters;
                    message->Designator = (designator.empty() == false ? designator + '.' + event : event);
                    message->Version = Core::JSONRPC::Message::DefaultVersion;

                    _service->Submit(loop->Id(), message);
                    loop++;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        uint32_t Register (const string& event, const string& designator) {
            uint32_t result = Core::ERROR_NONE;
            uint32_t id = Core::Thread::GetContext<ThreadLocalStorageId>()._id;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index == _observers.end()) {
                _observers[event].emplace_back(id, designator);
            }
            else if (std::find(index->second.begin(), index->second.end(), Observer(id, designator)) == index->second.end()) {
                index->second.emplace_back(id, designator);
            }
            else {
                result = Core::ERROR_DUPLICATE_KEY;
            }

            _adminLock.Unlock();

            return (result);
        }
        uint32_t Unregister (const string& event, const string& designator) {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;
            uint32_t id = Core::Thread::GetContext<ThreadLocalStorageId>()._id;

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.find(event);

            if (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();
                Observer key(id, designator);

                while ((loop != clients.end()) && (*loop != key)) { loop++; }

                if (loop != clients.end()) {
                    clients.erase(loop);
                    if (clients.empty() == true) {
                        _observers.erase(index);
                    }
                    result = Core::ERROR_NONE;
                }
            }

            _adminLock.Unlock();

            return (result);
 
        }
        virtual void Closed(const uint32_t id) override {

            _adminLock.Lock();

            ObserverMap::iterator index = _observers.begin();

            while (index != _observers.end()) {
                ObserverList& clients = index->second;
                ObserverList::iterator loop = clients.begin();

                while (loop != clients.end()) {
                    if (loop->Id() != id) {
                        loop++;
                    }
                    else {
                        loop = clients.erase(loop);
                        
                   }
                }
                if (clients.empty() == true) {
                    index = _observers.erase(index);
                }
                else {
                    index++;
                }
            }

            _adminLock.Unlock();
        }
   
    private:
        Core::CriticalSection _adminLock;
        Core::JSONRPC::Handler _handler;
        ObserverMap _observers;
        IShell* _service;

        static Core::ProxyPoolType<Web::JSONBodyType< Core::JSONRPC::Message > > _jsonRPCMessageFactory;
    };



} } // Namespace WPEFramework::PluginHost

