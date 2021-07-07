 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifndef __IPCCONNECTOR_H_
#define __IPCCONNECTOR_H_

#include "Factory.h"
#include "IAction.h"
#include "Link.h"
#include "Module.h"
#include "Portability.h"
#include "SocketPort.h"
#include "TypeTraits.h"

namespace WPEFramework {

namespace Core {

    class IPCChannel;

    struct IMessage {
    public:
        typedef IMessage BaseElement;
        typedef uint32_t Identifier;

        class Serializer {
        public:
            Serializer(const Serializer&) = delete;
            Serializer& operator=(const Serializer&) = delete;

            Serializer()
                : _length(0)
                , _offset(0)
                , _current(nullptr)
            {
            }
            virtual ~Serializer()
            {
                ASSERT(_current == nullptr);
            }

        public:
            bool Submit(const IMessage& element)
            {

                ASSERT(_current == nullptr);

                // TODO: Make sure it is thread safe. The _current needs to
                // be written as the last parameter so in case the
                // serialize gets triggered due to another read/write cycle,
                // Serialize will not start processing until the current (and
                // thius all other parameters) are set correctly.
                _length = element.Length();
                _offset = 0;
                _current = &element;

                ASSERT(_length <= 0x1FFFFFFF);

                return (true);
            }

            // The Serialize and Deserialize methods allow the content to be serialized/deserialized.
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength)
            {
                uint16_t result = 0;

                while ((_current != nullptr) && (result < maxLength)) {
                    if (_offset < 4) {
                        uint32_t length = _length + CommandSize();

                        // Write the length. Continue as long as the top bt is active..
                        while ((_offset < 4) && (result < maxLength)) {
                            uint32_t value = length >> (7 * _offset);
                            stream[result] = ((value & 0x7F) | (value >= 0x80 ? 0x80 : 0x00));
                            result++;

                            if (value >= 0x80) {
                                _offset++;
                            } else {
                                _offset = 4;
                            }
                        }
                    }

                    // Write the command, Same structure as length..
                    while ((_offset < 8) && (result < maxLength)) {
                        uint32_t value = _current->Label() >> (7 * (_offset - 4));
                        stream[result] = ((value & 0x7F) | (value >= 0x80 ? 0x80 : 0x00));
                        result++;

                        if (value >= 0x80) {
                            _offset++;
                        } else {
                            _offset = 8;
                        }
                    }

                    if (result < maxLength) {
                        // Write the command, Same structure as length..
                        uint16_t handled = _current->Serialize(&stream[result], maxLength - result, _offset - 8);

                        result += handled;
                        _offset += handled;

                        ASSERT_VERBOSE((_offset - 8) <= _length, "%d <= %d", (_offset - 8), _length);

                        if ((_offset - 8) == _length) {
                            const IMessage* ready = _current;
                            _current = nullptr;

                            // we are done, send out that we copied it all
                            Serialized(*ready);
                        }
                    }
                }

                return (result);
            }
            virtual void Serialized(const IMessage& element) = 0;

        private:
            inline uint32_t CommandSize() const
            {
                return (_current->Label() > 0x1FFFFF ? 4 : (_current->Label() > 0xCFFF ? 3 : (_current->Label() > 0x7F ? 2 : 1)));
            }

        private:
            uint32_t _length;
            uint32_t _offset;
            const IMessage* _current;
        };

        class Deserializer {
        private:
            Deserializer(const Deserializer&);
            Deserializer& operator=(const Deserializer&);

        public:
            Deserializer()
                : _length(0)
                , _offset(0)
                , _label(0)
                , _current(nullptr)
            {
            }
            virtual ~Deserializer()
            {
            }

        public:
            virtual void Deserialized(IMessage& element) = 0;
            virtual IMessage* Element(const uint32_t& label) = 0;

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength)
            {
                uint16_t result = 0;

                while (result < maxLength) {
					if ((_current == nullptr) && (_offset < 8)) {
                        // We have nothing, start by getting the length/command
                        while ((_offset < 4) && (result < maxLength)) {
                            _length |= ((stream[result] & (_offset == 3 ? 0xFF : 0x7F)) << (7 * _offset));

                            if ((stream[result++] & 0x80) != 0) {
                                _offset++;
                            } else {
                                _offset = 4;
                            }
                        }

                        while ((_offset < 8) && (result < maxLength)) {
                            _label |= ((stream[result] & (_offset == 7 ? 0xFF : 0x7F)) << (7 * (_offset - 4)));
                            _length--;

                            if ((stream[result++] & 0x80) != 0) {
                                _offset++;
                            } else {
                                _offset = 8;
                            }
                        }

                        if (_offset == 8) {
                            _current = Element(_label);
                            _label = 0;
                        }
                    }

                    ASSERT((_offset - 8) <= _length);

                    if ((_offset - 8) < _length) {

                        // There could be multiple packages in this frame, do not read/handle more than what fits in the frame.
                        uint16_t handled((maxLength - result) > static_cast<uint16_t>(_length - (_offset - 8)) ? static_cast<uint16_t>(_length - (_offset - 8)) : (maxLength - result));

                        if (_current != nullptr) {
                            handled = _current->Deserialize(&stream[result], handled, _offset - 8);
                        }

                        _offset += handled;
                        result += handled;
                    }

                    ASSERT((_offset - 8) <= _length);

                    if ((_offset - 8) == _length) {
                        if (_current != nullptr) {
                            IMessage* ready = _current;
                            _current = nullptr;
                            Deserialized(*ready);
                        }
                        _offset = 0;
                        _length = 0;
                    }
                }
                return (result);
            }

        private:
            uint32_t _length;
            uint32_t _offset;
            uint32_t _label;
            IMessage* _current;
        };

    public:
        virtual ~IMessage() {}

        virtual uint32_t Label() const = 0;
        virtual uint32_t Length() const = 0;
        virtual uint16_t Serialize(uint8_t[] /* stream*/, const uint16_t /* maxLength */, const uint32_t offset) const = 0;
        virtual uint16_t Deserialize(const uint8_t[] /* stream*/, const uint16_t /* maxLength */, const uint32_t offset) = 0;
    };

    struct EXTERNAL IIPC {
        inline IIPC() {}
        virtual ~IIPC();

        virtual uint32_t Label() const = 0;
        virtual ProxyType<IMessage> IParameters() = 0;
        virtual ProxyType<IMessage> IResponse() = 0;
    };

    struct EXTERNAL IIPCServer {
        inline IIPCServer() {}
        virtual ~IIPCServer();

        // ================================== CALLED ON COMMUNICATION THREAD =====================================
        // Procedure is always called on the communication thread. so it means that during the context of this
        // call, no state changes on the communication channel can happen. That in turn allows us to "check" for
        // reference counted channels and if needed, increment our reference count before any state changes on
        // the communication channel can happen.
        virtual void Procedure(IPCChannel& source, Core::ProxyType<IIPC>& message) = 0;
    };

    template <const uint32_t IDENTIFIER, typename PARAMETERS, typename RESPONSE>
    class IPCMessageType : public IIPC {
    private:
        template <typename PACKAGE, const uint32_t REALIDENTIFIER>
        class RawSerializedType : public Core::IMessage, public IReferenceCounted {
        private:
            RawSerializedType(const RawSerializedType<PACKAGE, REALIDENTIFIER>& copy) = delete;
            RawSerializedType<PACKAGE, REALIDENTIFIER>& operator=(const RawSerializedType<PACKAGE, REALIDENTIFIER>& copy) = delete;

        public:
            RawSerializedType(IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>& parent)
                : _package()
                , _parent(parent)
            {
            }
            RawSerializedType(IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>& parent, const PACKAGE& package)
                : _package(package)
                , _parent(parent)
            {
            }
            virtual ~RawSerializedType()
            {
            }

        public:
            inline void Clear()
            {
                __Clear();
            }
            inline PACKAGE& Package()
            {
                return (_package);
            }
            virtual uint32_t Label() const
            {
                return (REALIDENTIFIER);
            }
            virtual uint32_t Length() const
            {
                return (_Length());
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_Serialize(stream, maxLength, offset));
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return _Deserialize(stream, maxLength, offset);
            }
            virtual void AddRef() const
            {
                _parent.AddRef();
            }
            virtual uint32_t Release() const
            {
                return (_parent.Release());
            }

        private:
            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            HAS_MEMBER(Clear, hasClear);


            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<hasClear<SUBJECT, void (SUBJECT::*)()>::value, void>::type
            __Clear()
            {
                _package.Clear();
            }

            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<!hasClear<SUBJECT, void (SUBJECT::*)()>::value, void>::type
            __Clear()
            {
            }

            // -----------------------------------------------------
            // Search for custom handling, Compile time !!!
            // -----------------------------------------------------
            HAS_MEMBER(Length, hasLength);


            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<hasLength<SUBJECT, uint32_t (SUBJECT::*)() const>::value, uint32_t>::type
            _Length() const
            {
                return (_package.Length());
            }

            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<!hasLength<SUBJECT, uint32_t (SUBJECT::*)() const>::value, uint32_t>::type
            _Length() const
            {
                return (sizeof(PACKAGE));
            }

            HAS_MEMBER(Serialize, hasSerialize);

            template <typename SUBJECT= PACKAGE>
            inline typename Core::TypeTraits::enable_if<hasSerialize<PACKAGE, uint16_t (SUBJECT::*)(uint8_t[], const uint16_t, const uint32_t) const> ::value, uint16_t>::type
            _Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_package.Serialize(stream, maxLength, offset));
            }

            template <typename SUBJECT= PACKAGE>
            inline typename Core::TypeTraits::enable_if<!hasSerialize<SUBJECT, uint16_t (SUBJECT::*)(uint8_t[], const uint16_t, const uint32_t) const> ::value, uint16_t>::type
            _Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                uint16_t result = 0;
                uint32_t packageLength = _Length();

                if (offset < packageLength) {
                    result = ((maxLength > (packageLength - offset)) ? (packageLength - offset) : maxLength);
                    ::memcpy(stream, &(reinterpret_cast<const uint8_t*>(&_package)[offset]), result);
                }
                return (result);
            }

            HAS_MEMBER(Deserialize, hasDeserialize);


            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<hasDeserialize<SUBJECT, uint16_t (SUBJECT::*)(const uint8_t[], const uint16_t, const uint32_t)>::value, uint16_t>::type
            _Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_package.Deserialize(stream, maxLength, offset));
            }

            template <typename SUBJECT=PACKAGE>
            inline typename Core::TypeTraits::enable_if<!hasDeserialize<SUBJECT, uint16_t (SUBJECT::*)(const uint8_t[], const uint16_t, const uint32_t)>::value, uint16_t>::type
            _Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                uint16_t result = 0;

                if (offset < sizeof(SUBJECT)) {
                    result = (maxLength > static_cast<uint16_t>(sizeof(SUBJECT) - offset) ? static_cast<uint16_t>(sizeof(SUBJECT) - offset) : maxLength);
                    ::memcpy(&(reinterpret_cast<uint8_t*>(&_package)[offset]), stream, result);
                }
                return (result);
            }

        private:
            PACKAGE _package;
            IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>& _parent;
        };

    private:
        IPCMessageType(const IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>&) = delete;
        IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>& operator=(const IPCMessageType<IDENTIFIER, PARAMETERS, RESPONSE>&) = delete;

    public:
        typedef PARAMETERS ParameterType;
        typedef RESPONSE ResponseType;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        IPCMessageType()
            : _parameters(*this)
            , _response(*this)
        {
        }
        IPCMessageType(const PARAMETERS& info)
            : _parameters(*this, info)
            , _response(*this)
        {
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

        virtual ~IPCMessageType()
        {
        }

    public:
        static inline uint32_t Id()
        {
            return (IDENTIFIER);
        }
        inline void Clear()
        {
            _parameters.Clear();
            _response.Clear();
        }
        inline PARAMETERS& Parameters()
        {
            return (_parameters.Package());
        }
        inline RESPONSE& Response()
        {
            return (_response.Package());
        }
        virtual uint32_t Label() const
        {
            return (IDENTIFIER);
        }
        virtual ProxyType<IMessage> IParameters()
        {
            return ProxyType<IMessage>(&_parameters, &_parameters);
        }
        virtual ProxyType<IMessage> IResponse()
        {
            return ProxyType<IMessage>(&_response, &_response);
        }

    private:
        // Make sure you created the final class as a ProxyType
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;

    private:
        RawSerializedType<PARAMETERS, (IDENTIFIER << 1)> _parameters;
        RawSerializedType<RESPONSE, ((IDENTIFIER << 1) | 0x1)> _response;
    };

    class EXTERNAL IPCChannel {
    private:
        IPCChannel(const IPCChannel&) = delete;
        IPCChannel& operator=(const IPCChannel&) = delete;

    public:
        class EXTERNAL IPCFactory {
        private:
            friend IPCChannel;

            IPCFactory(const IPCFactory& copy) = delete;
            IPCFactory& operator=(const IPCFactory&) = delete;

            IPCFactory()
                : _lock()
                , _inbound()
                , _outbound()
                , _callback(nullptr)
                , _factory()
                , _handlers()
            {
            }
            inline void Factory(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory)
            {
                ASSERT((_factory.IsValid() == false) && (factory.IsValid() == true));

                _factory = factory;
            }

        public:
            IPCFactory(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory)
                : _lock()
                , _inbound()
                , _outbound()
                , _callback(nullptr)
                , _factory(factory)
                , _handlers()
            {
                // Only creat the IPCFactory with a valid base factory
                ASSERT(factory.IsValid());
            }
            ~IPCFactory()
            {

                // We expect what you register, to be unregistered before the IPCCHannel is closed !!!
                ASSERT(_handlers.size() == 0);

                std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.begin());

                while (index != _handlers.end()) {
                    (*index).second.Release();
                    index++;
                }

                _handlers.clear();
            }

        public:
            inline void Register(const uint32_t id, const ProxyType<IIPCServer>& handler)
            {
                _lock.Lock();

				ASSERT(handler.IsValid() == true);
                ASSERT(_handlers.find(id) == _handlers.end());

                _handlers.insert(std::pair<uint32_t, ProxyType<IIPCServer>>(id, handler));

                _lock.Unlock();
            }

            inline void Unregister(const uint32_t id)
            {
                _lock.Lock();

                std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.find(id));

                ASSERT(index != _handlers.end());

                if (index != _handlers.end()) {
                    _handlers.erase(index);
                }

                _lock.Unlock();
            }

            inline bool InProgress() const
            {
                return (_outbound.IsValid());
            }

            inline ProxyType<IMessage> Element(const uint32_t& identifier)
            {
                ProxyType<IMessage> result;
                uint32_t searchIdentifier(identifier >> 1);

                _lock.Lock();

                if (identifier & 0x01) {
                    if ((_outbound.IsValid() == true) && (_outbound->Label() == searchIdentifier)) {
                        result = _outbound->IResponse();
                    } else {
                        TRACE_L1("Unexpected response message for ID [%d].\n", searchIdentifier);
                    }
                } else {
                    ASSERT(_inbound.IsValid() == false);

                    ProxyType<IIPC> rpcCall(_factory->Element(searchIdentifier));

                    if (rpcCall.IsValid() == true) {
                        _inbound = rpcCall;
                        result = rpcCall->IParameters();
                    } else {
                        TRACE_L1("No RPC method definition for ID [%d].\n", searchIdentifier);
                    }
                }

                _lock.Unlock();

                return (result);
            }

            inline void Flush()
            {
                _lock.Lock();

                TRACE_L1("Flushing the IPC mechanims. %d", __LINE__);

                _callback = nullptr;

                if (_outbound.IsValid() == true) {
                    _outbound.Release();
                }
                if (_inbound.IsValid() == true) {
                    _inbound.Release();
                }

                _lock.Unlock();
            }

            inline ProxyType<IIPCServer> ReceivedMessage(const Core::ProxyType<IMessage>& rhs, Core::ProxyType<IIPC>& inbound)
            {
                ProxyType<IIPCServer> procedure;

                _lock.Lock();

                if ((_outbound.IsValid() == true) && (_outbound->IResponse() == rhs)) {

                    ASSERT(_callback != nullptr);

                    ProxyType<IIPC> handledObject(_outbound);

                    _outbound.Release();
                    _callback->Dispatch(*handledObject);
                    _callback = nullptr;
                }
                // If this is *NOT* the outbound call, it is inbound and thus it must have been registered
                else if (_inbound.IsValid() == true) {

                    std::map<uint32_t, ProxyType<IIPCServer>>::iterator index(_handlers.find(_inbound->Label()));

					ASSERT(index != _handlers.end());

                    if (index != _handlers.end()) {
                        procedure = (*index).second;
                        inbound = _inbound;
                    } else {
                        TRACE_L1("No handler defined to handle the incoming frames. [%d]", _inbound->Label());
                    }

                    _inbound.Release();
                } else {
                    ASSERT(false && "Received something that is neither an inbound nor on outbound!!!");
                }

                _lock.Unlock();

                return (procedure);
            }

            inline void SetOutbound(const Core::ProxyType<IIPC>& outbound, IDispatchType<IIPC>* callback)
            {
                _lock.Lock();

                ASSERT((outbound.IsValid() == true) && (callback != nullptr));
                ASSERT((_outbound.IsValid() == false) && (_callback == nullptr));

                _outbound = outbound;
                _callback = callback;

                _lock.Unlock();
            }

            inline bool AbortOutbound()
            {
                bool result = false;

                _lock.Lock();

                if (_outbound.IsValid() == true) {

                    result = true;

                    if (_callback != nullptr) {
                        _callback->Dispatch(*_outbound);
                        _callback = nullptr;
                    }

                    _outbound.Release();
                } else {
                    ASSERT(_callback == nullptr);
                }

                _lock.Unlock();

                return (result);
            }

        private:
            mutable CriticalSection _lock;
            Core::ProxyType<IIPC> _inbound;
            mutable Core::ProxyType<IIPC> _outbound;
            IDispatchType<IIPC>* _callback;
            Core::ProxyType<FactoryType<IIPC, uint32_t>> _factory;
            std::map<uint32_t, ProxyType<IIPCServer>> _handlers;
        };

    protected:
        IPCChannel()
            : _administration()
        {
        }
        inline void Factory(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory)
        {
            _administration.Factory(factory);
        }

    public:
        IPCChannel(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory)
            : _administration(factory)
        {
        }
        virtual ~IPCChannel();

    public:
        inline void Register(const uint32_t id, const ProxyType<IIPCServer>& handler)
        {
            _administration.Register(id, handler);
        }

        inline void Unregister(const uint32_t id)
        {
            _administration.Unregister(id);
        }

        inline void Abort()
        {
            _administration.AbortOutbound();
        }
        template <typename ACTUALELEMENT>
        inline uint32_t Invoke(ProxyType<ACTUALELEMENT>& command, IDispatchType<IIPC>* completed)
        {
            Core::ProxyType<IIPC> base(Core::proxy_cast<IIPC>(command));
            return (Execute(base, completed));
        }
        template <typename ACTUALELEMENT>
        inline uint32_t Invoke(ProxyType<ACTUALELEMENT>& command, const uint32_t waitTime)
        {
            Core::ProxyType<IIPC> base(Core::proxy_cast<IIPC>(command));
            return (Execute(base, waitTime));
        }
        inline uint32_t Invoke(ProxyType<Core::IIPC>& command, IDispatchType<IIPC>* completed)
        {
            return (Execute(command, completed));
        }
        inline uint32_t Invoke(ProxyType<Core::IIPC>& command, const uint32_t waitTime)
        {
            return (Execute(command, waitTime));
        }

        virtual uint32_t ReportResponse(Core::ProxyType<IIPC>& inbound) = 0;

    private:
        virtual uint32_t Execute(ProxyType<IIPC>& command, IDispatchType<IIPC>* completed) = 0;
        virtual uint32_t Execute(ProxyType<IIPC>& command, const uint32_t waitTime) = 0;

    protected:
        IPCFactory _administration;
    };

    template <typename ACTUALSOURCE, typename EXTENSION>
    class IPCChannelType : public IPCChannel {
    private:
        IPCChannelType(const IPCChannelType<ACTUALSOURCE, EXTENSION>&) = delete;
        IPCChannelType<ACTUALSOURCE, EXTENSION>& operator=(const IPCChannelType<ACTUALSOURCE, EXTENSION>&) = delete;

        class IPCLink : public LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&> {
        private:
            typedef LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&> BaseClass;

            IPCLink() = delete;
            IPCLink(const IPCLink&) = delete;
            IPCLink& operator=(const IPCLink&) = delete;

        public:
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            template <typename ARG1>
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory, ARG1 arg1)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory, arg1)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            template <typename ARG1, typename ARG2>
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory, ARG1 arg1, ARG2 arg2)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory, arg1, arg2)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            template <typename ARG1, typename ARG2, typename ARG3>
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory, ARG1 arg1, ARG2 arg2, ARG3 arg3)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory, arg1, arg2, arg3)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory, arg1, arg2, arg3, arg4)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
            IPCLink(IPCChannelType<ACTUALSOURCE, EXTENSION>* parent, IPCFactory* factory, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
                : LinkType<ACTUALSOURCE, IMessage, IMessage, IPCFactory&>(2, *factory, arg1, arg2, arg3, arg4, arg5)
                , _factory(*factory)
                , _parent(*parent)
            {
            }
            ~IPCLink()
            {
            }

        public:
            inline bool SendResponse(Core::ProxyType<IIPC>& inbound)
            {
                ASSERT(inbound.IsValid() == true);

                // This is an inbound call, Report what we have processed !!!
                return (BaseClass::Submit(inbound->IResponse()));
            }

            // Notification of a INBOUND element received.
            virtual void Received(Core::ProxyType<IMessage>& message)
            {

                Core::ProxyType<IIPC> inbound;
                ProxyType<IIPCServer> handler(_factory.ReceivedMessage(message, inbound));

                if (handler.IsValid() == true) {
                    _parent.CallProcedure(handler, inbound);
                } 
            }

            // Notification of a Response send.
            virtual void Send(const Core::ProxyType<IMessage>& message VARIABLE_IS_NOT_USED)
            {
                // Oke, nice we send out the info. Nothing to do. It will all be triggered by the Receive..
            }

            // Notification of a channel state change..
            virtual void StateChange()
            {
                if (_parent.Source().IsOpen() == false) {
                    // Whatever s hapening, Flush what we were doing..
                    _parent.Abort();
                    _factory.Flush();
                }

                _parent.StateChange();
            }

        private:
            IPCFactory& _factory;
            IPCChannelType<ACTUALSOURCE, EXTENSION>& _parent;
        };

        class IPCTrigger : public IDispatchType<IIPC> {
        private:
            IPCTrigger() = delete;
            IPCTrigger(const IPCTrigger&) = delete;
            IPCTrigger& operator=(const IPCTrigger&) = delete;

        public:
            IPCTrigger(IPCFactory& administration)
                : _administration(administration)
                , _signal(false, true)
            {
            }
            virtual ~IPCTrigger()
            {
            }

        public:
            uint32_t Wait(const uint32_t waitTime)
            {
                uint32_t result = Core::ERROR_NONE;

                // Now we wait for ever, to get a signal that we are done :-)
                if (_signal.Lock(waitTime) != Core::ERROR_NONE) {
                    _administration.AbortOutbound();

                    result = Core::ERROR_TIMEDOUT;
                } else if (_administration.AbortOutbound() == true) {
                    result = Core::ERROR_ASYNC_FAILED;
                }

                return (result);
            }
            virtual void Dispatch(IIPC& /* element */)
            {
                _signal.SetEvent();
            }

        private:
            IPCFactory& _administration;
            Event _signal;
        };

    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        template <typename ARG1>
        IPCChannelType(ARG1 arg1)
            : IPCChannel()
            , _link(this, &_administration, arg1)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2>
        IPCChannelType(ARG1 arg1, ARG2 arg2)
            : IPCChannel()
            , _link(this, &_administration, arg1, arg2)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3>
        IPCChannelType(ARG1 arg1, ARG2 arg2, ARG3 arg3)
            : IPCChannel()
            , _link(this, &_administration, arg1, arg2, arg3)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
        IPCChannelType(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
            : IPCChannel()
            , _link(this, &_administration, arg1, arg2, arg3, arg4)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
        IPCChannelType(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
            : IPCChannel()
            , _link(this, &_administration, arg1, arg2, arg3, arg4, arg5)
            , _extension(this)
        {
        }
        template <typename ARG1>
        IPCChannelType(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory, ARG1 arg1)
            : IPCChannel(factory)
            , _link(this, &_administration, arg1)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2>
        IPCChannelType(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory, ARG1 arg1, ARG2 arg2)
            : IPCChannel(factory)
            , _link(this, &_administration, arg1, arg2)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3>
        IPCChannelType(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory, ARG1 arg1, ARG2 arg2, ARG3 arg3)
            : IPCChannel(factory)
            , _link(this, &_administration, arg1, arg2, arg3)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
        IPCChannelType(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
            : IPCChannel(factory)
            , _link(this, &_administration, arg1, arg2, arg3, arg4)
            , _extension(this)
        {
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
        IPCChannelType(Core::ProxyType<FactoryType<IIPC, uint32_t>>& factory, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
            : IPCChannel(factory)
            , _link(this, &_administration, arg1, arg2, arg3, arg4, arg5)
            , _extension(this)
        {
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

        virtual ~IPCChannelType()
        {
        }

    public:
        inline EXTENSION& Extension()
        {
            return (_extension);
        }
        inline const EXTENSION& Extension() const
        {
            return (_extension);
        }
        inline ACTUALSOURCE& Source()
        {
            return (_link.Link());
        }
        inline const ACTUALSOURCE& Source() const
        {
            return (_link.Link());
        }
        inline bool InProgress() const
        {
            return (_administration.InProgress());
        }
        virtual uint32_t ReportResponse(Core::ProxyType<IIPC>& inbound)
        {

            // We got the event, start the invoke, wait for the event to be set again..
            _link.SendResponse(inbound);

            return (Core::ERROR_NONE);
        }
        virtual void StateChange()
        {
            __StateChange();
        }

    private:
        HAS_MEMBER(StateChange, hasStateChange);

        template <typename T=EXTENSION>
        inline typename Core::TypeTraits::enable_if<hasStateChange<T, void (T::*)()> ::value, void>::type
        __StateChange()
        {
            _extension.StateChange();
        }


        template <typename T=EXTENSION>
        inline typename Core::TypeTraits::enable_if<!hasStateChange<T, void (T::*)()> ::value, void>::type
        __StateChange()
        {
        }

        virtual uint32_t Execute(ProxyType<IIPC>& command, IDispatchType<IIPC>* completed)
        {
            uint32_t success = Core::ERROR_UNAVAILABLE;

            _serialize.Lock();

            if (_administration.InProgress() == true) {
                success = Core::ERROR_INPROGRESS;
            } else if (_link.IsOpen() == true) {
                // We need to accept a CONST object to avoid an additional object creation
                // proxy casted objects.
                _administration.SetOutbound(command, completed);

                // Send out the
                _link.Submit(command->IParameters());

                success = Core::ERROR_NONE;
            }

            _serialize.Unlock();

            return (success);
        }
        virtual uint32_t Execute(ProxyType<IIPC>& command, const uint32_t waitTime)
        {
            uint32_t success = Core::ERROR_CONNECTION_CLOSED;

            _serialize.Lock();

            if (_link.IsOpen() == true) {
                IPCTrigger sink(_administration);

                // We need to accept a CONST object to avoid an additional object creation
                // proxy casted objects.
                _administration.SetOutbound(command, &sink);

                // Send out the
                _link.Submit(command->IParameters());

                success = sink.Wait(waitTime);
            }

            _serialize.Unlock();

            return (success);
        }
        inline void CallProcedure(ProxyType<IIPCServer>& procedure, ProxyType<IIPC>& message)
        {
            procedure->Procedure(*this, message);
        }

    private:
        CriticalSection _serialize;
        IPCLink _link;
        EXTENSION _extension;
    };
}
} // namespace Core

#endif // __IPCCONNECTOR_H_
