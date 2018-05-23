#ifndef __WEBLINK_H
#define __WEBLINK_H

#include "Module.h"
#include "WebRequest.h"
#include "WebResponse.h"
#include "WebSerializer.h"
#include "WebTransform.h"

namespace WPEFramework {
namespace Web {
    template <typename LINK, typename INBOUND, typename OUTBOUND, typename ALLOCATOR, typename TRANSFORM = NoTransform>
    class WebLinkType {
    private:
        typedef typename INBOUND::Deserializer BaseDeserializer;
        typedef typename OUTBOUND::Serializer BaseSerializer;
        typedef WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR, TRANSFORM> ThisClass;

        class SerializerImpl : public BaseSerializer {
        private:
            SerializerImpl();
            SerializerImpl(const SerializerImpl&);
            SerializerImpl& operator=(const SerializerImpl&);

        public:
            SerializerImpl(ThisClass& parent, const uint8_t queueSize)
                : OUTBOUND::Serializer()
                , _parent(parent)
                , _lock()
                , _queue(queueSize)
            {
            }
            virtual ~SerializerImpl()
            {
            }

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
                }
                else {
                    _lock.Unlock();
                }
            }

        private:
            virtual void Serialized(const typename OUTBOUND::BaseElement& element)
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
                }
                else {
                    _lock.Unlock();
                }
            }

        private:
            ThisClass& _parent;
            Core::CriticalSection _lock;
            Core::ProxyList<OUTBOUND> _queue;
        };

        class DeserializerImpl : public BaseDeserializer {
        private:
            DeserializerImpl();
            DeserializerImpl(const DeserializerImpl&);
            DeserializerImpl& operator=(const DeserializerImpl&);

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
            virtual void Deserialized(typename INBOUND::BaseElement& element)
            {
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));
                DEBUG_VARIABLE(element);

                _parent.Received(_current);

                _current.Release();
            }
            virtual typename INBOUND::BaseElement* Element()
            {
                _current = _pool.Element();

                ASSERT(_current.IsValid());

                return static_cast<typename INBOUND::BaseElement*>(&(*_current));
            }
            virtual bool LinkBody(typename INBOUND::BaseElement& element)
            {
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));

                _parent.LinkBody(_current);

                return (element.HasBody());
            }

        private:
            ThisClass& _parent;
            Core::ProxyType<INBOUND> _current;
            ALLOCATOR _pool;
        };

        template <typename PARENTCLASS, typename ACTUALLINK>
        class HandlerType : public ACTUALLINK {
        private:
            HandlerType();
            HandlerType(const HandlerType<PARENTCLASS, ACTUALLINK>&);
            HandlerType<PARENTCLASS, ACTUALLINK>& operator=(const HandlerType<PARENTCLASS, ACTUALLINK>&);

        public:
            HandlerType(PARENTCLASS& parent)
                : ACTUALLINK()
                , _parent(parent)
            {
            }
            template <typename Arg1>
            HandlerType(PARENTCLASS& parent, Arg1 arg1)
                : ACTUALLINK(arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2)
                : ACTUALLINK(arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : ACTUALLINK(arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : ACTUALLINK(arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : ACTUALLINK(arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            virtual ~HandlerType()
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
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                _activity = true;
                return (_parent.SendData(_parent, dataFrame, maxSendSize));
            }

            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                _activity = true;
                return (_parent.ReceiveData(_parent, dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
                _parent.StateChange();
            }

        private:
            bool _activity;
            PARENTCLASS& _parent;
        };

        WebLinkType();
        WebLinkType(const WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&);
        WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const WebLinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&);

    public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename Arg1>
        WebLinkType(const uint8_t queueSize, Arg1 arg1)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebLinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebLinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebLinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebLinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
        {
        }

        template <typename Arg1>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        WebLinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
        {
        }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

        virtual ~WebLinkType()
        {
            _channel.Close(Core::infinite);
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
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<INBOUND>& element) = 0;

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

    private:
        inline void Trigger()
        {
            _channel.Trigger();
        }
        // -------------------------------------------------------------
        // Check for Transform methods Inbound/Outbound on _transformer
        // -------------------------------------------------------------
        HAS_MEMBER(Transform, hasTransform);

        typedef hasTransform<TRANSFORM, uint16_t (TRANSFORM::*)(BaseDeserializer&, uint8_t* data, const uint16_t maxSize)> TraitDeserializer;
        typedef hasTransform<TRANSFORM, uint16_t (TRANSFORM::*)(BaseSerializer&, uint8_t* data, const uint16_t maxSize)> TraitSerializer;

        template <typename CLASSNAME>
        inline typename Core::TypeTraits::enable_if<CLASSNAME::TraitDeserializer::value, uint16_t>::type
        ReceiveData(const CLASSNAME&, uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_transformer.Transform(_deserialiserImpl, dataFrame, receivedSize));
        }

        template <typename CLASSNAME>
        inline typename Core::TypeTraits::enable_if<!CLASSNAME::TraitDeserializer::value, uint16_t>::type
        ReceiveData(const CLASSNAME&, uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_deserialiserImpl.Deserialize(dataFrame, receivedSize));
        }

        template <typename CLASSNAME>
        inline typename Core::TypeTraits::enable_if<CLASSNAME::TraitSerializer::value, uint16_t>::type
        SendData(const CLASSNAME&, uint8_t* dataFrame, const uint16_t receivedSize)
        {
            return (_transformer.Transform(_serializerImpl, dataFrame, receivedSize));
        }

        template <typename CLASSNAME>
        inline typename Core::TypeTraits::enable_if<!CLASSNAME::TraitSerializer::value, uint16_t>::type
        SendData(const CLASSNAME&, uint8_t* dataFrame, const uint16_t receivedSize)
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
