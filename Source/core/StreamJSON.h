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
 
#ifndef __STREAMJSON__
#define __STREAMJSON__

#include "JSON.h"
#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    template <typename SOURCE, typename ALLOCATOR, typename INTERFACE>// = Core::JSON::IElement>
    class StreamJSONType {
    private:
        typedef StreamJSONType<SOURCE, ALLOCATOR, INTERFACE> ParentClass;

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
                _sendQueue.Clear();
            }

        public:
            inline bool IsIdle() const
            {
                return (_sendQueue.Count() == 0);
            }
            bool Submit(const ProxyType<INTERFACE>& entry)
            {
                _adminLock.Lock();

                _sendQueue.Add(const_cast<ProxyType<INTERFACE>&>(entry));

                bool trigger = (_sendQueue.Count() == 1);

                _adminLock.Unlock();

                return (trigger);
            }
            inline uint16_t Serialize(uint8_t* stream, const uint16_t length) const
            {
                uint16_t loaded = 0;

                _adminLock.Lock();

                if (_sendQueue.Count() > 0) {
                    loaded = Serialize(_sendQueue[0], stream, length);
                    if ((_offset == 0) || (loaded != length)) {
                        Core::ProxyType<INTERFACE> current = _sendQueue[0];
                        _parent.Send(current);

                        _sendQueue.Remove(0);
                        _offset = 0;
                    }
                }

                _adminLock.Unlock();

                return (loaded);
            }

        private:
            inline uint16_t Serialize(const Core::ProxyType<Core::JSON::IElement>& source, uint8_t* stream, const uint16_t length) const {
                return(source->Serialize(reinterpret_cast<char*>(stream), length, _offset));
            }
            inline uint16_t Serialize(const Core::ProxyType<Core::JSON::IMessagePack>& source, uint8_t* stream, const uint16_t length) const {
                return(source->Serialize(stream, length, _offset));
            }
            
        private:
            ParentClass& _parent;
            mutable Core::CriticalSection _adminLock;
            mutable Core::ProxyList<INTERFACE> _sendQueue;
            mutable uint32_t _offset;
        };
        class DeserializerImpl {
        public:
            DeserializerImpl() = delete;
            DeserializerImpl(const DeserializerImpl&) = delete;
            DeserializerImpl& operator=(const DeserializerImpl&) = delete;

            DeserializerImpl(ParentClass& parent, const uint8_t slotSize)
                : _parent(parent)
                , _factory(slotSize)
                , _current()
                , _offset(0)
            {
            }
            DeserializerImpl(ParentClass& parent, ALLOCATOR allocator)
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
            inline uint16_t Deserialize(const uint8_t* stream, const uint16_t length)
            {
                uint16_t loaded = 0;

                if (_current.IsValid() == false) {
                    _current = Core::ProxyType<INTERFACE>(_factory.Element(EMPTY_STRING));
                    _offset = 0;
                }
                if (_current.IsValid() == true) {
                    loaded = Deserialize(_current, stream, length);
                    if ((_offset == 0) || (loaded != length)) {
                        _parent.Received(_current);
                        _current.Release();
                    }
                }

                return (loaded);
            }

        private:
            inline uint16_t Deserialize(const Core::ProxyType<Core::JSON::IElement>& source, const uint8_t* stream, const uint16_t length) {
                return(source->Deserialize(reinterpret_cast<const char*>(stream), length, _offset));
            }
            inline uint16_t Deserialize(const Core::ProxyType<Core::JSON::IMessagePack>& source, const uint8_t* stream, const uint16_t length) {
                return (source->Deserialize(stream, length, _offset));
            }

        private:
            ParentClass& _parent;
            ALLOCATOR _factory;
            Core::ProxyType<INTERFACE> _current;
            uint32_t _offset;
        };

        class HandlerType : public SOURCE {
        public:
            HandlerType() = delete;
            HandlerType(const HandlerType&) = delete;
            HandlerType& operator=(const HandlerType&) = delete;

            HandlerType(ParentClass& parent)
                : SOURCE()
                , _parent(parent)
            {
            }
            template <typename... Args>
            HandlerType(ParentClass& parent, Args... args)
                : SOURCE(args...)
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
            ParentClass& _parent;
        };

    public:
        StreamJSONType(const StreamJSONType<SOURCE, ALLOCATOR, INTERFACE>&) = delete;
        StreamJSONType<SOURCE, ALLOCATOR, INTERFACE >& operator=(const StreamJSONType<SOURCE, ALLOCATOR, INTERFACE>&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
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
POP_WARNING()

        virtual ~StreamJSONType() {
            _channel.Close(Core::infinite);
        }

    public:
        inline SOURCE& Link()
        {
            return (_channel);
        }
        virtual void Received(ProxyType<INTERFACE>& element) = 0;
        virtual void Send(ProxyType<INTERFACE>& element) = 0;
        virtual void StateChange() = 0;

        inline void Submit(const ProxyType<INTERFACE>& element)
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
        HandlerType _channel;
        SerializerImpl _serializer;
        DeserializerImpl _deserializer;
    };
}
} // namespace Core

#endif // __STREAMJSON__
