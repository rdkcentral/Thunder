#ifndef __COM_IUNKNOWN_H
#define __COM_IUNKNOWN_H

#include "Administrator.h"
#include "Messages.h"
#include "Module.h"

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
        UnknownStubType(const UnknownStubType<INTERFACE, METHODS>&) = delete;
        UnknownStubType<INTERFACE, METHODS>& operator=(const UnknownStubType<INTERFACE, METHODS>&) = delete;

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
            INTERFACE* result = reinterpret_cast<INTERFACE*>(incomingData);

            //if (result == nullptr) {

            //	result = reinterpret_cast<INTERFACE*>(incomingData);
            //}

            return (result);
        }
        virtual void Handle(const uint16_t index, Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message)
        {
            uint16_t baseNumber(UnknownStub::Length());

            if (index < baseNumber) {
                UnknownStub::Handle(index, channel, message);
            } else if ((index - baseNumber) < _myHandlerCount) {
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

        enum registration : uint8_t {
            UNREGISTERED = 0x01,
            PENDING_ADDREF = 0x02,
            REGISTERED = 0x04,
            PENDING_RELEASE = 0x08,
            CACHING = 0x10
        };

    public:
        UnknownProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const uint32_t interfaceId, const bool remoteRefCounted, Core::IUnknown* parent)
            : _remoteAddRef(remoteRefCounted ? REGISTERED : UNREGISTERED)
            , _refCount(remoteRefCounted ? 1 : 0)
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
        inline uint32_t Release()
        {
            return (_parent.Release());
        }
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
        void EnableCaching()
        {
            uint8_t value(UNREGISTERED);

            // Seems we really would like to "preserve" this interface, so report it in use
            if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED | CACHING, std::memory_order_release, std::memory_order_relaxed) == false) {
                value = REGISTERED;
                if (_remoteAddRef.compare_exchange_weak(value, REGISTERED | CACHING, std::memory_order_release, std::memory_order_relaxed) == false) {
                    TRACE_L1("Could not turn on the caching flag... Whaaatt !!!, %d", _remoteAddRef.load());
                }
            }
        }
        // Adding a reference can only be in a cached way if we reach the "registered" level
        void AddReference() const
        {
            uint32_t newValue = Core::InterlockedIncrement(_refCount);

            // By definition we can only reach the AddRef of 2
            if (newValue == 2) {

                uint8_t value(UNREGISTERED | CACHING);

                // Seems we really would like to "preserve" this interface, so report it in use
                if (_remoteAddRef.compare_exchange_weak(value, PENDING_ADDREF | CACHING, std::memory_order_release, std::memory_order_relaxed) == true) {
                    // INcrement for the registration..
                    RPC::Administrator::Instance().RegisterProxy(const_cast<UnknownProxy&>(*this));
                }
            }
        }
        uint32_t DropReference() const
        {
            uint32_t result(Core::ERROR_NONE);
            uint32_t newValue = Core::InterlockedDecrement(_refCount);

            if (newValue == 0) {
                ASSERT(_remoteAddRef == UNREGISTERED);

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            } else if (newValue == 1) {
                uint8_t value(REGISTERED);
                if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == true) {
                    RPC::Administrator::Instance().UnregisterProxy(const_cast<UnknownProxy&>(*this));
                    RemoteRelease();
                }
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            } else if (newValue == 2) {
                uint8_t value = REGISTERED | CACHING;
                if (_remoteAddRef.compare_exchange_weak(value, PENDING_RELEASE | CACHING, std::memory_order_release, std::memory_order_relaxed) == true) {
                    RPC::Administrator::Instance().UnregisterProxy(const_cast<UnknownProxy&>(*this));
                } else {
                    value = (PENDING_ADDREF | CACHING);
                    if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED | CACHING, std::memory_order_release, std::memory_order_relaxed) == true) {

                        RPC::Administrator::Instance().UnregisterProxy(const_cast<UnknownProxy&>(*this));
                    }
                }
            }
            return (result);
        }
        bool ShouldAddRefRemotely()
        {
            uint8_t value(CACHING | PENDING_ADDREF);
            return (_remoteAddRef.compare_exchange_weak(value, REGISTERED, std::memory_order_release, std::memory_order_relaxed) == true);
        }
        bool ShouldReleaseRemotely()
        {
            uint8_t value(CACHING | PENDING_RELEASE);
            return (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == true);
        }
        void ClearCache()
        {
            uint8_t value(CACHING | REGISTERED);
            if (_remoteAddRef.compare_exchange_weak(value, REGISTERED, std::memory_order_release, std::memory_order_relaxed) == false) {
                value = CACHING | UNREGISTERED;
                if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == false) {
                    TRACE_L1("Interesting, can not clear the CACHING FLAG. Current state: %d", _remoteAddRef.load());
                }
            }
        }
        // This method is used to forcefully clear out the Proxies for channels that have unexpectedly closed !!!
        uint32_t Destroy()
        {
            uint8_t value(REGISTERED);

            if (_remoteAddRef.compare_exchange_weak(value, UNREGISTERED, std::memory_order_release, std::memory_order_relaxed) == true) {
                RPC::Administrator::Instance().UnregisterProxy(*this);
            }

            /// Next Release should destruct this. We are to be destructed!!!
            _refCount = 1;
            return (_parent.Release());
        }
        void RemoteRelease() const
        {

            // We have reached "0", signal the other side..
            Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

            message->Parameters().Set(_implementation, _interfaceId, 1);

            // Just try the destruction for few Seconds...
            uint32_t result = Invoke(message, RPC::CommunicationTimeOut);

            if (result != Core::ERROR_NONE) {
                TRACE_L1("Could not remote release the Proxy.");
            }
        }

        inline const Core::ProxyType<Core::IPCChannel>& Channel() const
        {
            return (_channel);
        }
        inline void* Implementation() const
        {
            return (_implementation);
        }
        inline uint32_t InterfaceId() const
        {
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
        friend class RPC::Administrator;
        mutable std::atomic<uint8_t> _remoteAddRef;
        mutable uint32_t _refCount;
        const uint32_t _interfaceId;
        void* _implementation;
        mutable Core::ProxyType<Core::IPCChannel> _channel;
        Core::IUnknown& _parent;
    };

    template <typename INTERFACE>
    class UnknownProxyType : public INTERFACE {
    private:
        UnknownProxyType(const UnknownProxyType<INTERFACE>&) = delete;
        UnknownProxyType<INTERFACE>& operator=(const UnknownProxyType<INTERFACE>&) = delete;

    public:
        typedef UnknownProxyType<INTERFACE> BaseClass;
        typedef Core::ProxyType<RPC::InvokeMessage> IPCMessage;

    public:
        UnknownProxyType(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool remoteRefCounted)
            : _unknown(channel, implementation, INTERFACE::ID, remoteRefCounted, this)
        {
        }
        virtual ~UnknownProxyType()
        {
        }

    public:
        inline UnknownProxy* Administration()
        {
            return (&_unknown);
        }
        inline IPCMessage Message(const uint8_t methodId) const
        {
            return (_unknown.Message(methodId));
        }
        inline uint32_t Invoke(Core::ProxyType<RPC::InvokeMessage>& message, const uint32_t waitTime = RPC::CommunicationTimeOut) const
        {
            return (_unknown.Invoke(message, waitTime));
        }
        virtual void AddRef() const override
        {
            _unknown.AddReference();
        }
        virtual uint32_t Release() const override
        {
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

            if (interfaceNumber == INTERFACE::ID) {
                // Just AddRef and return..
                AddRef();
                result = static_cast<INTERFACE*>(this);
            } else if (interfaceNumber == Core::IUnknown::ID) {
                // Just AddRef and return..
                AddRef();
                result = static_cast<Core::IUnknown*>(this);
            } else {
                Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());
                RPC::Data::Frame::Writer parameters(message->Parameters().Writer());

                message->Parameters().Set(_unknown.Implementation(), INTERFACE::ID, 2);
                parameters.Number<uint32_t>(interfaceNumber);
                if (_unknown.Invoke(message, RPC::CommunicationTimeOut) == Core::ERROR_NONE) {
                    RPC::Data::Frame::Reader response(message->Response().Reader());

                    // From what is returned, we need to create a proxy
                    ProxyStub::UnknownProxy* instance = RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), response.Number<void*>(), interfaceNumber, true, interfaceNumber, false);
                    result = (instance != nullptr ? instance->QueryInterface(interfaceNumber) : nullptr);
                }
            }

            return (result);
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* QueryInterface()
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(QueryInterface(ACTUAL_INTERFACE::ID)));
        }
        inline void* Interface(void* implementation, const uint32_t interfaceId) const
        {
            void* result = nullptr;

            // From what is returned, we need to create a proxy
            ProxyStub::UnknownProxy* instance = RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(), implementation, interfaceId, true, interfaceId, false);
            result = (instance != nullptr ? instance->QueryInterface(interfaceId) : nullptr);

            return (result);
        }
        inline void Complete(RPC::Data::Frame::Reader& reader) const
        {

            while (reader.HasData() == true) {
                ASSERT(reader.Length() >= (sizeof(void*) + sizeof(uint32_t)));
                void* impl = reader.Number<void*>();
                uint32_t id = reader.Number<uint32_t>();
                if ((id & 0x80000000) == 0) {
                    // Just AddRef this implementation
                    RPC::Administrator::Instance().AddRef(impl, id);
                } else {
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
