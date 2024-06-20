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

#ifndef __WEBLINK_H
#define __WEBLINK_H

#include "Module.h"
#include "WebRequest.h"
#include "WebResponse.h"
#include "WebSerializer.h"
#include "WebTransform.h"

namespace Thunder {
namespace Web {
    template <typename LINK, typename INBOUND, typename OUTBOUND, typename ALLOCATOR, typename TRANSFORM = NoTransform>
    class WebLinkType {
    private:
        using BaseDeserializer = typename INBOUND::Deserializer;
        using BaseSerializer = typename OUTBOUND::Serializer;
        using ThisClass = WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR, TRANSFORM>;
        using AllocatorType = typename std::remove_reference<ALLOCATOR>::type;

        class SerializerImpl : public BaseSerializer {
        public:
            SerializerImpl() = delete;
            SerializerImpl(const SerializerImpl&) = delete;
            SerializerImpl& operator=(const SerializerImpl&) = delete;

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

                ASSERT(&element == static_cast<typename OUTBOUND::BaseElement*>(&(*(_queue[0]))));
                DEBUG_VARIABLE(element);

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

        class DeserializerImpl : public BaseDeserializer {
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
            void Deserialized(typename INBOUND::BaseElement& element)
            {
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));
                DEBUG_VARIABLE(element);

                _parent.Received(_current);

                _current.Release();
            }
            typename INBOUND::BaseElement* Element() override
            {
                _current = _pool.Element();

                ASSERT(_current.IsValid());

                return static_cast<typename INBOUND::BaseElement*>(&(*_current));
            }
            bool LinkBody(typename INBOUND::BaseElement& element) override
            {
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));

                _parent.LinkBody(_current);

                return (element.HasBody());
            }
            AllocatorType& Allocator()
            {
                return (_pool);
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
                , _activity(true)
                , _parent(parent)
            {
            }
            ~HandlerType() override
            {
            }

        public:
            inline void ResetActivity()
            {
                _activity = false;
            }
            inline bool HasActivity() const
            {
                return (_activity);
            }
            inline ACTUALLINK& Link()
            {
                return (*this);
            }
            inline const ACTUALLINK& Link() const
            {
                return (*this);
            }
            // Methods to extract and insert data into the socket buffers
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
            {
                _activity = true;
                return (_parent.SendData( dataFrame, maxSendSize));
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
            {
                _activity = true;
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }
            // Signal a state change, Opened, Closed or Accepted
            void StateChange() override
            {
                _parent.StateChange();
            }

        private:
            bool _activity;
            PARENTCLASS& _parent;
        };

    public:
        WebLinkType() = delete;
        WebLinkType(const WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&) = delete;
        WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        WebLinkType(const uint8_t queueSize, Args&&... args)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, std::forward<Args>(args)...)
        {
        }
        template <typename... Args>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Args&&... args)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, std::forward<Args>(args)...)
        {
        }
POP_WARNING()
        virtual ~WebLinkType()
        {
            _channel.Close(Core::infinite);
        }

    public:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<INBOUND>& element) = 0;

        // Notification of a Request received.
        virtual void Received(Core::ProxyType<INBOUND>& text) = 0;

        // Notification of a Response send.
        virtual void Send(const Core::ProxyType<OUTBOUND>& text) = 0;

        // Notification of a channel state change..
        virtual void StateChange() = 0;

        // Submit an OUTBOUND object into the channel
        inline bool Submit(const Core::ProxyType<OUTBOUND>& element)
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
        inline void Flush()
        {
            _serializerImpl.Flush();
            _deserialiserImpl.Flush();
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        inline bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        inline bool IsClose() const
        {
            return (_channel.IsClose());
        }
        inline void ResetActivity()
        {
            return (_channel.ResetActivity());
        }
        inline bool HasActivity() const
        {
            return (_channel.HasActivity());
        }
        inline LINK& Link()
        {
            return (_channel.Link());
        }
        inline const LINK& Link() const
        {
            return (_channel.Link());
        }
 
    protected:
        AllocatorType& Allocator() {
            return (_deserialiserImpl.Allocator());
        }

    private:
        inline void Trigger()
        {
            _channel.Trigger();
        }
        // -------------------------------------------------------------
        // Check for Transform methods Inbound/Outbound on _transformer
        // -------------------------------------------------------------
        IS_MEMBER_AVAILABLE(Transform, hasTransform);

        template <typename CLASSNAME = TRANSFORM>
        inline typename Core::TypeTraits::enable_if<hasTransform<CLASSNAME, uint16_t, BaseDeserializer&, uint8_t*, const uint16_t>::value, uint16_t>::type
        ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_transformer.Transform(_deserialiserImpl, dataFrame, receivedSize));
        }

        template <typename CLASSNAME = TRANSFORM>
        inline typename Core::TypeTraits::enable_if<!hasTransform<CLASSNAME, uint16_t, BaseDeserializer&, uint8_t*, const uint16_t>::value, uint16_t>::type
        ReceiveData( uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_deserialiserImpl.Deserialize(dataFrame, receivedSize));
        }

        template <typename CLASSNAME = TRANSFORM>
        inline typename Core::TypeTraits::enable_if<hasTransform<CLASSNAME, uint16_t, BaseSerializer&, uint8_t*, const uint16_t>::value, uint16_t>::type
        SendData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_transformer.Transform(_serializerImpl, dataFrame, receivedSize));
        }

        template <typename CLASSNAME=TRANSFORM>
        inline typename Core::TypeTraits::enable_if<!hasTransform<CLASSNAME, uint16_t, BaseSerializer&, uint8_t*, const uint16_t>::value, uint16_t>::type
        SendData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_serializerImpl.Serialize(dataFrame, receivedSize));
        }

    private:
        SerializerImpl _serializerImpl;
        DeserializerImpl _deserialiserImpl;
        HandlerType<ThisClass, LINK> _channel;
        TRANSFORM _transformer;
    };
}
} // Namespace Web

#endif //__WEBLINK_H
