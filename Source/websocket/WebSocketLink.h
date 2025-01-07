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

#pragma once

#include "Module.h"
#include "WebLink.h"
#include "WebRequest.h"
#include "WebResponse.h"

namespace Thunder {
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
                // Reserved ranges
                // 0x3-0x7
                // 0xB-0xF
                // Following are outside reseved 4-bit ranges
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

        public:
            Protocol() = delete;
            Protocol(const Protocol&) = delete;
            Protocol& operator=(const Protocol&) = delete;
            
            Protocol(const bool binary, const bool masking)
                : _setFlags((masking ? 0x80 : 0x00) | (binary ? 0x02 : 0x01))
                , _progressInfo(0)
                , _pendingReceiveBytes(0)
                , _frameType(TEXT)
                , _controlStatus(0)
            {
                ::memset(_scrambleKey, 0, sizeof(_scrambleKey));
            }
            ~Protocol()
            {
            }

        public:
            std::string RequestKey() const;
            std::string ResponseKey(const std::string& requestKey) const;

            void Ping()
            {
                _controlStatus |= REQUEST_PING;
            }
            void Pong()
            {
                _controlStatus |= REQUEST_PONG;
            }
            void Close()
            {
                _controlStatus |= REQUEST_CLOSE;
            }
            bool ReceiveInProgress() const
            {
                return ((_progressInfo & 0x80) != 0);
            }
            bool SendInProgress() const
            {
                return ((_progressInfo & 0x40) != 0);
            }
            bool IsCompleteMessage() const
            {
                return (_pendingReceiveBytes == 0);
            }
            void Flush()
            {
                _pendingReceiveBytes = 0;
            }
            WebSocket::Protocol::frameType FrameType() const
            {
                return (_frameType);
            }
            void Binary(const bool binary)
            {
                _setFlags = ((_setFlags & 0xFC) | (binary ? 0x02 : 0x01));
            }
            bool Binary() const
            {
                return ((_setFlags & 0x02) != 0);
            }
            void Masking(const bool masking)
            {
                _setFlags = (masking ? (_setFlags | 0x80) : (_setFlags & 0x7F));
            }
            bool Masking() const
            {
                return ((_setFlags & 0x80) != 0);
            }

            uint16_t Encoder(uint8_t* dataFrame, const uint16_t maxSendSize, const uint16_t usedSize);
            uint16_t Decoder(uint8_t* dataFrame, uint16_t& receivedSize);

        private:
            inline void GenerateMaskKey(uint8_t *maskKey)
            {
                uint32_t value;
                // Generate a new mask value
                Crypto::Random(value);
                maskKey[0] = value & 0xFF;
                maskKey[1] = (value >> 8) & 0xFF;
                maskKey[2] = (value >> 16) & 0xFF;
                maskKey[3] = (value >> 24) & 0xFF;
            }

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
        enum EnumlinkState : uint8_t {
            WEBSERVICE = 0x01,
            UPGRADING = 0x02,
            WEBSOCKET = 0x04,
            SUSPENDED = 0x08,
            ACTIVITY  = 0x10
        };

        DEPRECATED constexpr static EnumlinkState WEBSERVER { EnumlinkState::WEBSERVICE };

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
                virtual ~SerializerImpl() = default;

            public:
                bool IsIdle() const
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
                uint16_t Serialize(uint8_t stream[], const uint16_t maxLength)
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
                    } else {
                        _parent.Serialized(Core::ProxyType<OUTBOUND>(realItem));
                    }

                    _adminLock.Lock();

                    if ((_parent.IsSuspended() == false) && (_queue.Count() > 0)) {
                        OUTBOUND::Serializer::Submit(*(_queue[0]));
                        _adminLock.Unlock();

                        _parent.Trigger();
                    } else {
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
                virtual ~DeserializerImpl() = default;

            public:
                bool IsIdle() const
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

        public:
            HandlerType() = delete;
            HandlerType(const HandlerType<ACTUALLINK>&) = delete;
            HandlerType<ACTUALLINK>& operator=(const HandlerType<ACTUALLINK>&) = delete;
            
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            template <typename... Args>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, Args&&... args)
                : ACTUALLINK(std::forward<Args>(args)...)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVICE)
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
            template <typename... Args>
            HandlerType(ParentClass& parent, const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Args&&... args)
                : ACTUALLINK(std::forward<Args>(args)...)
                , _handler(binary, masking)
                , _parent(parent)
                , _adminLock()
                , _state(WEBSERVICE)
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
POP_WARNING()
            ~HandlerType() override {
                // If this assert fires, it means the socket was not closed
                // by the one who opened it. That is unexpected. The creater
                // of this link, should (besides opening it) also close it.
                ASSERT(ACTUALLINK::IsClosed() == true);
                ACTUALLINK::Close(Core::infinite);
            }

        public:
            bool IsOpen() const
            {
                return ((ACTUALLINK::IsOpen() == true) && ((State() & SUSPENDED) == 0));
            }
            bool IsSuspended() const
            {
                return ((ACTUALLINK::IsSuspended() == true) || ((State() & SUSPENDED) != 0));
            }
            bool IsClosed() const
            {
                return (ACTUALLINK::IsClosed() == true);
            }
            bool IsWebServer() const
            {
                return ((State() & WEBSERVICE) != 0);
            }
            bool IsUpgrading() const
            {
                return ((State() & UPGRADING) != 0);
            }
            bool IsWebSocket() const
            {
                return ((State() & WEBSOCKET) != 0);
            }
            bool IsCompleted() const
            {
                return (_handler.ReceiveInProgress() == false);
            }
            const string& Path() const
            {
                return (_path);
            }
            const ProtocolsArray& Protocols() const
            {
                return (_protocol);
            }
            void Protocols(const ProtocolsArray& protocols)
            {
                _protocol = protocols;
            }
            const string& Query() const
            {
                return (_query);
            }
            const string& Origin() const
            {
                return (_origin);
            }
            void Binary(const bool binary)
            {
                _handler.Binary(binary);
            }
            bool Binary() const
            {
                return (_handler.Binary());
            }
            void Masking(const bool masking)
            {
                _handler.Masking(masking);
            }
            void Ping()
            {
                _pingFireTime = Core::Time::Now().Ticks();

                _adminLock.Lock();

                _handler.Ping();

                _adminLock.Unlock();

                ACTUALLINK::Trigger();
            }
            void Pong()
            {
                _pingFireTime = Core::Time::Now().Ticks();

                _adminLock.Lock();

                _handler.Pong();

                _adminLock.Unlock();

                ACTUALLINK::Trigger();
            }
            bool Masking() const
            {
                return (_handler.Masking());
            }
            bool Upgrade(const string& protocol, const string& path)
            {
                string empty;

                return (UpgradeToWebSocket(protocol, path, empty, ACTUALLINK::LocalId(), TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>()));
            }
            bool Upgrade(const string& protocol, const string& path, const string& query, const string& origin)
            {
                return (UpgradeToWebSocket(protocol, path, query, origin, TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>()));
            }
            void AbortUpgrade(const Web::WebStatus status, const string& reason)
            {
                // Do not return SWITCH if you would like to abort !!!!!
                ASSERT(status != Web::STATUS_SWITCH_PROTOCOL);

                _webSocketMessage->ErrorCode = status;
                _webSocketMessage->Message = reason;
            }
            void Submit(const Core::ProxyType<OUTBOUND>& element)
            {
                _adminLock.Lock();

                if ((IsSuspended() == false) && (IsOpen() == true) && (_serializerImpl.Submit(element) == true)) {

                    _adminLock.Unlock();

                    ACTUALLINK::Trigger();
                } else {
                    _adminLock.Unlock();
                }
            }
            uint32_t Close(const uint32_t waitTime)
            {
                uint32_t result = 0;

                _adminLock.Lock();

                if (IsSuspended() == false) {
                    if ((State() & WEBSOCKET) != 0) {
                        // Send out a close message
                        // TODO: Creat a message we can SEND
                    }

                    // Do not accept any new messages.
                    _state |= SUSPENDED;
                }

                _adminLock.Unlock();

                result = CheckForClose(waitTime);

                return (result);
            }

            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                uint16_t result = 0;

                _adminLock.Lock();

                _state |= ACTIVITY;

                if ((_state & WEBSOCKET) != 0) {
                    if (maxSendSize > 8) {
                        result = _parent.SendData(&(dataFrame[4]), (maxSendSize - 8));

                        result = _handler.Encoder(dataFrame, (maxSendSize - 8), result);
                    }
                } else {
                    result = _serializerImpl.Serialize(dataFrame, maxSendSize);
                }

                _adminLock.Unlock();

                if (result == 0) {
                    CheckForClose(0);
                }

                return (result);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                uint16_t result = 0;

                _adminLock.Lock();

                _state |= ACTIVITY;

                if ((_state & WEBSOCKET) != 0) {
                    bool tooSmall = false;

                    // check for multiple messages if available...
                    while ((result < receivedSize) && (tooSmall == false)) {
                        uint16_t actualDataSize = receivedSize - result;
                        uint16_t headerSize = _handler.Decoder(const_cast<uint8_t*>(&dataFrame[result]), actualDataSize);
                        uint64_t payloadSizeInControlFrame;

                        tooSmall = ((headerSize == 0) && (actualDataSize == 0));

                        if (tooSmall == false) {
                            if ((_handler.FrameType() & 0xF0) != 0) {

                                TRACE_L1("Oops we received uncomprehensable data on the web socket 0x%X", _handler.FrameType());

                                // Oops we got an error, Change link state !!
                                // ACTUALLINK::Close(0);

                                // Nothing we can do with this shit..
                                result = receivedSize;
                            } else if ((_handler.FrameType() & 0x08) != 0) {
                                // Build the associated message with this..
                                // _commandData += dataFrame

                                // If the message is complete execute the required command..
                                if (_handler.IsCompleteMessage() == true) {
                                    // It a control frame, react with the proper action.
                                    if (_handler.FrameType() == WebSocket::Protocol::PING) {
                                        // Send a PONG
                                        _handler.Pong();
                                        ACTUALLINK::Trigger();
                                    } else if (_handler.FrameType() == WebSocket::Protocol::CLOSE) {
                                        // Send a close response
                                        _handler.Close();
                                        ACTUALLINK::Trigger();
                                    } else if (_handler.FrameType() == WebSocket::Protocol::PONG) {
                                        if (_pingFireTime != 0) {
                                            TRACE_L1("Ping acknowledged by a pong in %d (uS)", static_cast<uint32_t>(static_cast<uint64_t>(Core::Time::Now().Ticks() - _pingFireTime)));
                                            _pingFireTime = 0;
                                        } else {
                                            TRACE_L1("Pong received but nu ping requested ??? [%d] ", __LINE__);
                                        }
                                    } else {
                                        // Unknown control command, log an error.
                                        TRACE_L1("WebSocket unknown command received (%d)", _handler.FrameType());
                                    }

                                    _commandData.clear();
                                }

                                payloadSizeInControlFrame = 0;
                                // skip payload bytes for control frames:
                                if (headerSize > 1) {
                                   payloadSizeInControlFrame = dataFrame[result+1] & 0x7F;
								   if (payloadSizeInControlFrame == 126) {
									   if (headerSize > 3) {
										   payloadSizeInControlFrame = ((dataFrame[result + 2] << 8) + dataFrame[result + 3]);
									   }
									   else {
										   TRACE_L1("Header too small for 16-bit extended payload size");
										   payloadSizeInControlFrame = 0;
									   }
								   }
								   else if (payloadSizeInControlFrame == 127) {
									   if (headerSize > 9) {
										   payloadSizeInControlFrame = dataFrame[result + 9];
										   for (int i = 8; i >= 2; i--) payloadSizeInControlFrame = (payloadSizeInControlFrame << 8) + dataFrame[result + i];
									   }
									   else {
										   TRACE_L1("Header too small for 64-bit jumbo payload size ");
										   payloadSizeInControlFrame = 0;
									   }
								   }
                                }

                                result += static_cast<uint16_t>(headerSize + payloadSizeInControlFrame); // actualDataSize

                            } else {
                                if (actualDataSize != 0) {
                                   _parent.ReceiveData(&(dataFrame[result + headerSize]), actualDataSize);
                                }

                                result += (headerSize + actualDataSize);
                            }
                        }
                    }
                } else {
                    result = _deserialiserImpl.Deserialize(dataFrame, receivedSize);
                }

                _adminLock.Unlock();

                if (result == 0) {
                    CheckForClose(0);
                }

                return (result);
            }

            // Signal a state change, Opened, Closed, Accepted or Error
            void StateChange() override
            {
                _adminLock.Lock();

                // If the connection is closed by peer 'during' socket write, cleanup response message
                if (IsClosed() == true) {
                    _serializerImpl.Flush();
                }

                _parent.StateChange();

                _adminLock.Unlock();
            }

            void ResetActivity()
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                _state &= ~ACTIVITY;
            }

            bool HasActivity() const
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);

                return ((_state & ACTIVITY) != 0);
            }

            void Lock() const {
                _adminLock.Lock();
            }

            void Unlock() const {
                _adminLock.Unlock();
            }

        private:
            inline uint16_t State() const
            {
                return (_state.load(Core::memory_order::memory_order_relaxed));
            }
            uint32_t CheckForClose(uint32_t waitTime)
            {
                uint32_t result = 0;

                if ((IsSuspended() == true) && (_serializerImpl.IsIdle() == true) && (_deserialiserImpl.IsIdle() == true) && (_parent.IsIdle() == true)) {
                    result = ACTUALLINK::Close(waitTime);
                } else
                    while (waitTime > 0) {
                        uint32_t sleepTime = (waitTime > 100 ? 100 : waitTime);

                        if (sleepTime > 0) {
                            SleepMs(sleepTime);

                            if (waitTime != Core::infinite) {
                                // Sleep for 100 ms and try again
                                waitTime -= sleepTime;
                            }
                        }
                        if ((IsOpen() == false) || ((IsSuspended() == true) && (_serializerImpl.IsIdle() == true) && (_deserialiserImpl.IsIdle() == true) && (_parent.IsIdle() == true))) {
                            result = ACTUALLINK::Close(waitTime);

                            waitTime = 0;
                        }
                    }

                return (result);
            }
            void Serialized(const Core::ProxyType<OUTBOUND>& element)
            {
                _parent.Send(Core::ProxyType<OUTBOUND>(element));
            }
            void Deserialized(Core::ProxyType<INBOUND>& element)
            {
                ReceivedWebSocket(element, TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>());
            }
            bool LinkBody(Core::ProxyType<INBOUND>& element)
            {
                _parent.LinkBody(element);

                return (element->HasBody());
            }
            void UpgradeCompleted()
            {
                UpgradeCompleted(TemplateIntToType<Core::TypeTraits::same_or_inherits<Web::Request, INBOUND>::value>());
            }

            // ----------------------------------------------------------------------------------------------
            // SERVER upgrade to WebSocket, Diffrentiation via compiletime type: const TemplateIntToType<1>&
            // ----------------------------------------------------------------------------------------------
            bool UpgradeToWebSocket(const string& /* protocol */, const string& /* path */, const string& /* query */, const string& /* origin */, const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // Servers can not upgrade to websockets
                ASSERT(false);

                return (false);
            }
            void ReceivedWebSocket(Core::ProxyType<INBOUND>& element, const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // we are still in the accepting mode
                if ((element->Upgrade.IsSet()) && (element->Upgrade.Value() == Request::UPGRADE_WEBSOCKET) && (element->Connection.IsSet()) && (element->Connection.Value() == Request::CONNECTION_UPGRADE)) {
                    // Multiple message might be coming in, protect the state before we make assumptions on it value.
                    _adminLock.Lock();

                    if ((_state & WEBSERVICE) == 0) {
                        _webSocketMessage->ErrorCode = Web::STATUS_INTERNAL_SERVER_ERROR;
                        _webSocketMessage->Message = _T("State of the link can not be upgraded.");
                    } else {
                        _state = static_cast<EnumlinkState>((_state & 0xF0) | UPGRADING);
                        _protocol = element->WebSocketProtocol.Value();
                        _path = element->Path;
                        if (element->Query.IsSet() == true) {
                            _query = element->Query.Value();
                        } else {
                            _query.clear();
                        }

                        _webSocketMessage->Message.clear();
                        _webSocketMessage->ErrorCode = Web::STATUS_SWITCH_PROTOCOL;

                        _parent.StateChange();

                        if (_webSocketMessage->ErrorCode != Web::STATUS_SWITCH_PROTOCOL) {
                            _state = (_state & 0xF0) | WEBSERVICE;
                            _path.clear();
                            _query.clear();
                            _protocol.Clear();
                        } else {
                            _webSocketMessage->Connection = Web::Response::CONNECTION_UPGRADE;
                            _webSocketMessage->Upgrade = Web::Response::UPGRADE_WEBSOCKET;
                            _webSocketMessage->WebSocketAccept = _handler.ResponseKey(element->WebSocketKey.Value());
                            if (_protocol.Empty() == false) {
                                //only one protocol should be selected
                                ASSERT(_protocol.Size() == 1);
                                _webSocketMessage->WebSocketProtocol = _protocol.First();
                            }
                        }
                    }

                    // Send out the result of the upgraded message.
                    _serializerImpl.Submit(_webSocketMessage);

                    _adminLock.Unlock();

                    ACTUALLINK::Trigger();
                } else {
                    _parent.Received(element);
                }
            }
            void UpgradeCompleted(const TemplateIntToType<1>& /* For compile time diffrentiation */)
            {
                // We send back the response on what we upgraded, Assuming it was succesfull, we are upgraded.
                if (_webSocketMessage->ErrorCode == Web::STATUS_SWITCH_PROTOCOL) {
                    ASSERT((_state & UPGRADING) != 0);

                    _adminLock.Lock();

                    _state = (_state & 0xF0) | WEBSOCKET;

                    _parent.StateChange();

                    _adminLock.Unlock();
                }
            }

            // ----------------------------------------------------------------------------------------------
            // CLIENT upgrade to WebSocket, Diffrentiation via compiletime type: const TemplateIntToType<0>&
            // ----------------------------------------------------------------------------------------------
            bool UpgradeToWebSocket(const string& protocol, const string& path, const string& query, const string& origin, const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                bool result = false;

                _adminLock.Lock();

                if ((_state & WEBSERVICE) != 0) {
                    result = true;
                    _state = (_state & 0xF0) | UPGRADING;
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
                        _webSocketMessage->WebSocketProtocol = Web::ProtocolsArray(protocol);
                    }

                    _query = query;
                    _path = path;
                    _protocol = Web::ProtocolsArray(protocol);

                    _serializerImpl.Submit(_webSocketMessage);
                    ACTUALLINK::Trigger();
                }

                _adminLock.Unlock();

                return (result);
            }
            void UpgradeCompleted(const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                // We send out the request to upgrade. So what wait for the answer...
            }
            void ReceivedWebSocket(Core::ProxyType<INBOUND>& element, const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                // We might receive a response on the update request
                if ((_webSocketMessage.IsValid() == true) && (element->ErrorCode == Web::STATUS_SWITCH_PROTOCOL) && (element->WebSocketAccept.Value() == _handler.ResponseKey(_webSocketMessage->WebSocketKey.Value()))) {
                    ASSERT((_state & UPGRADING) != 0);

                    _adminLock.Lock();

                    // Seems like we succeeded, turn on the link..
                    _state = (_state & 0xF0) | WEBSOCKET;

                    _parent.StateChange();

                    _adminLock.Unlock();

                    ACTUALLINK::Trigger();
                } else if ((_webSocketMessage.IsValid() == true) && (element->ErrorCode == Web::STATUS_FORBIDDEN)) {
                    ASSERT((_state & UPGRADING) != 0);

                    // Not allowed websocket
                    Close(0);
                } else {
                    _parent.Received(element);
                }
            }

        private:
            WebSocket::Protocol _handler;
            ParentClass& _parent;
            mutable Core::CriticalSection _adminLock;
            std::atomic<uint8_t> _state;
            SerializerImpl _serializerImpl;
            DeserializerImpl _deserialiserImpl;
            string _path;
            ProtocolsArray _protocol;
            string _query;
            string _origin;
            string _commandData;
            Core::ProxyType<typename OUTBOUND::BaseElement> _webSocketMessage;
            uint64_t _pingFireTime;
        };

    public:
        WebSocketLinkType() = delete;
        WebSocketLinkType(const WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& copy) = delete;
        WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const WebSocketLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& RHS) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, Args&&... args)
            : _channel(*this, binary, masking, queueSize, std::forward<Args>(args)...)
        {
        }
        template <typename... Args>
        WebSocketLinkType(const bool binary, const bool masking, const uint8_t queueSize, ALLOCATOR allocator, Args&&... args)
            : _channel(*this, binary, masking, queueSize, allocator, std::forward<Args>(args)...)
        {
        }
POP_WARNING()
        virtual ~WebSocketLinkType() = default;

    public:
        LINK& Link()
        {
            return (_channel);
        }
        const LINK& Link() const
        {
            return (_channel);
        }
        string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        string LocalId() const
        {
            return (_channel.LocalId());
        }
        bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        bool IsCompleted() const
        {
            return (_channel.IsCompleted());
        }
        void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        bool Binary() const
        {
            return (_channel.Binary());
        }
        void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        bool Masking() const
        {
            return (_channel.Masking());
        }
        void ResetActivity()
        {
            return (_channel.ResetActivity());
        }
        bool HasActivity() const
        {
            return (_channel.HasActivity());
        }
        const string& Path() const
        {
            return (_channel.Path());
        }
        const string& Query() const
        {
            return (_channel.Query());
        }
        const ProtocolsArray& Protocols() const
        {
            return (_channel.Protocols());
        }
        void Protocols(const ProtocolsArray& protocols)
        {
            _channel.Protocols(protocols);
        }
        bool Upgrade(const string& protocol, const string& path, const string& query, const string& origin)
        {
            return (_channel.Upgrade(protocol, path, query, origin));
        }
        void AbortUpgrade(const Web::WebStatus status, const string& reason)
        {
            return (_channel.AbortUpgrade(status, reason));
        }
        uint32_t WaitForLink(const uint32_t time) const
        {
            // Make sure the state does not change in the mean time.
            Lock();

            uint32_t waiting = (time == Core::infinite ? Core::infinite : time); // Expect time in MS.

            // Right, a wait till connection is closed is requested..
            while ((waiting > 0) && (IsWebSocket() == false)) {
                uint32_t sleepSlot = (waiting > SLEEPSLOT_POLLING_TIME ? SLEEPSLOT_POLLING_TIME : waiting);

                Unlock();
                // Right, lets sleep in slices of 100 ms
                SleepMs(sleepSlot);
                Lock();

                waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
            }

            uint32_t result = (((time == 0) || (IsWebSocket() == true)) ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
            Unlock();
            return (result);
        }
        uint32_t Open(const uint32_t waitTime)
        {
            _channel.Open(0);

            return WaitForLink(waitTime);
        }
        uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        void Ping()
        {
            _channel.Ping();
        }
        void Pong()
        {
            _channel.Pong();
        }
        void Trigger()
        {
            _channel.Trigger();
        }
        void Flush()
        {
            _channel.Flush();
        }
        void Submit(Core::ProxyType<OUTBOUND> element)
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

    protected:
        void Lock() const {
            _channel.Lock();
        }
        void Unlock() const {
            _channel.Unlock();
        }

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

        public:
            Handler() = delete;
            Handler(const Handler<ACTUALLINK>& copy) = delete;
            Handler<ACTUALLINK>& operator=(const Handler<ACTUALLINK>& RHS) = delete;

            template <typename... Args>
            Handler(ThisClass& parent, const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Args&&... args)
                : BaseClass(binary, masking, 1, WebSocket::ResponseAllocator::Instance(), std::forward<Args>(args)...)
                , _parent(parent)
                , _path(path)
                , _protocol(protocol)
                , _query(query)
                , _origin(origin)
            {
            }
            ~Handler() override = default;

        public:
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            bool IsIdle() const override
            {
                return (_parent.IsIdle());
            }
            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override
            {
                _parent.StateChange();

                if ((BaseClass::IsOpen() == true) && (BaseClass::IsWebSocket() == false)) {
                    BaseClass::Upgrade(_protocol, _path, _query, _origin);
                }
            }
            void Received(Core::ProxyType<Web::Response>& /* text */) override
            {
                // This is a pure WebSocket, no web responses !!!!
                TRACE_L1("Received a response(full) on a Websocket (%d)", 0);
            }
            void Send(const Core::ProxyType<Web::Request>& /* text */) override
            {
                // This is a pure WebSocket, no web responses !!!!
                ASSERT(false);
            }
            void LinkBody(Core::ProxyType<Web::Response>& /* element */) override
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

    public:
        WebSocketClientType() = delete;
        WebSocketClientType(const WebSocketClientType<LINK>& copy) = delete;
        WebSocketClientType<LINK>& operator=(const WebSocketClientType<LINK>& RHS) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        WebSocketClientType(const string& path, const string& protocol, const string& query, const string& origin, const bool binary, const bool masking, Args&&... args)
            : _channel(*this, path, protocol, query, origin, binary, masking, std::forward<Args>(args)...)
        {
        }
POP_WARNING()

        virtual ~WebSocketClientType() = default;

    public:
        LINK& Link()
        {
            return (_channel.Link());
        }
        const LINK& Link() const
        {
            return (_channel.Link());
        }
        bool IsOpen() const
        {
            return ( (_channel.IsOpen()) && (_channel.IsWebSocket()) );
        }
        bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        bool IsCompleted() const
        {
            return (_channel.IsCompleted());
        }
        void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        bool Binary() const
        {
            return (_channel.Binary());
        }
        void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        bool Masking() const
        {
            return (_channel.Masking());
        }
        uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        void Trigger()
        {
            _channel.Trigger();
        }
        string LocalId() const
        {
            return (_channel.LocalId());
        }
        void Ping()
        {
            _channel.Ping();
        }
        void Pong()
        {
            _channel.Pong();
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

        public:
            Handler() = delete;
            Handler(const Handler<ACTUALLINK>& copy) = delete;
            Handler<ACTUALLINK>& operator=(const Handler<ACTUALLINK>& RHS) = delete;

            template <typename... Args>
            Handler(ThisClass& parent, const bool binary, const bool masking, Args&&... args)
                : BaseClass(binary, masking, 1, WebSocket::RequestAllocator::Instance(), std::forward<Args>(args)...)
                , _parent(parent)
            {
            }
            ~Handler() override = default;

        public:
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            bool IsIdle() const override
            {
                return (_parent.IsIdle());
            }
            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override
            {
                _parent.StateChange();
            }
            void Received(Core::ProxyType<Web::Request>& /* text */) override
            {
                // This is a pure WebSocket, no web responses !!!!
                TRACE_L1("Received a request(full) on a Websocket (%d)", 0);
            }
            void Send(const Core::ProxyType<Web::Response>& /* text */) override
            {
                // This is a pure WebSocket, no web responses !!!!
                ASSERT(false);
            }
            void LinkBody(Core::ProxyType<Web::Request>& /* element */) override
            {
                // This is a pure WebSocket, no web requests !!!!
                TRACE_L1("Received a request(body) on a Websocket (%d)", 0);
            }

        private:
            ThisClass& _parent;
        };

    public:
        WebSocketServerType() = delete;
        WebSocketServerType(const WebSocketServerType<LINK>& copy) = delete;
        WebSocketServerType<LINK>& operator=(const WebSocketServerType<LINK>& RHS) = delete;

        template <typename... Args>
        WebSocketServerType(const bool binary, const bool masking, Args&&... args)
            : _channel(*this, binary, masking, std::forward<Args>(args)...)
        {
        }
        virtual ~WebSocketServerType() = default;

    public:
        LINK& Link()
        {
            return (_channel.Link());
        }
        const LINK& Link() const
        {
            return (_channel.Link());
        }
        bool IsOpen() const
        {
            return (_channel.IsOpen() && _channel.IsWebSocket());
        }
        bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        bool IsWebServer() const
        {
            return (_channel.IsWebServer());
        }
        bool IsWebSocket() const
        {
            return (_channel.IsWebSocket());
        }
        bool IsUpgrading() const
        {
            return (_channel.IsUpgrading());
        }
        bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        bool IsCompleted() const
        {
            return (_channel.IsCompleted());
        }
        void Binary(const bool binary)
        {
            _channel.Binary(binary);
        }
        bool Binary() const
        {
            return (_channel.Binary());
        }
        void Masking(const bool masking)
        {
            _channel.Masking(masking);
        }
        bool Masking() const
        {
            return (_channel.Masking());
        }
        uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        void Trigger()
        {
            _channel.Trigger();
        }
        string LocalId() const
        {
            return (_channel.LocalId());
        }
        void Ping()
        {
            _channel.Ping();
        }
        void Pong()
        {
            _channel.Pong();
        }

        virtual bool IsIdle() const = 0;
        virtual void StateChange() = 0;
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

    private:
        Handler<LINK> _channel;
    };
}
} // namespace Thunder.Web
