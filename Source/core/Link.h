#ifndef __LINK_H
#define __LINK_H

#include "Module.h"

#include "Portability.h"
#include "Proxy.h"

namespace WPEFramework {
namespace Core {
    template <typename LINK, typename INBOUND, typename OUTBOUND, typename ALLOCATOR>
    class LinkType {
    private:
        typedef LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR> ThisClass;

        class SerializerImpl : public OUTBOUND::Serializer {
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

                DEBUG_VARIABLE(element);
                ASSERT(&element == static_cast<typename OUTBOUND::BaseElement*>(&(*(_queue[0]))));

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

        class DeserializerImpl : public INBOUND::Deserializer {
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
                DEBUG_VARIABLE(element);
                ASSERT(&element == static_cast<typename INBOUND::BaseElement*>(&(*(_current))));

                _parent.Received(_current);

                _current.Release();
            }
            virtual typename INBOUND::BaseElement* Element(const typename INBOUND::Identifier& id)
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
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
                _parent.StateChange();
            }

        private:
            PARENTCLASS& _parent;
        };

        LinkType();
        LinkType(const LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&);
        LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>& operator=(const LinkType<LINK, INBOUND, OUTBOUND, ALLOCATOR>&);

    public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename Arg1>
        LinkType(const uint8_t queueSize, Arg1 arg1)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        LinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        LinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        LinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        LinkType(const uint8_t queueSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, queueSize)
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
        {
        }

        template <typename Arg1>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1)
        {
        }
        template <typename Arg1, typename Arg2>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3, arg4)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        LinkType(const uint8_t queueSize, ALLOCATOR responseAllocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _serializerImpl(*this, queueSize)
            , _deserialiserImpl(*this, responseAllocator)
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
        {
        }
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

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
