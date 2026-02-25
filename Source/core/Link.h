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
 
#ifndef __LINK_H
#define __LINK_H

#include "Module.h"

#include "Portability.h"
#include "Proxy.h"

namespace Thunder {
namespace Core {
    template <typename LINK, typename INBOUND, typename OUTBOUND, typename ALLOCATOR>
    class LinkType {
    private:
        typedef LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR> ThisClass;

        class SerializerImpl : public OUTBOUND::Serializer {
        public:
            SerializerImpl() = delete;
            SerializerImpl(const SerializerImpl&) = delete;
            SerializerImpl & operator=(const SerializerImpl&) = delete;

            SerializerImpl(ThisClass& parent, const uint8_t queueSize)
                : OUTBOUND::Serializer()
                , _parent(parent)
                , _lock()
                , _queue(queueSize)
            {
            }
            ~SerializerImpl() override = default;

        public:
            void Submit(const Core::ProxyType<OUTBOUND>& element)
            {
                _lock.Lock();

                _queue.Add(const_cast<Core::ProxyType<OUTBOUND>&>(element));

                // See if we need to push the first one..
                if (_queue.Count() == 1) {
                    OUTBOUND::Serializer::Submit(*element);
                    _lock.Unlock();

                    _parent.Trigger();
                } else {
                    _lock.Unlock();
                }
            }

        private:
            void Serialized(const typename OUTBOUND::BaseElement& element) override
            {
                _lock.Lock();

                ASSERT(_queue.Count() > 0);

                DEBUG_VARIABLE(element);
                ASSERT(&element == static_cast<typename OUTBOUND::BaseElement*>(&(*(_queue[0]))));

                _parent.Send(_queue[0]);

                _queue.Remove(0);

                // See if we have something else to push
                if (_queue.Count() > 0) {
                    OUTBOUND::Serializer::Submit(*(_queue[0]));
                    _lock.Unlock();

                    _parent.Trigger();
                } else {
                    _lock.Unlock();
                }
            }

        private:
            ThisClass& _parent;
            Core::CriticalSection _lock;
            Core::ProxyList<OUTBOUND> _queue;
        };

        class DeserializerImpl : public INBOUND::Deserializer {
        public:
            DeserializerImpl() = delete;
            DeserializerImpl(const DeserializerImpl&) = delete;
            DeserializerImpl& operator=(const DeserializerImpl&) = delete;

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
            ~DeserializerImpl() override = default;

        public:
            void Deserialized(typename INBOUND::BaseElement& element) override
            {
                DEBUG_VARIABLE(element);
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));

                _parent.Received(_current);

                _current.Release();
            }
            typename INBOUND::BaseElement* Element(const typename INBOUND::Identifier& id) override
            {
                _current = _pool.Element(id);

#ifdef __DEBUG__
                if (_current.IsValid() == false) {
                    TRACE_L1("Could not get an element.\n %d", 0);
                }
#endif

                return (_current.IsValid() ? static_cast<typename INBOUND::BaseElement*>(&(*_current)) : nullptr);
            }

        private:
            ThisClass& _parent;
            Core::ProxyType<INBOUND> _current;
            ALLOCATOR _pool;
        };

        template <typename PARENTCLASS, typename ACTUALLINK>
        class HandlerType : public ACTUALLINK {
        public:
            HandlerType() = delete;
            HandlerType(const HandlerType<PARENTCLASS, ACTUALLINK>&) = delete;
            HandlerType<PARENTCLASS, ACTUALLINK>& operator=(const HandlerType<PARENTCLASS, ACTUALLINK>&) = delete;

            template <typename... Args>
            HandlerType(PARENTCLASS& parent, Args&&... args)
                : ACTUALLINK(std::forward<Args>(args)...)
                , _parent(parent)
            {
            }
            ~HandlerType() override = default;

        public:
            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override
            {
                _parent.StateChange();
            }

        private:
            PARENTCLASS& _parent;
        };

    public:
        LinkType() = delete;
        LinkType(const LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&) = delete;
        LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        LinkType(const uint8_t queueSize, Args&&... args)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, std::forward<Args>(args)...)
        {
        }

        template <typename... Args>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Args&&... args)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, std::forward<Args>(args)...)
        {
        }
POP_WARNING()

        virtual ~LinkType()
        {
            _channel.Close(Core::infinite);
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

        // Notification of a Request received.
        virtual void Received(Core::ProxyType<INBOUND>& text) = 0;

        // Notification of a Response send.
        virtual void Send(const Core::ProxyType<OUTBOUND>& text) = 0;

        // Notification of a channel state change..
        virtual void StateChange() = 0;

        // Submit an OUTBOUND object into the channel
        bool Submit(const Core::ProxyType<OUTBOUND>& element)
        {
            if (_channel.IsOpen() == true) {
                _serializerImpl.Submit(element);
            }

            return (true);
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        inline bool HasError() const
        {
            return (_channel.HasError());
        }

    private:
        inline void Trigger()
        {
            _channel.Trigger();
        }
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            // Serialize Response
            return (_serializerImpl.Serialize(dataFrame, maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            // Deserialize Request
            return (_deserialiserImpl.Deserialize(dataFrame, receivedSize));
        }

    private:
        SerializerImpl _serializerImpl;
        DeserializerImpl _deserialiserImpl;
        HandlerType<ThisClass, LINK> _channel;
    };
}

} // Namespace Core

#endif //__LINK_H
