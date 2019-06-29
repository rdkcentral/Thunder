#ifndef __STREAMJSON__
#define __STREAMJSON__

#include "JSON.h"
#include "Module.h"
#include "Portability.h"
#include "StreamText.h"

namespace WPEFramework {
namespace Core {
    template <typename SOURCE, typename ALLOCATOR>
    class StreamJSONType {
    private:
        typedef StreamJSONType<SOURCE, ALLOCATOR> ParentClass;

		class SerializerImpl {
        public:
            SerializerImpl() = delete;
            SerializerImpl(const SerializerImpl&) = delete;
            SerializerImpl& operator=(const SerializerImpl&) = delete;

            SerializerImpl(ParentClass& parent, const uint8_t slotSize)
                : _parent(parent)
                , _adminLock()
                , _sendQueue(slotSize)
                , _offset(0)
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
            bool Submit(const ProxyType<JSON::IElement>& entry)
            {
                _adminLock.Lock();

                _sendQueue.Add(const_cast<ProxyType<JSON::IElement>&>(entry));

                bool trigger = (_sendQueue.Count() == 1);

                _adminLock.Unlock();

                return (trigger);
            }
            inline uint16_t Serialize(char* stream, const uint16_t length) const
            {
                uint16_t loaded = 0;

                _adminLock.Lock();

                if (_sendQueue.Count() > 0) {
                    loaded = _sendQueue[0]->Serialize(stream, length, _offset);
                    if ((_offset == 0) || (loaded != length)) {

                        Core::ProxyType<JSON::IElement> current;

                        _sendQueue.Remove(0, current);
                        _offset = 0;
                    }
                }

                _adminLock.Unlock();

                return (loaded);
            }

        private:
            ParentClass& _parent;
            mutable Core::CriticalSection _adminLock;
            mutable Core::ProxyList<JSON::IElement> _sendQueue;
            mutable uint16_t _offset;
        };
        template <typename OBJECTALLOCATOR>
        class DeserializerImpl {
        public:
            DeserializerImpl() = delete;
            DeserializerImpl(const DeserializerImpl<OBJECTALLOCATOR>&) = delete;
            DeserializerImpl<OBJECTALLOCATOR>& operator=(const DeserializerImpl<OBJECTALLOCATOR>&) = delete;

            DeserializerImpl(ParentClass& parent, const uint8_t slotSize)
                : _parent(parent)
                , _factory(slotSize)
                , _current()
                , _offset(0)
            {
            }
            DeserializerImpl(ParentClass& parent, OBJECTALLOCATOR allocator)
                : _parent(parent)
                , _factory(allocator)
                , _current()
                , _offset(0)
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
            inline uint16_t Deserialize(const char* stream, const uint16_t length)
            {
                uint16_t loaded = 0;

                if (_current.IsValid() == false) {
                    _current = Core::ProxyType<Core::JSON::IElement>(_factory.Element(EMPTY_STRING));
                    _offset = 0;
            }
                if (_current.IsValid() == true) {
                    loaded = _current->Deserialize(stream, length, _offset);
                    if ((_offset == 0) || (loaded != length)) {
                        _parent.Received(_current);
                        _current.Release();
                    }
                }

                return (loaded);
            }

        private:
            ParentClass& _parent;
            ALLOCATOR _factory;
            Core::ProxyType<Core::JSON::IElement> _current;
            uint16_t _offset;
        };

        template <typename PARENTCLASS, typename ACTUALSOURCE>
        class HandlerType : public ACTUALSOURCE {
        public:
            HandlerType() = delete;
            HandlerType(const HandlerType<PARENTCLASS, ACTUALSOURCE>&) = delete;
            HandlerType<PARENTCLASS, ACTUALSOURCE>& operator=(const HandlerType<PARENTCLASS, ACTUALSOURCE>&) = delete;

            HandlerType(PARENTCLASS& parent)
                : ACTUALSOURCE()
                , _parent(parent)
            {
            }
            template <typename... Args>
            HandlerType(PARENTCLASS& parent, Args... args)
                : ACTUALSOURCE(args...)
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

    public:
        StreamJSONType(const StreamJSONType<SOURCE, ALLOCATOR>&) = delete;
        StreamJSONType<SOURCE, ALLOCATOR>& operator=(const StreamJSONType<SOURCE, ALLOCATOR>&) = delete;

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename... Args>
        StreamJSONType(uint8_t slotSize, ALLOCATOR& allocator, Args... args)
            : _channel(*this, args...)
            , _serializer(*this, slotSize)
            , _deserializer(*this, allocator)
        {
        }

        template <typename... Args>
        StreamJSONType(uint8_t slotSize, Args... args)
            : _channel(*this, args...)
            , _serializer(*this, slotSize)
            , _deserializer(*this, slotSize)
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
            return (_serializer.Serialize(reinterpret_cast<char*>(dataFrame), maxSendSize));
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            uint16_t handled = 0;

            do {
                handled += _deserializer.Deserialize(reinterpret_cast<const char*>(&dataFrame[handled]), (receivedSize - handled));

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
