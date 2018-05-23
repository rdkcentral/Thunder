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

    template <typename INTERFACE, MethodHandler METHODS[], typename BASECLASS>
    class StubType : public BASECLASS {
    public:
        typedef INTERFACE* CLASS_INTERFACE;

    private:
        StubType(const StubType<INTERFACE, METHODS, BASECLASS>&);
        StubType<INTERFACE, METHODS, BASECLASS>& operator=(const StubType<INTERFACE, METHODS, BASECLASS>&);

    public:
        StubType()
        {
            _myHandlerCount = 0;

            while (METHODS[_myHandlerCount] != nullptr) {
                _myHandlerCount++;
            }
        }
        virtual ~StubType()
        {
        }

    public:
        inline uint16_t Length() const
        {
            return (_myHandlerCount + BASECLASS::Length());
        }
        virtual Core::IUnknown* Convert(void* incomingData) const
        {
            return (reinterpret_cast<INTERFACE*>(incomingData));
        }
        virtual void Handle(const uint16_t index, Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message)
        {
            uint16_t baseNumber(BASECLASS::Length());

            if (index < baseNumber) {
                BASECLASS::Handle(index, channel, message);
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
    template <typename INTERFACE>
    class UnknownProxyType : public INTERFACE {
    protected:
        typedef UnknownProxyType<INTERFACE> BaseClass;
        typedef Core::ProxyType<RPC::InvokeMessage> IPCMessage;

    private:
        UnknownProxyType(const UnknownProxyType<INTERFACE>&) = delete;
        UnknownProxyType<INTERFACE>& operator=(const UnknownProxyType<INTERFACE>&) = delete;

    public:
        UnknownProxyType(Core::ProxyType<Core::IPCChannel>& channel, void* implementation, bool otherSideInformed)
            : _otherSideInformed(otherSideInformed)
            , _refCount(1)
            , _implementation(implementation)
            , _channel(channel)
        {
        }
        virtual ~UnknownProxyType()
        {
            // We might need to kill the Proxy registration..
            RPC::Administrator::Instance().DeleteProxy(_implementation);
        }

    public:
        inline IPCMessage Message(const uint8_t methodId) const
        {
            IPCMessage message(RPC::Administrator::Instance().Message());

            message->Parameters().Set(_implementation, INTERFACE::ID, methodId + 3);

            return (message);
        }
        inline uint32_t Invoke(IPCMessage& message, const uint32_t waitTime = RPC::CommunicationTimeOut) const
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
        virtual void AddRef() const
        {

            if ((Core::InterlockedIncrement(_refCount) == 2) && (_otherSideInformed == false)) {

                // Seems we really would like to "preserve" this interface, so report it in use
                _otherSideInformed = true;

                Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

                message->Parameters().Set(_implementation, INTERFACE::ID, 0);

                // Just try the destruction for few Seconds...
                if (Invoke(message, RPC::CommunicationTimeOut) != Core::ERROR_NONE) {
                    TRACE_L1("Could NOT addref the actual object from the Proxy. %d", __LINE__);
                }
            }
        }

        virtual uint32_t Release() const
        {

            uint32_t result(Core::ERROR_NONE);

            if (Core::InterlockedDecrement(_refCount) == 0) {

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;

                if (_otherSideInformed == true) {

                    // We have reached "0", signal the other side..
                    Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

                    message->Parameters().Set(_implementation, INTERFACE::ID, 1);

                    // Just try the destruction for few Seconds...
					result = Invoke(message, RPC::CommunicationTimeOut);
					
					if ( (result != Core::ERROR_NONE) && (result != Core::ERROR_CONNECTION_CLOSED) ) {
                        result = Core::ERROR_DESTRUCTION_FAILED;
                    }
                }

                delete this;
            }

            return (result);
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* QueryInterface()
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(QueryInterface(ACTUAL_INTERFACE::ID)));
        }
        virtual void* QueryInterface(const uint32_t interfaceNumber)
        {
            void* result = nullptr;

            // We have reached "0", signal the other side..
            Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());
            RPC::Data::Frame::Writer parameters(message->Parameters().Writer());

            message->Parameters().Set(_implementation, INTERFACE::ID, 2);
            parameters.Number<uint32_t>(interfaceNumber);
            if (Invoke(message, RPC::CommunicationTimeOut) == Core::ERROR_NONE) {
                RPC::Data::Frame::Reader response(message->Response().Reader());

                // From what is returned, we need to create a proxy
                result = CreateProxy(response.Number<Core::IUnknown*>(), interfaceNumber);
            }

            return (result);
        }
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* CreateProxy(void* implementation)
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(CreateProxy(implementation, ACTUAL_INTERFACE::ID)));
        }
        inline void* CreateProxy(void* implementation, const uint32_t id)
        {
            return (RPC::Administrator::Instance().CreateProxy(id, _channel, implementation, false, true));
        }

    private:
        mutable bool _otherSideInformed;
        mutable uint32_t _refCount;
        void* _implementation;
        mutable Core::ProxyType<Core::IPCChannel> _channel;
    };
}
} // namespace ProxyStub

#endif //  __COM_IUNKNOWN_H
