#ifndef __COM_IUNKNOWN_H
#define __COM_IUNKNOWN_H

#include "Module.h"
#include "Messages.h"
#include "Administrator.h"

namespace WPEFramework {
namespace ProxyStub {
    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    typedef void (*MethodHandler)(Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message);

    class EXTERNAL UnknownStub { 
    private: 
        UnknownStub(const UnknownStub&) = delete; 
        UnknownStub& operator=(const UnknownStub&) = delete; 
 
    public: 
        UnknownStub(); 
        virtual ~UnknownStub(); 
 
    public: 
        inline uint16_t Length() const 
        { 
            return (3); 
        } 
        virtual Core::IUnknown* Convert(void* incomingData) const; 
        virtual void Handle(const uint16_t index, Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message); 
    }; 

    template <typename INTERFACE, MethodHandler METHODS[]>
    class UnknownStubType : public UnknownStub {
    public:
        typedef INTERFACE* CLASS_INTERFACE;

    private:
        UnknownStubType(const UnknownStubType<INTERFACE, METHODS>&);
        UnknownStubType<INTERFACE, METHODS>& operator=(const UnknownStubType<INTERFACE, METHODS>&);

    public:
        UnknownStubType()
        {
            _myHandlerCount = 0;

            while (METHODS[_myHandlerCount] != nullptr) {
                _myHandlerCount++;
            }
        }
        virtual ~UnknownStubType()
        {
        }

    public:
        inline uint16_t Length() const
        {
            return (_myHandlerCount + UnknownStub::Length());
        }
        virtual Core::IUnknown* Convert(void* incomingData) const
        {
		Core::IUnknown* iuptr = reinterpret_cast<Core::IUnknown*>(incomingData);
			
				
		INTERFACE * result = dynamic_cast<INTERFACE*>(iuptr);
			
		if (result == nullptr) {
		
			result = reinterpret_cast<INTERFACE*>(incomingData);
		}
			
		return (result);
	}
        virtual void Handle(const uint16_t index, Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message)
        {
            uint16_t baseNumber(UnknownStub::Length());

            if (index < baseNumber) {
                UnknownStub::Handle(index, channel, message);
            }
            else if ((index - baseNumber) < _myHandlerCount) {
                MethodHandler handler(METHODS[index - baseNumber]);

                ASSERT(handler != nullptr);

                handler(channel, message);
            }
        }

    private:
        uint16_t _myHandlerCount;
    };

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------

    class UnknownProxy {
    private:
        UnknownProxy(const UnknownProxy&) = delete;
        UnknownProxy& operator=(const UnknownProxy&) = delete;

        enum registration {
            UNREGISTERED,
            PENDING_ADDREF,
            REGISTERED,
            PENDING_RELEASE
        };

    public:
        UnknownProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const uint32_t interfaceId, const bool remoteRefCounted, Core::IUnknown* parent)
            : _remoteAddRef(remoteRefCounted ? REGISTERED : UNREGISTERED)
            , _refCount(remoteRefCounted ? 2 : 0)
            , _interfaceId(interfaceId)
            , _implementation(implementation)
            , _channel(channel)
            , _parent(*parent)
        {
        }
        virtual ~UnknownProxy()
        {
        }

    public:
        inline Core::ProxyType<RPC::InvokeMessage> Message(const uint8_t methodId) const
        {
            Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

            message->Parameters().Set(_implementation, _interfaceId, methodId + 3);

            return (message);
        }
        inline uint32_t Invoke(Core::ProxyType<RPC::InvokeMessage>& message, const uint32_t waitTime = RPC::CommunicationTimeOut) const
        {
            ASSERT(_channel.IsValid() == true);

            uint32_t result = _channel->Invoke(message, waitTime);

            if (result != Core::ERROR_NONE) {
                // Oops something failed on the communication. Report it.
                TRACE_L1("IPC method invokation failed for 0x%X", message->Parameters().InterfaceId());
                TRACE_L1("IPC method invoke failed with error %d", result);
            }

            return (result);
        }
	void AddRef() const
        {
            if (Core::InterlockedIncrement(_refCount) == 2) {

                ASSERT (_remoteAddRef == UNREGISTERED);

                registration value (UNREGISTERED);

                // Seems we really would like to "preserve" this interface, so report it in use
                if (_remoteAddRef.compare_exchange_weak(value, PENDING_ADDREF, std::memory_order_release, std::memory_order_relaxed) == true) {
                    RPC::Administrator::Instance().RegisterProxy(const_cast<UnknownProxy&>(*this));
                }
            }
        }
        uint32_t Release() const {
            return(_parent.Release());
        }
        uint32_t DropReference() const
        {
            uint32_t result(Core::ERROR_NONE);
            uint32_t newValue = Core::InterlockedDecrement(_refCount);

            if (newValue == 0) {
                ASSERT (_remoteAddRef == UNREGISTERED);

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }
            else if (newValue == 1) {

                ASSERT (_remoteAddRef == REGISTERED);

                // Request for destruction, do this on antoher thread, as this is an Proxy that has been addRefed on
                // the other size, it needs to Release the AddRef on the other side and we do not want to issue 
                // the release on a possible invoke request.
                registration value (REGISTERED);
                if (_remoteAddRef.compare_exchange_weak(value, PENDING_RELEASE, std::memory_order_release, std::memory_order_relaxed) == true) {
                    RPC::Administrator::Instance().UnregisterProxy(const_cast<UnknownProxy&>(*this));
                }
            }
            return (result);
        }
        bool ShouldAddRefRemotely() {
            registration value (PENDING_ADDREF);
            return (_remoteAddRef.compare_exchange_weak(value, REGISTERED, std::memory_order_release, std::memory_order_relaxed) == true);
        }
        bool ShouldReleaseRemotely() {
            registration value (PENDING_RELEASE);
            return (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == true);
        }
        uint32_t Destroy() {
            registration value (REGISTERED);
            
            if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == true) {
                RPC::Administrator::Instance().UnregisterProxy(*this);
            }

            /// Next Release should destruct this. We are to be destructed!!!
            _refCount = 1;
            return (_parent.Release());
        }

/*
        uint32_t RemoteRelease () {

            // We have reached "0", signal the other side..
            Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

            message->Parameters().Set(_implementation, _interfaceId, 1);

            // Just try the destruction for few Seconds...
	    uint32_t result = Invoke(message, RPC::CommunicationTimeOut);
					
            if ( (result != Core::ERROR_NONE) && (result != Core::ERROR_CONNECTION_CLOSED) ) {
                result = Core::ERROR_DESTRUCTION_FAILED;
            }

            return (result);
        }
*/

        inline const Core::ProxyType<Core::IPCChannel>& Channel() const {
            return (_channel);
        }
        inline void* Implementation () const {
            return (_implementation);
        }
        inline uint32_t InterfaceId () const {
            return (_interfaceId);
        }
        inline void* QueryInterface(const uint32_t id)
        {
            return (_parent.QueryInterface(id));
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* QueryInterface()
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(QueryInterface(ACTUAL_INTERFACE::ID)));
        }
 
    private:
        mutable std::atomic<registration> _remoteAddRef;
        mutable uint32_t _refCount;
        const uint32_t _interfaceId;
        void* _implementation;
        mutable Core::ProxyType<Core::IPCChannel> _channel;
        Core::IUnknown& _parent;
    };

    template <typename INTERFACE>
    class UnknownProxyType : public INTERFACE  {
    private:
        UnknownProxyType(const UnknownProxyType<INTERFACE>&) = delete;
        UnknownProxyType<INTERFACE>& operator=(const UnknownProxyType<INTERFACE>&) = delete;

    public:
        typedef UnknownProxyType<INTERFACE> BaseClass;
        typedef Core::ProxyType<RPC::InvokeMessage> IPCMessage;

    public:
        UnknownProxyType(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool remoteRefCounted)
            : _unknown(channel, implementation, INTERFACE::ID, remoteRefCounted, this) {
        }
        virtual ~UnknownProxyType()
        {
        }

    public:
        inline UnknownProxy* Administration () {
            return (&_unknown);
        }
        inline IPCMessage Message(const uint8_t methodId) const {
            return (_unknown.Message(methodId));
        }
        inline uint32_t Invoke(Core::ProxyType<RPC::InvokeMessage>& message, const uint32_t waitTime = RPC::CommunicationTimeOut) const {
            return (_unknown.Invoke(message, waitTime));
        }
	virtual void AddRef() const override {
            _unknown.AddRef();
        }
        virtual uint32_t Release() const override {
            uint32_t result = Core::ERROR_NONE;

            if (_unknown.DropReference() == Core::ERROR_DESTRUCTION_SUCCEEDED) {
                delete (this);
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }
            return (result);
        }
        virtual void* QueryInterface(const uint32_t interfaceNumber) override
        {
            void* result = nullptr;

            if (interfaceNumber == INTERFACE::ID ) {
                // Just AddRef and return..
                AddRef();
                result = static_cast<INTERFACE*>(this);
            }
            else if (interfaceNumber == Core::IUnknown::ID ) {
                // Just AddRef and return..
                AddRef();
                result = static_cast<Core::IUnknown*>(this);
            }
            else {
                Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());
                RPC::Data::Frame::Writer parameters(message->Parameters().Writer());

                message->Parameters().Set(_unknown.Implementation(), INTERFACE::ID, 2);
                parameters.Number<uint32_t>(interfaceNumber);
                if (_unknown.Invoke(message, RPC::CommunicationTimeOut) == Core::ERROR_NONE) {
                    RPC::Data::Frame::Reader response(message->Response().Reader());

                    // From what is returned, we need to create a proxy
                    result = RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), response.Number<void*>(), interfaceNumber, true, interfaceNumber);
                }
            }

            return (result);
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* QueryInterface()
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(QueryInterface(ACTUAL_INTERFACE::ID)));
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* ProxyInstance(void* implementation)
        {
            return (RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), implementation, ACTUAL_INTERFACE::ID, true, ACTUAL_INTERFACE::ID));
	}
        inline void* ProxyInstance(void* implementation, const uint32_t id)
        {
            return (RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), implementation, id, true, id));
	}
        template <typename RESULTOBJECT>
        inline RESULTOBJECT Number(const RPC::Data::Output& response) const {
            RPC::Data::Frame::Reader reader (response.Reader());
            RESULTOBJECT result = reader.Number<RESULTOBJECT>();
            Complete(reader);
            return (result);
        }
        inline bool Boolean(const RPC::Data::Output& response) const {
            RPC::Data::Frame::Reader reader (response.Reader());
            bool result = reader.Boolean();
            Complete(reader);
            return (result);
        }
        inline string Text (const RPC::Data::Output& response) const {
            RPC::Data::Frame::Reader reader (response.Reader());
            string result = reader.Text();
            Complete(reader);
            return (result);
        }
        inline void* Interface(const RPC::Data::Output& response, const uint32_t interfaceId) const {
            RPC::Data::Frame::Reader reader(response.Reader());

            void* result = RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), reader.Number<void*>(), interfaceId, true, interfaceId);

            Complete(reader);

            return (result);
        }
        inline void Complete(const RPC::Data::Output& response) const {
            if (response.Length() > 0) {
                RPC::Data::Frame::Reader reader (response.Reader());
                Complete(reader);
            }
        }
        inline void Complete (RPC::Data::Frame::Reader& reader) const {
            
            while (reader.HasData() == true) {
                ASSERT(reader.Length() >= (sizeof(void*) + sizeof(uint32_t)));
                void* impl = reader.Number<void*>();
                uint32_t id = reader.Number<uint32_t>();
                if ((id & 0x80000000) == 0) {
                    // Just AddRef this implementation
                    RPC::Administrator::Instance().AddRef(impl, id);
                }
                else {
                    // Just Release this implementation
                    id = id ^ 0x80000000;
                    RPC::Administrator::Instance().Release(impl, id);
                }
            }
        }
 
    private:
        UnknownProxy _unknown;
    };
}
} // namespace ProxyStub

#endif //  __COM_IUNKNOWN_H
