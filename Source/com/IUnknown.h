/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __COM_IUNKNOWN_H
#define __COM_IUNKNOWN_H

#include "Module.h"
#include "Ids.h"
#include "Administrator.h"
#include "Messages.h"

namespace WPEFramework {

namespace RPC {
    class Communicator;
}

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
	virtual Core::IUnknown* Convert(void* incomingData) const {
            return (reinterpret_cast<Core::IUnknown*>(incomingData));
        }
	virtual uint32_t InterfaceId() const {
            return (Core::IUnknown::ID);
        }
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
            return (reinterpret_cast<INTERFACE*>(incomingData));
        }
	virtual uint32_t InterfaceId() const {
            return (INTERFACE::ID);
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
    // Proxy/Stub generation requirements:
    // 1) Interfaces, used as retrieved in the stub code are called inbound. These proxies hold no reference on account of
    //    the callee. The initial AddRef on this proxy is an AddRef that can be cached. 
    // 2) Interfaces, returned in the proxy code are called outbound. Outbound proxies are referenced by definition, and thus
    //    should be released.
    //
    // Characteristics of proxies:
    // Inbound proxies can cache a pending AddRef if required.
    // Inbound and Outbound do require a remote Release on a transaition from 1 -> 0 AddRef.
    // -------------------------------------------------------------------------------------------

    class UnknownProxy {
    private:
        UnknownProxy(const UnknownProxy&) = delete;
        UnknownProxy& operator=(const UnknownProxy&) = delete;

    private:
        enum mode : uint8_t {
            CACHING_ADDREF   = 0x01,
            CACHING_RELEASE  = 0x02
        };

    public:
        UnknownProxy(const Core::ProxyType<Core::IPCChannel>& channel, const RPC::instance_id& implementation, const uint32_t interfaceId, const bool outbound, Core::IUnknown& parent)
            : _adminLock()
            , _refCount(0)
            , _mode(outbound ? 0 : CACHING_ADDREF)
            , _interfaceId(interfaceId)
            , _implementation(implementation)
            , _parent(parent)
            , _channel(channel)
            , _remoteReferences(1)
        {
        }
        virtual ~UnknownProxy()
        {
        }

    public:
        // -------------------------------------------------------------------------------------------------------------------------------
        // Proxy/Stub (both) environment calls
        // -------------------------------------------------------------------------------------------------------------------------------
        void* Aquire(const bool outbound, const uint32_t id) {

            void* result = nullptr;

            _adminLock.Lock();

            if (_refCount > 0) {
                if (outbound == false) {
                    _mode |= CACHING_RELEASE;
                }
                // This will increment the refcount of this PS, on behalf of the user.
                result = _parent.QueryInterface(id);

                // This is an additional call to an interface for which we already have a Proxy issued.
                // This is triggerd by the other side, offering us an interface. So once this proxy goes
                // out of scope, we also need to "release" the AddRef associated with this on the other
                // side..
                _remoteReferences++;
            }
            _adminLock.Unlock();

            return(result);
        }
        void AddRef() const {
            _adminLock.Lock();
            _refCount++;
            _adminLock.Unlock();
        }
        uint32_t Release() const {
            uint32_t result = Core::ERROR_NONE;

            _adminLock.Lock();
            _refCount--;

            if (_refCount != 0) {
                _adminLock.Unlock();
            }
            else {

                if ( (_mode & (CACHING_RELEASE|CACHING_ADDREF)) == 0) {

                    // We have reached "0", signal the other side..
                    Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());

                    message->Parameters().Set(_implementation, _interfaceId, 1);

                    // Pass on the number of reference we need to lower, since it is indictaed by the amount of times this proxy had to be created
                    message->Parameters().Writer().Number<uint32_t>(_remoteReferences);

                    // Just try the destruction for few Seconds...
                    result = Invoke(message, RPC::CommunicationTimeOut);

                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("Could not remote release the Proxy.");
                    } else {
                        // Pass the remote release return value through
                        result = message->Response().Reader().Number<uint32_t>();
                    }
                }

                _adminLock.Unlock();

                // Remove our selves from the Administration, we are done..
                RPC::Administrator::Instance().UnregisterProxy(*this);

                result = (result == Core::ERROR_NONE ? Core::ERROR_DESTRUCTION_SUCCEEDED : result);
            }
            return (result);
        }
        inline void* RemoteInterface(const uint32_t id) const
        {
            void* result = nullptr;
            Core::ProxyType<RPC::InvokeMessage> message(RPC::Administrator::Instance().Message());
            RPC::Data::Frame::Writer parameters(message->Parameters().Writer());

            message->Parameters().Set(_implementation, _interfaceId, 2);
            parameters.Number<uint32_t>(id);
            if (Invoke(message, RPC::CommunicationTimeOut) == Core::ERROR_NONE) {
                RPC::Data::Frame::Reader response(message->Response().Reader());
                RPC::instance_id impl = response.Number<RPC::instance_id>();
                // From what is returned, we need to create a proxy
                RPC::Administrator::Instance().ProxyInstance(_channel, impl, true, id, result);
            }
            return (result);
        }
        inline Core::IUnknown* Parent() 
        {
            return (&_parent);
        }
        const Core::ProxyType<Core::IPCChannel>& Channel() const
        {
            return (_channel);
        }
        inline uint32_t InterfaceId() const
        {
            return (_interfaceId);
        }
        inline const RPC::instance_id& Implementation() const
        {
            return (_implementation);
        }

        // -------------------------------------------------------------------------------------------------------------------------------
        // Proxy environment calls
        // -------------------------------------------------------------------------------------------------------------------------------
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
                TRACE_L1("IPC method invokation failed for 0x%X, error: %d", message->Parameters().InterfaceId(), result);
            }

            return (result);
        }
        inline void Complete(RPC::Data::Frame::Reader& reader) const
        {
            while (reader.HasData() == true) {
                ASSERT(reader.Length() >= (sizeof(RPC::instance_id)));  // IMPLEMENTATION SIZE !!!!
                void* impl = reinterpret_cast<void*>(reader.Number<RPC::instance_id>());
                uint32_t id = reader.Number<uint32_t>();

                if ((id & 0x80000000) == 0) {
                    // Just AddRef this implementation
                    RPC::Administrator::Instance().AddRef(_channel, impl, id);
                } else {
                    // Just Release this implementation
                    id = id ^ 0x80000000;
                    RPC::Administrator::Instance().Release(_channel, impl, id, 1);
                }
            }
        }

        // -------------------------------------------------------------------------------------------------------------------------------
        // Stub environment calls
        // -------------------------------------------------------------------------------------------------------------------------------
        // This method should only be called from the administrator from stub implementation methods
        // It should be called through the Administrator::Release(ProxyStub*, Message::Response) !!
	    // Concurrent access trhough this code is prevented by the CriticalSection in the Administrator
        template <typename ACTUAL_INTERFACE>
        inline ACTUAL_INTERFACE* QueryInterface() const
        {
            return (reinterpret_cast<ACTUAL_INTERFACE*>(QueryInterface(ACTUAL_INTERFACE::ID)));
        }
        inline void* QueryInterface(const uint32_t id) const
        {
            ASSERT (_interfaceId == id);

            return (_parent.QueryInterface(id));
        }
        inline void Complete(RPC::Data::Output& response) 
        {
            uint32_t result = Release();

            _adminLock.Lock();

            if ((_mode & CACHING_ADDREF) != 0) {

                // We completed the first cycle. Clear Pending, if it was active..
                _mode ^= CACHING_ADDREF;

                if (_refCount == 1) {
                    response.AddImplementation(_implementation, _interfaceId);
                }
            }
            else if ((_mode & CACHING_RELEASE) != 0)  {

                // We completed the current cycle. Clear the CACHING_RELEASE, if it was active..
                _mode ^= CACHING_RELEASE;

                if (_refCount == 0) {
                    response.AddImplementation(_implementation, _interfaceId | 0x80000000);
                }
            }

            _adminLock.Unlock();

            if (result != Core::ERROR_NONE) {
                delete &_parent;
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        mutable uint32_t _refCount;
        uint8_t _mode;
        const uint32_t _interfaceId;
        RPC::instance_id _implementation;
        Core::IUnknown& _parent;
        mutable Core::ProxyType<Core::IPCChannel> _channel;
        uint32_t _remoteReferences;
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
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        UnknownProxyType(const Core::ProxyType<Core::IPCChannel>& channel, const RPC::instance_id& implementation, const bool outbound)
            : _unknown(channel, implementation, INTERFACE::ID, outbound, *this)
        {
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        virtual ~UnknownProxyType()
        {
        }

    public:
        UnknownProxy* Administration() 
        {
            return(&_unknown);
        }

        // -------------------------------------------------------------------------------------------------------------------------------
        // Proxy environment calls
        // -------------------------------------------------------------------------------------------------------------------------------
        inline IPCMessage Message(const uint8_t methodId) const
        {
            return (_unknown.Message(methodId));
        }
        inline uint32_t Invoke(Core::ProxyType<RPC::InvokeMessage>& message, const uint32_t waitTime = RPC::CommunicationTimeOut) const
        {
            return (_unknown.Invoke(message, waitTime));
        }
        inline void* Interface(const RPC::instance_id& implementation, const uint32_t id) const
        {
            void* result = nullptr;
            RPC::Administrator::Instance().ProxyInstance(_unknown.Channel(),implementation,true,id,result);
            return (result);
        }
        inline void Complete(RPC::Data::Frame::Reader& reader) const
        {
            return (_unknown.Complete(reader));
        }

        // -------------------------------------------------------------------------------------------------------------------------------
        // Applications calls to the Proxy
        // -------------------------------------------------------------------------------------------------------------------------------
        virtual void AddRef() const override
        {
            _unknown.AddRef();
        }
        virtual uint32_t Release() const override
        {
            uint32_t result = _unknown.Release();

            if (result != Core::ERROR_NONE) {
                delete (this);
            }

            return (result);
        }
        virtual void* QueryInterface(const uint32_t interfaceNumber) override
        {
            void* result = nullptr;

            if (interfaceNumber == INTERFACE::ID) {
                // Just AddRef and return..
                _unknown.AddRef();
                result = static_cast<INTERFACE*>(this);
            } else if (interfaceNumber == Core::IUnknown::ID) {
                // Just AddRef and return..
                _unknown.AddRef();
                result = static_cast<Core::IUnknown*>(this);
            } else {
                result = _unknown.RemoteInterface(interfaceNumber);
            }

            return (result);
        }
    private:
        UnknownProxy _unknown;
    };
}
} // namespace ProxyStub

#endif //  __COM_IUNKNOWN_H
