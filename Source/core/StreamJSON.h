#ifndef __STREAMJSON__
#define __STREAMJSON__

#include "Module.h"
#include "Portability.h"
#include "JSON.h"
#include "StreamText.h"

namespace WPEFramework {
namespace Core {
    template <typename SOURCE, typename ALLOCATOR>
    class StreamJSONType {
    private:
        typedef StreamJSONType<SOURCE, ALLOCATOR> ParentClass;

        class SerializerImpl : public Core::JSON::IElement::Serializer {
        private:
            SerializerImpl();
            SerializerImpl(const SerializerImpl&);
            SerializerImpl& operator=(const SerializerImpl&);

        public:
            SerializerImpl(ParentClass& parent, const uint8_t slotSize)
                : Core::JSON::IElement::Serializer()
                , _parent(parent)
                , _sendQueue(slotSize)
            {
            }
            ~SerializerImpl()
            {
            }

        public:
            inline bool IsIdle() const
            {
                return (_sendQueue.Count() == 0);
            }
            void Submit(const ProxyType<JSON::IElement>& entry)
            {
                _adminLock.Lock();

                _sendQueue.Add(const_cast<ProxyType<JSON::IElement>&>(entry));

                if (_sendQueue.Count() == 1) {
                    Core::JSON::IElement::Serializer::Submit(*entry);
                }

                _adminLock.Unlock();
            }
            virtual void Serialized(const Core::JSON::IElement& element)
            {
                _adminLock.Lock();

                Core::ProxyType<JSON::IElement> current;

                _sendQueue.Remove(0, current);

                ASSERT(&(*(current)) == &element);

                if (_sendQueue.Count() > 0) {
                    Core::JSON::IElement::Serializer::Submit(*(_sendQueue[0]));
                }

                _adminLock.Unlock();

                _parent.Send(current);
            }

        private:
            ParentClass& _parent;
            Core::ProxyList<JSON::IElement> _sendQueue;
            Core::CriticalSection _adminLock;
        };

        template <typename OBJECTALLOCATOR>
        class DeserializerImpl : public Core::JSON::IElement::Deserializer {
        private:
            DeserializerImpl();
            DeserializerImpl(const DeserializerImpl<OBJECTALLOCATOR>&);
            DeserializerImpl<OBJECTALLOCATOR>& operator=(const DeserializerImpl<OBJECTALLOCATOR>&);

        public:
            DeserializerImpl(ParentClass& parent, const uint8_t slotSize)
                : Core::JSON::IElement::Deserializer()
                , _parent(parent)
                , _factory(slotSize)
            {
            }
            DeserializerImpl(ParentClass& parent, OBJECTALLOCATOR allocator)
                : Core::JSON::IElement::Deserializer()
                , _parent(parent)
                , _factory(allocator)
            {
            }
            ~DeserializerImpl()
            {
            }

        public:
            inline bool IsIdle() const
            {
                return (_current.IsValid() == false);
            }
            virtual Core::JSON::IElement* Element(const string& identifier)
            {
                _current = _factory.Element(identifier);

                return (_current.IsValid() == true ? &(*_current) : nullptr);
            }
            virtual void Deserialized(Core::JSON::IElement& element)
            {
                ASSERT(&element == &(*_current));

                _parent.Received(_current);

                _current.Release();
            }

        private:
            ParentClass& _parent;
            ALLOCATOR _factory;
            ProxyType<JSON::IElement> _current;
        };

        template <typename PARENTCLASS, typename ACTUALSOURCE>
        class HandlerType : public ACTUALSOURCE {
        private:
            HandlerType();
            HandlerType(const HandlerType<PARENTCLASS, ACTUALSOURCE>&);
            HandlerType<PARENTCLASS, ACTUALSOURCE>& operator=(const HandlerType<PARENTCLASS, ACTUALSOURCE>&);

        public:
            HandlerType(PARENTCLASS& parent)
                : ACTUALSOURCE()
                , _parent(parent)
            {
            }
            template <typename Arg1>
            HandlerType(PARENTCLASS& parent, Arg1 arg1)
                : ACTUALSOURCE(arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2)
                : ACTUALSOURCE(arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : ACTUALSOURCE(arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10, typename Arg11>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10, Arg11 arg11)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
                , _parent(parent)
            {
            }
            ~HandlerType()
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
            virtual bool IsIdle() const
            {
                return (_parent.IsIdle());
            }

        private:
            PARENTCLASS& _parent;
        };

        typedef StreamTextType<SOURCE, ALLOCATOR> BaseClass;

        StreamJSONType(const StreamJSONType<SOURCE, ALLOCATOR>&);
        StreamJSONType<SOURCE, ALLOCATOR>& operator=(const StreamJSONType<SOURCE, ALLOCATOR>&);

    public:
		#ifdef __WIN32__
		#pragma warning(disable : 4355)
		#endif
        template <typename Arg1>
        StreamJSONType(uint8_t slotSize, Arg1 arg1)
            : _channel(*this, arg1)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2)
            : _channel(*this, arg1, arg2)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, arg1, arg2, arg3)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, arg1, arg2, arg3, arg4)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
        StreamJSONType(uint8_t slotSize, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
        {
        }
        template <typename Arg1>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1)
            : _channel(*this, arg1)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2)
            : _channel(*this, arg1, arg2)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, arg1, arg2, arg3)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, arg1, arg2, arg3, arg4)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10, typename Arg11>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10, Arg11 arg11)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }
		#ifdef __WIN32__
		#pragma warning(default : 4355)
		#endif

        virtual ~StreamJSONType()
        {
            _channel.Close(Core::infinite);
        }

    public:
        inline SOURCE& Link()
        {
            return (_channel);
        }
        virtual void Received(ProxyType<JSON::IElement>& element) = 0;
        virtual void Send(ProxyType<JSON::IElement>& element) = 0;
        virtual void StateChange() = 0;

        inline void Submit(const ProxyType<JSON::IElement>& element)
        {
            if (_channel.IsOpen() == true) {
                _serializer.Submit(element);
                _channel.Trigger();
            }
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

    private:
        virtual bool IsIdle() const
        {
            return ((_serializer.IsIdle() == true) && (_deserializer.IsIdle() == true));
        }
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            return (_serializer.Serialize(dataFrame, maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            uint16_t handled = 0;

            do {
                handled += _deserializer.Deserialize(&dataFrame[handled], (receivedSize - handled));

                // The dataframe can hold more items....
            } while (handled < receivedSize);

            return (handled);
        }

    private:
        HandlerType<ParentClass, SOURCE> _channel;
        SerializerImpl _serializer;
        DeserializerImpl<ALLOCATOR> _deserializer;
    };
}
} // namespace Core

#endif // __STREAMJSON__
