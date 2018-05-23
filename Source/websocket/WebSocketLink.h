#ifndef __WEBSOCKETLINK_H
#define __WEBSOCKETLINK_H

#include "Module.h"
#include "WebRequest.h"
#include "WebResponse.h"
#include "WebLink.h"

namespace WPEFramework {
namespace Web {
    namespace WebSocket {
        class EXTERNAL Protocol {
        public:
            enum frameType {
                TEXT = 0x01,
                BINARY = 0x02,
                CLOSE = 0x08,
                PING = 0x09,
                PONG = 0x0A,
                VIOLATION = 0x10, // e.g. a control package without a FIN flag
                TOO_BIG = 0x20, // Protocol max support for 2^16 message per chunk
                INCONSISTENT = 0x30 // e.g. Protocol defined as Text, but received a binary.
            };

        private:
            enum controlTypes {
                REQUEST_CLOSE = 0x01,
                REQUEST_PING = 0x02,
                REQUEST_PONG = 0x04,
                CLOSE_INPROGRESS = 0x08
            };

            Protocol() = delete;
            Protocol(const Protocol&) = delete;
            Protocol& operator=(const Protocol&) = delete;

        public:
            Protocol(const bool binary, const bool masking)
                : _setFlags((masking ? 0x80 : 0x00) | (binary ? 0x02 : 0x01))
                , _progressInfo(0)
                , _pendingReceiveBytes(0)
                , _controlStatus(0)

            {
            }
            ~Protocol()
            {
            }

        public:
            std::string RequestKey() const;
            std::string ResponseKey(const std::string& requestKey) const;

            inline void Ping()
            {
                _controlStatus |= REQUEST_PING;
            }
            inline void Pong()
            {
                _controlStatus |= REQUEST_PONG;
            }
            inline void Close()
            {
                _controlStatus |= REQUEST_CLOSE;
            }
            inline bool ReceiveInProgress() const
            {
                return ((_progressInfo & 0x80) != 0);
            }
            inline bool SendInProgress() const
            {
                return ((_progressInfo & 0x40) != 0);
            }
            inline bool IsCompleteMessage() const
            {
                return (_pendingReceiveBytes == 0);
            }
            inline void Flush()
            {
                _pendingReceiveBytes = 0;
            }
            inline WebSocket::Protocol::frameType FrameType() const
            {
                return (_frameType);
            }
            inline void Binary(const bool binary)
            {
                _setFlags = ((_setFlags & 0xFC) | (binary ? 0x02 : 0x01));
            }
            inline bool Binary() const
            {
                return ((_setFlags & 0x02) != 0);
            }
            inline void Masking(const bool masking)
            {
                _setFlags = (masking ? (_setFlags | 0x80) : (_setFlags & 0x7F));
            }
            inline bool Masking() const
            {
                return ((_setFlags & 0x80) != 0);
            }

            uint16_t Encoder(uint8_t* dataFrame, const uint16_t maxSendSize, const uint16_t usedSize);
            uint16_t Decoder(uint8_t* dataFrame, uint16_t& receivedSize);

        private:
            uint8_t _setFlags;
            uint8_t _progressInfo;
            uint32_t _pendingReceiveBytes;
            frameType _frameType;
            uint8_t _scrambleKey[4];
            uint8_t _controlStatus;
        };

        class EXTERNAL RequestAllocator : public Core::ProxyPoolType<Web::Request> {
        private:
            RequestAllocator(const RequestAllocator&) = delete;
            RequestAllocator& operator=(const RequestAllocator&) = delete;

        public:
            static RequestAllocator& Instance();

            RequestAllocator()
                : Core::ProxyPoolType<Web::Request>(5)
            {
            }
            ~RequestAllocator()
            {
            }
        };

        class EXTERNAL ResponseAllocator : public Core::ProxyPoolType<Web::Response> {
        private:
            ResponseAllocator(const ResponseAllocator&) = delete;
            ResponseAllocator& operator=(const ResponseAllocator&) = delete;

        public:
            static ResponseAllocator& Instance();

            ResponseAllocator()
                : Core::ProxyPoolType<Web::Response>(5)
            {
            }
            ~ResponseAllocator()
            {
            }
        };
    }

    template <typename LINK, typename INBOUND, typename OUTBOUND, typename ALLOCATOR>
    class WebSocketLinkType {
    public:
        enum EnumlinkState {
            WEBSERVER = 0x01,
            UPGRADING = 0x02,
            WEBSOCKET = 0x04,
            SUSPENDED = 0x08,
            ACTIVITY = 0x10
        };

    private:
        WebSocketLinkType();
        WebSocketLinkType(const WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& copy) = delete;
        WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& RHS) = delete;

        typedef WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR> ParentClass;

    private:
        template <typename ACTUALLINK>
        class HandlerType : public ACTUALLINK {
        private:
            typedef HandlerType<ACTUALLINK> ThisClass;

            class SerializerImpl : public OUTBOUND::Serializer {
            private:
                typedef typename OUTBOUND::Serializer BaseClass;

                SerializerImpl() = delete;
                SerializerImpl(const SerializerImpl&) = delete;
                SerializerImpl& operator=(const SerializerImpl&) = delete;

            public:
                SerializerImpl(ThisClass& parent, const uint8_t queueSize)
                    : OUTBOUND::Serializer()
                    , _parent(parent)
                    , _adminLock()
                    , _queue(queueSize)
                {
                }
                virtual ~SerializerImpl()
                {
                }

            public:
                inline bool IsIdle() const
                {
                    return (_queue.Count() == 0);
                }
                bool Submit(const Core::ProxyType<typename OUTBOUND::BaseElement>& element)
                {
                    bool result = false;

                    _adminLock.Lock();

                    _queue.Add(const_cast<Core::ProxyType<typename OUTBOUND::BaseElement>&>(element));

                    // See if we need to push the first one..
                    if (_queue.Count() == 1) {
                        result = true;
                        OUTBOUND::Serializer::Submit(*element);
                    }

                    _adminLock.Unlock();

                    return (result);
                }
                inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength)
                {
                    return (OUTBOUND::Serializer::Serialize(stream, maxLength));
                }
                void Flush()
                {
                    _adminLock.Lock();

                    OUTBOUND::Serializer::Flush();

                    while (_queue.Count() > 0) {
                        Core::ProxyType<OUTBOUND> element;
                        _queue.Remove(0, element);

                        _adminLock.Unlock();

                        _parent.Serialized(element);

                        _adminLock.Lock();
                    }

                    _adminLock.Unlock();
                }

            private:
                virtual void Serialized(const typename OUTBOUND::BaseElement& element)
                {
                    _adminLock.Lock();

                    ASSERT(&element == static_cast<OUTBOUND*>(&(*(_queue[0]))));
                    DEBUG_VARIABLE(element);

                    Core::ProxyType<typename OUTBOUND::BaseElement> realItem;
                    _queue.Remove(0, realItem);

                    _adminLock.Unlock();

                    if (_parent._webSocketMessage == realItem) {
                        _parent.UpgradeCompleted();
                    }
                    else {
                        _parent.Serialized(Core::proxy_cast<OUTBOUND>(realItem));
                    }

                    _adminLock.Lock();

                    if ((_parent.IsSuspended() == false) && (_queue.Count() > 0)) {
                        OUTBOUND::Serializer::Submit(*(_queue[0]));
                        _adminLock.Unlock();

                        _parent.Trigger();
                    }
                    else {
                        _adminLock.Unlock();
                    }

                    // TRACE_L1("Released the ref object %s [%p]\n", typeid(typename OUTBOUND::BaseElement).name(), &static_cast<typename OUTBOUND::BaseElement&>(*realItem));
                }

            private:
                ThisClass& _parent;
                Core::CriticalSection _adminLock;
                Core::ProxyList<typename OUTBOUND::BaseElement> _queue;
            };
            class DeserializerImpl : public INBOUND::Deserializer {
            private:
                DeserializerImpl() = delete;
                DeserializerImpl(const DeserializerImpl&) = delete;
                DeserializerImpl& operator=(const DeserializerImpl&) = delete;

            public:
                DeserializerImpl(ThisClass& parent, const uint8_t queueSize)
                    : INBOUND::Deserializer()
                    , _parent(parent)
                    , _pool(queueSize)
                {
                }
                DeserializerImpl(ThisClass& parent, ALLOCATOR allocator)
                    : INBOUND::Deserializer()
                    , _parent(parent)
                    , _pool(allocator)
                {
                }
                virtual ~DeserializerImpl()
                {
                }

            public:
                inline bool IsIdle() const
                {
                    return (_current.IsValid() == false);
                }
                virtual void Deserialized(typename INBOUND::BaseElement& element)
                {
                    ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));
                    DEBUG_VARIABLE(element);

                    _parent.Deserialized(_current);

                    _current.Release();
                }
                virtual typename INBOUND::BaseElement* Element()
                {
                    if (_parent.IsSuspended() == false) {
                        _current = _pool.Element();

                        ASSERT(_current.IsValid());

                        return static_cast<typename INBOUND::BaseElement*>(&(*_current));
                    }

                    return (nullptr);
                }
                virtual bool LinkBody(typename INBOUND::BaseElement& element)
                {
                    ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));
                    DEBUG_VARIABLE(element);

                    return (_parent.LinkBody(_current));
                }

            private:
                ThisClass& _parent;
                Core::ProxyType<INBOUND> _current;
                ALLOCATOR _pool;
            };

        private:
            HandlerType() = delete;
            HandlerType(const HandlerType<ACTUALLINK>&) = delete;
            HandlerType<ACTUALLINK>& operator=(const HandlerType<ACTUALLINK>&) = delete;

        public:
			#ifdef __WIN32__ 
			#pragma warning( disable : 4355 )
			#endif
            template <typename Arg1>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1)
                : ACTUALLINK(arg1)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2)
                : ACTUALLINK(arg1, arg2)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : ACTUALLINK(arg1, arg2, arg3)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : ACTUALLINK(arg1, arg2, arg3, arg4)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5, arg6)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, queueSize)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1)
                : ACTUALLINK(arg1)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2)
                : ACTUALLINK(arg1, arg2)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : ACTUALLINK(arg1, arg2, arg3)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : ACTUALLINK(arg1, arg2, arg3, arg4)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5, arg6)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVER)
                , _serializerImpl(*this, queueSize)
                , _deserialiserImpl(*this, allocator)
                , _path()
                , _protocol()
                , _query()
                , _origin()
                , _webSocketMessage(Core::ProxyType<typename OUTBOUND::BaseElement>::Create())
                , _pingFireTime(0)
            {
            }
			#ifdef __WIN32__ 
			#pragma warning( default : 4355 )
			#endif

            virtual ~HandlerType()
            {
            }

        public:
            inline bool IsOpen() const
            {
                return ((ACTUALLINK::IsOpen() == true) && ((_state & SUSPENDED) == 0));
            }
            inline bool IsSuspended() const
            {
                return ((ACTUALLINK::IsSuspended() == true) || ((_state & SUSPENDED) != 0));
            }
            inline bool IsClosed() const
            {
                return (ACTUALLINK::IsClosed() == true);
            }
            inline bool IsWebServer() const
            {
                return ((_state & WEBSERVER) != 0);
            }
            inline bool IsUpgrading() const
            {
                return ((_state & UPGRADING) != 0);
            }
            inline bool IsWebSocket() const
            {
                return ((_state & WEBSOCKET) != 0);
            }
            inline bool IsCompleted () const {
                return (_handler.ReceiveInProgress() == false);
            }
            inline const string& Path() const
            {
                return (_path);
            }
            inline const string& Protocol() const
            {
                return (_protocol);
            }
            inline const string& Query() const
            {
                return (_query);
            }
            inline const string& Origin() const
            {
                return (_origin);
            }
            inline void Binary(const bool binary)
            {
                _handler.Binary(binary);
            }
            inline bool Binary() const
            {
                return (_handler.Binary());
            }
            inline void Masking(const bool masking)
            {
                _handler.Masking(masking);
            }
            inline void Ping()
            {
                _pingFireTime = Core::Time::Now().Ticks();

                _adminLock.Lock();

                _handler.Ping();

                _adminLock.Unlock();

                ACTUALLINK::Trigger();
            }
            inline bool Masking() const
            {
                return (_handler.Masking());
            }
            inline bool Upgrade(const string& protocol, const string& path)
            {
                string empty;

                return (UpgradeToWebSocket(protocol, path, empty, ACTUALLINK::LocalId(), TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>()));
            }
            inline bool Upgrade(const string& protocol, const string& path, const string& query, const string& origin)
            {
                return (UpgradeToWebSocket(protocol, path, query, origin, TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>()));
            }
            inline void AbortUpgrade(const Web::WebStatus status, const string& reason)
            {
                // Do not return SWITCH if you would like to abort !!!!!
                ASSERT(status != Web::STATUS_SWITCH_PROTOCOL);

                _webSocketMessage->ErrorCode = status;
                _webSocketMessage->Message = reason;
            }
            inline void Submit(const Core::ProxyType<OUTBOUND>& element)
            {
                _adminLock.Lock();

                if ((IsSuspended() == false) && (IsOpen() == true) && (_serializerImpl.Submit(element) == true)) {

                    _adminLock.Unlock();

                    ACTUALLINK::Trigger();
                }
                else {
                    _adminLock.Unlock();
                }
            }
            inline uint32_t Close(const uint32_t waitTime)
            {
                uint32_t result = 0;

                _adminLock.Lock();

                if (IsSuspended() == false) {
                    if ((_state & WEBSOCKET) != 0) {
                        // Send out a close message
                        // TODO: Creat a message we can SEND
                    }

                    // Do not accept any new messages.
                    _state = static_cast<EnumlinkState>(_state | SUSPENDED);
                }

                _adminLock.Unlock();

                result = CheckForClose(waitTime);

                return (result);
            }

            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                uint16_t result = 0;

                _adminLock.Lock();

                _state = static_cast<EnumlinkState>(_state | ACTIVITY);

                if ((_state & WEBSOCKET) != 0) {
                    if (maxSendSize > 4) {
                        result = _parent.SendData(&(dataFrame[4]), (maxSendSize - 4));

                        result = _handler.Encoder(dataFrame, (maxSendSize - 4), result);
                    }
                }
                else {
                    result = _serializerImpl.Serialize(dataFrame, maxSendSize);
                }

                _adminLock.Unlock();

                if (result == 0) {
                    CheckForClose(0);
                }

                return (result);
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                uint16_t result = 0;

                _adminLock.Lock();

                _state = static_cast<EnumlinkState>(_state | ACTIVITY);

                if ((_state & WEBSOCKET) != 0) {
                    bool tooSmall = false;

                    // check for multiple messages if available...
                    while ((result < receivedSize) && (tooSmall == false)) {
                        uint16_t actualDataSize = receivedSize - result;
                        uint16_t headerSize = _handler.Decoder(const_cast<uint8_t*>(&dataFrame[result]), actualDataSize);

                        tooSmall = ((headerSize == 0) && (actualDataSize == 0));

                        if (tooSmall == false) {
                            if ((_handler.FrameType() & 0xF0) != 0) {
								
								TRACE_L1("Oops we received uncomprehensable data on the web socket 0x%X", _handler.FrameType());

                                // Oops we got an error, Change link state !!
                                // ACTUALLINK::Close(0);

                                // Nothing we can do with this shit..
                                result = receivedSize;
                            }
                            else if ((_handler.FrameType() & 0x08) != 0) {
                                // Build the associated message with this..
                                // _commandData += dataFrame

                                // If the message is complete execute the required command..
                                if (_handler.IsCompleteMessage() == true) {
                                    // It a control frame, react with the proper action.
                                    if (_handler.FrameType() == WebSocket::Protocol::PING) {
                                        // Send a PONG
                                        _handler.Pong();
                                        ACTUALLINK::Trigger();
                                    }
                                    else if (_handler.FrameType() == WebSocket::Protocol::CLOSE) {
                                        // Send a close response
                                        _handler.Close();
                                        ACTUALLINK::Trigger();
                                    }
                                    else if (_handler.FrameType() == WebSocket::Protocol::PONG) {
                                        if (_pingFireTime != 0) {
                                            TRACE_L1("Ping acknowledged by a pong in %d (uS)", static_cast<uint32_t>(static_cast<uint64_t>(Core::Time::Now().Ticks() - _pingFireTime)));
                                            _pingFireTime = 0;
                                        }
                                        else {
                                            TRACE_L1("Pong received but nu ping requested ??? [%d] ", __LINE__);
                                        }
                                    }
                                    else {
                                        // Unknown control command, log an error.
                                        TRACE_L1("WebSocket unknown command received (%d)", _handler.FrameType());
                                    }

                                    _commandData.clear();
                                }

                                result += headerSize; // actualDataSize
                            }
                            else {
                                _parent.ReceiveData(&(dataFrame[result + headerSize]), actualDataSize);

                                result += (headerSize + actualDataSize);
                            }
                        }
                    }
                }
                else {
                    result = _deserialiserImpl.Deserialize(dataFrame, receivedSize);
                }

                _adminLock.Unlock();

                if (result == 0) {
                    CheckForClose(0);
                }

                return (result);
            }

            // Signal a state change, Opened, Closed, Accepted or Error
            virtual void StateChange()
            {
                _adminLock.Lock();

                // If the connection is closed by peer 'during' socket write, cleanup response message
                if (IsClosed() == true) {
                    _serializerImpl.Flush();
                }

                _parent.StateChange();

                _adminLock.Unlock();
            }

            inline void ResetActivity()
            {
                _state = static_cast<EnumlinkState>(_state & (~ACTIVITY));
            }

            inline bool HasActivity() const
            {
                return ((_state & ACTIVITY) != 0);
            }

        private:
            inline uint32_t CheckForClose(uint32_t waitTime)
            {
                uint32_t result = 0;

                if ((IsSuspended() == true) && (_serializerImpl.IsIdle() == true) && (_deserialiserImpl.IsIdle() == true) && (_parent.IsIdle() == true)) {
                    result = ACTUALLINK::Close(waitTime);
                }
                else
                    while (waitTime > 0) {
                        uint32_t sleepTime = (waitTime > 100 ? 100 : waitTime);

                        if (sleepTime > 0) {
                            SleepMs(sleepTime);

                            if (waitTime != Core::infinite) {
                                // Sleep for 100 ms and try again
                                waitTime -= sleepTime;
                            }
                        }
                        if ((IsSuspended() == true) && (_serializerImpl.IsIdle() == true) && (_deserialiserImpl.IsIdle() == true) && (_parent.IsIdle() == true)) {
                            result = ACTUALLINK::Close(waitTime);

                            waitTime = 0;
                        }
                    }

                return (result);
            }
            inline void Serialized(const Core::ProxyType<OUTBOUND>& element)
            {
                _parent.Send(Core::proxy_cast<OUTBOUND>(element));
            }
            inline void Deserialized(Core::ProxyType<INBOUND>& element)
            {
                ReceivedWebSocket(element, TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>());
            }
            inline bool LinkBody(Core::ProxyType<INBOUND>& element)
            {
                _parent.LinkBody(element);

                return (element->HasBody());
            }
            inline void UpgradeCompleted()
            {
                UpgradeCompleted(TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>());
            }

            // ----------------------------------------------------------------------------------------------
            // SERVER upgrade to WebSocket, Diffrentiation via compiletime type: const TemplateIntToType<1>&
            // ----------------------------------------------------------------------------------------------
            inline bool UpgradeToWebSocket(const string& /* protocol */, const string& /* path */, const string& /* query */, const string& /* origin */, const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // Servers can not upgrade to websockets
                ASSERT(false);

                return (false);
            }
            inline void ReceivedWebSocket(Core::ProxyType<INBOUND>& element, const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // we are still in the accepting mode
                if ((element->Upgrade.IsSet()) && (element->Upgrade.Value() == Request::UPGRADE_WEBSOCKET) && (element->Connection.IsSet()) && (element->Connection.Value() == Request::CONNECTION_UPGRADE)) {
                    // Multiple message might be coming in, protect the state before we make assumptions on it value.
                    _adminLock.Lock();

                    if ((_state & WEBSERVER) == 0) {
                        _webSocketMessage->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        _webSocketMessage->Message = _T("State of the link can not be upgraded.");
                    }
                    else {
                        _state = static_cast<EnumlinkState>((_state & 0xF0) | UPGRADING);
                        _protocol = element->WebSocketProtocol.Value();
                        _path = element->Path;
                        if (element->Query.IsSet() == true) {
                            _query = element->Query.Value();
                        }
                        else {
                            _query.clear();
                        }

                        _webSocketMessage->Message.clear();
                        _webSocketMessage->ErrorCode = Web::STATUS_SWITCH_PROTOCOL;

                        _parent.StateChange();

                        if (_webSocketMessage->ErrorCode != Web::STATUS_SWITCH_PROTOCOL) {
                            _state = static_cast<EnumlinkState>((_state & 0xF0) | WEBSERVER);
                            _path.clear();
                            _query.clear();
                            _protocol.clear();
                        }
                        else {
                            _webSocketMessage->Connection = Web::Response::CONNECTION_UPGRADE;
                            _webSocketMessage->Upgrade = Web::Response::UPGRADE_WEBSOCKET;
                            _webSocketMessage->WebSocketAccept = _handler.ResponseKey(element->WebSocketKey.Value());
                            if (_protocol.empty() == false) {
                                _webSocketMessage->WebSocketProtocol = _protocol;
                            }
                        }
                    }

                    // Send out the result of the upgraded message.
                    _serializerImpl.Submit(_webSocketMessage);

                    _adminLock.Unlock();

                    ACTUALLINK::Trigger();
                }
                else {
                    _parent.Received(element);
                }
            }
            inline void UpgradeCompleted(const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // We send back the response on what we upgraded, Assuming it was succesfull, we are upgraded.
                if (_webSocketMessage->ErrorCode == Web::STATUS_SWITCH_PROTOCOL) {
                    ASSERT((_state & UPGRADING) != 0);

                    _adminLock.Lock();

                    _state = static_cast<EnumlinkState>((_state & 0xF0) | WEBSOCKET);

                    _parent.StateChange();

                    _adminLock.Unlock();
                }
            }

            // ----------------------------------------------------------------------------------------------
            // CLIENT upgrade to WebSocket, Diffrentiation via compiletime type: const TemplateIntToType<0>&
            // ----------------------------------------------------------------------------------------------
            inline bool UpgradeToWebSocket(const string& protocol, const string& path, const string& query, const string& origin, const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                bool result = false;

                _adminLock.Lock();

                if ((_state & WEBSERVER) != 0) {
                    result = true;
                    _state = static_cast<EnumlinkState>((_state & 0xF0) | UPGRADING);
                    _origin = (origin.empty() ? ACTUALLINK::LocalId() : origin);

                    _webSocketMessage->Verb = Web::Request::HTTP_GET;

                    _webSocketMessage->Origin = (origin.empty() ? ACTUALLINK::LocalId() : origin);
                    _webSocketMessage->Connection = Web::Request::CONNECTION_UPGRADE;
                    _webSocketMessage->Upgrade = Web::Request::UPGRADE_WEBSOCKET;
                    _webSocketMessage->WebSocketVersion = 13;
                    _webSocketMessage->Host = ACTUALLINK::RemoteId();
                    _webSocketMessage->WebSocketKey = _handler.RequestKey();

                    if (path.empty() == false) {
                        _webSocketMessage->Path = path;
                    }
                    if (query.empty() == false) {
                        _webSocketMessage->Query = query;
                    }
                    if (protocol.empty() == false) {
                        _webSocketMessage->WebSocketProtocol = protocol;
                    }

                    _query = query;
                    _path = path;
                    _protocol = protocol;

                    _serializerImpl.Submit(_webSocketMessage);
                    Trigger();
                }

                _adminLock.Unlock();

                return (result);
            }
            inline void UpgradeCompleted(const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                // We send out the request to upgrade. So what wait for the answer...
            }
            inline void ReceivedWebSocket(Core::ProxyType<INBOUND>& element, const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                // We might receive a response on the update request
                if ((_webSocketMessage.IsValid() == true) && (element->ErrorCode == Web::STATUS_SWITCH_PROTOCOL) && (element->WebSocketAccept.Value() == _handler.ResponseKey(_webSocketMessage->WebSocketKey.Value()))) {
                    ASSERT((_state & UPGRADING) != 0);

                    _adminLock.Lock();

                    // Seems like we succeeded, turn on the link..
                    _state = static_cast<EnumlinkState>((_state & 0xF0) | WEBSOCKET);

                    _parent.StateChange();

                    _adminLock.Unlock();
                }
                else {
                    _parent.Received(element);
                }
            }

        private:
            WebSocket::Protocol _handler;
            ParentClass& _parent;
            Core::CriticalSection _adminLock;
            EnumlinkState _state;
            SerializerImpl _serializerImpl;
            DeserializerImpl _deserialiserImpl;
            string _path;
            string _protocol;
            string _query;
            string _origin;
            string _commandData;
            Core::ProxyType<typename OUTBOUND::BaseElement> _webSocketMessage;
            uint64_t _pingFireTime;
        };

    public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename Arg1>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1)
            : _channel(*this, binary, masking, queueSize, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2)
            : _channel(*this, binary, masking, queueSize, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, binary, masking, queueSize, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, binary, masking, queueSize, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, binary, masking, queueSize, arg1, arg2, arg3, arg4, arg5)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, binary, masking, queueSize, arg1, arg2, arg3, arg4, arg5, arg6)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, binary, masking, queueSize, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        {
        }
        template <typename Arg1>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1)
            : _channel(*this, binary, masking, queueSize, allocator, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2, arg3, arg4, arg5)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2, arg3, arg4, arg5, arg6)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, binary, masking, queueSize, allocator, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        {
        }

#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

        virtual ~WebSocketLinkType()
        {
        }

    public:
        inline LINK& Link()
        {
            return (_channel);
        }
        inline const LINK& Link() const
        {
            return (_channel);
        }
        inline string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        inline string LocalId() const
        {
            return (_channel.LocalId());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        inline bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        inline bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        inline bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        inline bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        inline bool IsCompleted () const
        {
            return (_channel.IsCompleted());
        }
        inline void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        inline bool Binary() const
        {
            return (_channel.Binary());
        }
        inline void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        inline bool Masking() const
        {
            return (_channel.Masking());
        }
        inline void ResetActivity()
        {
            return (_channel.ResetActivity());
        }
        inline bool HasActivity() const
        {
            return (_channel.HasActivity());
        }
        inline const string& Path() const
        {
            return (_channel.Path());
        }
        inline const string& Query() const
        {
            return (_channel.Query());
        }
        inline const string& Protocol() const
        {
            return (_channel.Protocol());
        }
        inline bool Upgrade(const string& protocol, const string& path, const string& query, const string& origin)
        {
            return (_channel.Upgrade(protocol, path, query, origin));
        }
        inline void AbortUpgrade(const Web::WebStatus status, const string& reason)
        {
            return (_channel.AbortUpgrade(status, reason));
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline void Ping()
        {
            _channel.Ping();
        }
        inline void Trigger()
        {
            _channel.Trigger();
        }
        inline void Flush()
        {
            _channel.Flush();
        }
        inline void Submit(Core::ProxyType<OUTBOUND> element)
        {
            _channel.Submit(element);
        }

        virtual void LinkBody(Core::ProxyType<INBOUND>& element) = 0;
        virtual void Received(Core::ProxyType<INBOUND>& element) = 0;
        virtual void Send(const Core::ProxyType<OUTBOUND>& element) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual void StateChange() = 0;
        virtual bool IsIdle() const = 0;

    private:
        HandlerType<LINK> _channel;
    };

    template <typename LINK>
    class WebSocketClientType {
    private:
        typedef WebSocketClientType<LINK> ThisClass;

        template <typename ACTUALLINK>
        class Handler : public WebSocketLinkType<ACTUALLINK, Web::Response, Web::Request, WebSocket::ResponseAllocator&> {
        private:
            typedef WebSocketLinkType<ACTUALLINK, Web::Response, Web::Request, WebSocket::ResponseAllocator&> BaseClass;

            Handler();
            Handler(const Handler<ACTUALLINK>& copy);
            Handler<ACTUALLINK>& operator=(const Handler<ACTUALLINK>& RHS);

        public:
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance())
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2, arg3)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2, arg3, arg4)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            ~Handler()
            {
            }

        public:
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            virtual bool IsIdle() const
            {
                return (_parent.IsIdle());
            }
            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
                _parent.StateChange();

                if ((BaseClass::IsOpen() == true) && (BaseClass::IsWebSocket() == false)) {
                    BaseClass::Upgrade(_protocol, _path, _query, _origin);
                }
            }
            virtual void Received(Core::ProxyType<Web::Response>& text)
            {
                // This is a pure WebSocket, no web responses !!!!
                TRACE_L1("Received a response(full) on a Websocket (%d)", 0);
            }
            virtual void Send(const Core::ProxyType<Web::Request>& text)
            {
                // This is a pure WebSocket, no web responses !!!!
                ASSERT(false);
            }
            virtual void LinkBody(Core::ProxyType<Web::Response>& element)
            {
                // This is a pure WebSocket, no web requests !!!!
                TRACE_L1("Received a response(full) on a Websocket (%d)", 0);
            }

        private:
            ThisClass& _parent;
            string _path;
            string _protocol;
            string _query;
            string _origin;
        };

    private:
        WebSocketClientType() = delete;
        WebSocketClientType(const WebSocketClientType<LINK>& copy) = delete;
        WebSocketClientType<LINK>& operator=(const WebSocketClientType<LINK>& RHS) = delete;

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
		WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking)
            : _channel(*this, path, protocol, query, origin, binary, masking)
        {
        }
        template <typename Arg1>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2, arg3, arg4, arg5)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2, arg3, arg4, arg5, arg6)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, path, protocol, query, origin, binary, masking, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        {
        }
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
		virtual ~WebSocketClientType()
        {
        }

    public:
        inline LINK& Link()
        {
            return (_channel.Link());
        }
        inline const LINK& Link() const
        {
            return (_channel.Link());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        inline bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        inline bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        inline bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        inline bool IsCompleted () const
        {
            return (_channel.IsCompleted());
        }
        inline void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        inline bool Binary() const
        {
            return (_channel.Binary());
        }
        inline void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        inline bool Masking() const
        {
            return (_channel.Masking());
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline void Trigger()
        {
            _channel.Trigger();
        }
        inline string LocalId() const
        {
            return (_channel.LocalId());
        }
        inline void Ping()
        {
            _channel.Ping();
        }

        virtual bool IsIdle() const = 0;
        virtual void StateChange() = 0;
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

    private:
        Handler<LINK> _channel;
    };

    template <typename LINK>
    class WebSocketServerType {
    private:
        typedef WebSocketServerType<LINK> ThisClass;

        template <typename ACTUALLINK>
        class Handler : public WebSocketLinkType<ACTUALLINK, Web::Request, Web::Response, WebSocket::RequestAllocator&> {
        private:
            typedef WebSocketLinkType<ACTUALLINK, Web::Request, Web::Response, WebSocket::RequestAllocator&> BaseClass;

            Handler() = delete;
            Handler(const Handler<ACTUALLINK>& copy) = delete;
            Handler<ACTUALLINK>& operator=(const Handler<ACTUALLINK>& RHS) = delete;

        public:
            Handler(ThisClass& parent, const bool binary, const bool masking)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance())
                , _parent(parent)
            {
            }
            template <typename Arg1>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            Handler(ThisClass& parent, const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _parent(parent)
            {
            }
            ~Handler()
            {
            }

        public:
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            virtual bool IsIdle() const
            {
                return (_parent.IsIdle());
            }
            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
                _parent.StateChange();
            }
            virtual void Received(Core::ProxyType<Web::Request>& text)
            {
                // This is a pure WebSocket, no web responses !!!!
                TRACE_L1("Received a request(full) on a Websocket (%d)", 0);
            }
            virtual void Send(const Core::ProxyType<Web::Response>& text)
            {
                // This is a pure WebSocket, no web responses !!!!
                ASSERT(false);
            }
            virtual void LinkBody(Core::ProxyType<Web::Request>& element)
            {
                // This is a pure WebSocket, no web requests !!!!
                TRACE_L1("Received a request(body) on a Websocket (%d)", 0);
            }

        private:
            ThisClass& _parent;
        };

    private:
        WebSocketServerType();
        WebSocketServerType(const WebSocketServerType<LINK>& copy);
        WebSocketServerType<LINK>& operator=(const WebSocketServerType<LINK>& RHS);

    public:
        WebSocketServerType(const bool binary, const bool masking)
            : _channel(*this, binary, masking)
        {
        }
        template <typename Arg1>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1)
            : _channel(*this, binary, masking, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2)
            : _channel(*this, binary, masking, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, binary, masking, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, binary, masking, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, binary, masking, arg1, arg2, arg3, arg4, arg5)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, binary, masking, arg1, arg2, arg3, arg4, arg5, arg6)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        WebSocketServerType(const bool binary, const bool masking, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, binary, masking, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        {
        }
        virtual ~WebSocketServerType()
        {
        }

    public:
        inline LINK& Link()
        {
            return (_channel.Link());
        }
        inline const LINK& Link() const
        {
            return (_channel.Link());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsWebSocket());
        }
        inline bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        inline bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        inline bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        inline bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        inline bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        inline bool IsCompleted () const
        {
            return (_channel.IsCompleted());
        }
        inline void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        inline bool Binary() const
        {
            return (_channel.Binary());
        }
        inline void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        inline bool Masking() const
        {
            return (_channel.Masking());
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline void Trigger()
        {
            _channel.Trigger();
        }
        inline string LocalId() const
        {
            return (_channel.LocalId());
        }
        inline void Ping()
        {
            _channel.Ping();
        }

        virtual bool IsIdle() const = 0;
        virtual void StateChange() = 0;
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

    private:
        Handler<LINK> _channel;
    };
}
} // namespace WPEFramework.HTTP

#endif
